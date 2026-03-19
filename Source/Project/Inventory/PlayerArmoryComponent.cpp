// Copyright Epic Games, Inc. All Rights Reserved.

#include "Inventory/PlayerArmoryComponent.h"
#include "Character/CurrencyComponent.h"
#include "Combat/CombatComponent.h"
#include "Combat/WeaponBase.h"
#include "GameFramework/Pawn.h"
#include "Project.h"

namespace {
int32 MakeGridCellIndex(const FIntPoint &Cell, int32 GridWidth) {
  return (Cell.Y * GridWidth) + Cell.X;
}
} // namespace

UPlayerArmoryComponent::UPlayerArmoryComponent() {
  PrimaryComponentTick.bCanEverTick = false;
  EnsureLoadoutItemIdsInitialized();
}

void UPlayerArmoryComponent::BeginPlay() {
  Super::BeginPlay();
  EnsureLoadoutItemIdsInitialized();
}

void UPlayerArmoryComponent::InitializeEmptySession(int32 InitialCurrency) {
  EnsureLoadoutItemIdsInitialized();

  bHasInitializedSessionState = true;
  CachedCurrency = FMath::Max(0, InitialCurrency);
  OwnedItems.Reset();
  for (FGuid &ItemId : LoadoutItemIds) {
    ItemId = FGuid();
  }
  ActiveSlot = EWeaponLoadoutSlot::Slot1Primary;

  ApplyStateToBoundPawn();
  BroadcastArmoryChanged();
}

void UPlayerArmoryComponent::BindToPawn(APawn *NewPawn) {
  if (BoundPawn.Get() == NewPawn) {
    return;
  }

  UnbindCurrencyComponent();

  BoundPawn = NewPawn;
  BoundCombatComponent = nullptr;

  if (BoundPawn.IsValid()) {
    BoundCurrencyComponent = BoundPawn->FindComponentByClass<UCurrencyComponent>();
    BoundCombatComponent = BoundPawn->FindComponentByClass<UCombatComponent>();
    BindCurrencyComponent(BoundCurrencyComponent.Get());
  } else {
    BoundCurrencyComponent.Reset();
  }

  if (bHasInitializedSessionState) {
    ApplyStateToBoundPawn();
  }
}

void UPlayerArmoryComponent::ApplyStateToBoundPawn() {
  if (!bHasInitializedSessionState || !BoundPawn.IsValid()) {
    return;
  }

  EnsureLoadoutItemIdsInitialized();

  if (UCurrencyComponent *CurrencyComponent = BoundCurrencyComponent.Get()) {
    CurrencyComponent->SetCurrency(CachedCurrency);
  }

  if (!BoundCombatComponent.IsValid()) {
    BoundCombatComponent = BoundPawn->FindComponentByClass<UCombatComponent>();
  }
  if (!BoundCombatComponent.IsValid()) {
    BroadcastArmoryChanged();
    return;
  }

  BoundCombatComponent->ClearLoadout();
  for (int32 SlotIndex = 0; SlotIndex < LoadoutItemIds.Num(); ++SlotIndex) {
    const FInventoryItemInstance *AssignedItem = FindItemById(LoadoutItemIds[SlotIndex]);
    if (!AssignedItem || !AssignedItem->WeaponClass) {
      continue;
    }

    BoundCombatComponent->SetWeaponForSlot(
        static_cast<EWeaponLoadoutSlot>(SlotIndex), AssignedItem->WeaponClass);

    if (AWeaponBase *SpawnedWeapon = BoundCombatComponent->GetWeaponInSlot(
            static_cast<EWeaponLoadoutSlot>(SlotIndex))) {
      FWeaponAmmoSaveData AmmoSaveData;
      AmmoSaveData.AmmoInMagazine = AssignedItem->CurrentAmmoInMagazine;
      AmmoSaveData.ReserveAmmo = AssignedItem->ReserveAmmo;
      SpawnedWeapon->ApplyAmmoSaveData(AmmoSaveData);
    }
  }

  const EWeaponLoadoutSlot DesiredActiveSlot =
      IsSlotOccupied(ActiveSlot) ? ActiveSlot : FindFallbackActiveSlot();
  if (IsSlotOccupied(DesiredActiveSlot)) {
    ActiveSlot = DesiredActiveSlot;
    BoundCombatComponent->SetActiveLoadoutSlot(DesiredActiveSlot);
  }

  BroadcastArmoryChanged();
}

void UPlayerArmoryComponent::CaptureSaveData(FPlayerSaveData &OutPlayerSaveData) {
  SyncRuntimeWeaponStateFromBoundPawn();

  OutPlayerSaveData.Currency = CachedCurrency;
  OutPlayerSaveData.StorageGridWidth = GetStorageGridWidth();
  OutPlayerSaveData.StorageGridHeight = GetStorageGridHeight();
  OutPlayerSaveData.ActiveSlot = ActiveSlot;
  OutPlayerSaveData.ArmoryItems.Reset();
  OutPlayerSaveData.ArmoryItems.Reserve(OwnedItems.Num());

  for (const FInventoryItemInstance &Item : OwnedItems) {
    FArmoryItemSaveData ItemSaveData;
    ItemSaveData.ItemId = Item.ItemId;
    ItemSaveData.WeaponClass = Item.WeaponClass;
    ItemSaveData.Container = Item.Container;
    ItemSaveData.GridPlacement = Item.GridPlacement;
    ItemSaveData.LoadoutSlot = Item.LoadoutSlot;
    ItemSaveData.AmmoData.AmmoInMagazine = Item.CurrentAmmoInMagazine;
    ItemSaveData.AmmoData.ReserveAmmo = Item.ReserveAmmo;
    OutPlayerSaveData.ArmoryItems.Add(MoveTemp(ItemSaveData));
  }
}

void UPlayerArmoryComponent::RestoreFromSaveData(
    const FPlayerSaveData &PlayerSaveData) {
  EnsureLoadoutItemIdsInitialized();

  bHasInitializedSessionState = true;
  CachedCurrency = FMath::Max(0, PlayerSaveData.Currency);
  StorageGridWidth = FMath::Max(1, PlayerSaveData.StorageGridWidth);
  StorageGridHeight = FMath::Max(1, PlayerSaveData.StorageGridHeight);
  ActiveSlot = PlayerSaveData.ActiveSlot;

  OwnedItems.Reset();
  for (FGuid &LoadoutItemId : LoadoutItemIds) {
    LoadoutItemId = FGuid();
  }

  for (const FArmoryItemSaveData &ItemSaveData : PlayerSaveData.ArmoryItems) {
    FInventoryItemInstance RestoredItem;
    if (!BuildInventoryItemFromSaveData(ItemSaveData, RestoredItem)) {
      continue;
    }

    if (RestoredItem.Container == EInventoryItemContainer::LoadoutSlot) {
      const int32 SlotIndex =
          ProjectWeaponLoadout::ToIndex(static_cast<uint8>(RestoredItem.LoadoutSlot));
      if (LoadoutItemIds.IsValidIndex(SlotIndex)) {
        LoadoutItemIds[SlotIndex] = RestoredItem.ItemId;
      }
    }

    OwnedItems.Add(MoveTemp(RestoredItem));
  }

  if (!IsSlotOccupied(ActiveSlot)) {
    ActiveSlot = FindFallbackActiveSlot();
  }

  ApplyStateToBoundPawn();
  BroadcastArmoryChanged();
}

bool UPlayerArmoryComponent::CanPurchaseWeapon(TSubclassOf<AWeaponBase> WeaponClass,
                                               int32 Price) const {
  if (!bHasInitializedSessionState || !WeaponClass || Price < 0) {
    return false;
  }

  return !HasOwnedWeapon(WeaponClass) && CachedCurrency >= Price &&
         CanStoreWeapon(WeaponClass);
}

bool UPlayerArmoryComponent::PurchaseWeapon(TSubclassOf<AWeaponBase> WeaponClass,
                                            int32 Price) {
  if (!CanPurchaseWeapon(WeaponClass, Price)) {
    return false;
  }

  FInventoryItemInstance NewItem;
  if (!BuildWeaponItemInstance(WeaponClass, NewItem)) {
    return false;
  }

  FInventoryGridPlacement Placement;
  if (!FindFirstFit(NewItem, Placement)) {
    return false;
  }

  bool bSpentCurrency = false;
  if (UCurrencyComponent *CurrencyComponent = BoundCurrencyComponent.Get()) {
    bSpentCurrency = CurrencyComponent->SpendCurrency(Price);
  } else {
    CachedCurrency = FMath::Max(0, CachedCurrency - Price);
    bSpentCurrency = true;
  }

  if (!bSpentCurrency) {
    return false;
  }

  NewItem.Container = EInventoryItemContainer::StorageGrid;
  NewItem.GridPlacement = Placement;
  OwnedItems.Add(MoveTemp(NewItem));
  BroadcastArmoryChanged();
  return true;
}

bool UPlayerArmoryComponent::CanStoreWeapon(
    TSubclassOf<AWeaponBase> WeaponClass) const {
  FInventoryItemInstance PendingWeaponItem;
  if (!BuildWeaponItemInstance(WeaponClass, PendingWeaponItem)) {
    return false;
  }

  FInventoryGridPlacement Placement;
  return FindFirstFit(PendingWeaponItem, Placement);
}

bool UPlayerArmoryComponent::HasOwnedWeapon(
    TSubclassOf<AWeaponBase> WeaponClass) const {
  return FindWeaponItem(WeaponClass) != nullptr;
}

TArray<TSubclassOf<AWeaponBase>> UPlayerArmoryComponent::GetOwnedWeapons() const {
  TArray<TSubclassOf<AWeaponBase>> WeaponClasses;
  WeaponClasses.Reserve(OwnedItems.Num());

  for (const FInventoryItemInstance &Item : OwnedItems) {
    if (Item.ItemKind == EInventoryItemKind::Weapon && Item.WeaponClass) {
      WeaponClasses.Add(Item.WeaponClass);
    }
  }

  return WeaponClasses;
}

TSubclassOf<AWeaponBase> UPlayerArmoryComponent::GetAssignedWeaponForSlot(
    EWeaponLoadoutSlot Slot) const {
  const FInventoryItemInstance *Item = FindItemById(GetLoadoutItemId(Slot));
  return Item ? Item->WeaponClass : nullptr;
}

TArray<TSubclassOf<AWeaponBase>>
UPlayerArmoryComponent::GetAssignedWeaponsBySlot() const {
  TArray<TSubclassOf<AWeaponBase>> AssignedWeapons;
  AssignedWeapons.SetNumZeroed(ProjectWeaponLoadout::SlotCount);

  for (int32 SlotIndex = 0; SlotIndex < ProjectWeaponLoadout::SlotCount; ++SlotIndex) {
    const FInventoryItemInstance *Item = FindItemById(LoadoutItemIds[SlotIndex]);
    AssignedWeapons[SlotIndex] = Item ? Item->WeaponClass : nullptr;
  }

  return AssignedWeapons;
}

bool UPlayerArmoryComponent::IsSlotOccupied(EWeaponLoadoutSlot Slot) const {
  return FindItemById(GetLoadoutItemId(Slot)) != nullptr;
}

bool UPlayerArmoryComponent::CanAssignWeaponToSlot(
    TSubclassOf<AWeaponBase> WeaponClass, EWeaponLoadoutSlot Slot) const {
  if (!bHasInitializedSessionState) {
    return false;
  }

  const FInventoryItemInstance *Item = FindWeaponItem(WeaponClass);
  if (!Item || !IsItemAllowedInLoadout(*Item, Slot)) {
    return false;
  }

  const FGuid TargetItemId = GetLoadoutItemId(Slot);
  if (!TargetItemId.IsValid() || TargetItemId == Item->ItemId) {
    return true;
  }

  return CanRelocateLoadoutOccupantToStorage(Slot, Item->ItemId);
}

bool UPlayerArmoryComponent::AssignWeaponToSlot(
    TSubclassOf<AWeaponBase> WeaponClass, EWeaponLoadoutSlot Slot) {
  if (!CanAssignWeaponToSlot(WeaponClass, Slot)) {
    return false;
  }

  FInventoryItemInstance *Item = FindWeaponItem(WeaponClass);
  if (!Item) {
    return false;
  }

  const FGuid TargetItemId = GetLoadoutItemId(Slot);
  if (TargetItemId.IsValid() && TargetItemId != Item->ItemId &&
      !RelocateLoadoutOccupantToStorage(Slot, Item->ItemId)) {
    return false;
  }

  return MoveItemToLoadoutInternal(*Item, Slot);
}

void UPlayerArmoryComponent::ClearSlot(EWeaponLoadoutSlot Slot) {
  FInventoryItemInstance *Item = FindItemById(GetLoadoutItemId(Slot));
  if (!Item) {
    return;
  }

  FInventoryGridPlacement Placement;
  if (!FindFirstFit(*Item, Placement)) {
    return;
  }

  MoveItemToGridInternal(*Item, Placement);
}

bool UPlayerArmoryComponent::SetActiveSlot(EWeaponLoadoutSlot Slot) {
  if (!IsSlotOccupied(Slot)) {
    return false;
  }

  ActiveSlot = Slot;
  if (BoundCombatComponent.IsValid()) {
    BoundCombatComponent->SetActiveLoadoutSlot(Slot);
  }

  BroadcastArmoryChanged();
  return true;
}

int32 UPlayerArmoryComponent::GetTotalGridCells() const {
  return GetStorageGridWidth() * GetStorageGridHeight();
}

void UPlayerArmoryComponent::SetStorageGridDimensions(int32 NewGridWidth,
                                                      int32 NewGridHeight) {
  const int32 SanitizedGridWidth = FMath::Max(1, NewGridWidth);
  const int32 SanitizedGridHeight = FMath::Max(1, NewGridHeight);

  if (StorageGridWidth == SanitizedGridWidth &&
      StorageGridHeight == SanitizedGridHeight) {
    return;
  }

  if (OwnedItems.Num() > 0) {
    return;
  }

  StorageGridWidth = SanitizedGridWidth;
  StorageGridHeight = SanitizedGridHeight;

  if (bHasInitializedSessionState) {
    BroadcastArmoryChanged();
  }
}

int32 UPlayerArmoryComponent::GetUsedGridCells() const {
  int32 UsedCells = 0;

  for (const FInventoryItemInstance &Item : OwnedItems) {
    if (Item.Container != EInventoryItemContainer::StorageGrid) {
      continue;
    }

    const FIntPoint Footprint =
        GetSanitizedFootprint(Item, Item.GridPlacement.bRotated);
    UsedCells += Footprint.X * Footprint.Y;
  }

  return UsedCells;
}

float UPlayerArmoryComponent::GetUsedWeight() const {
  float TotalWeight = 0.0f;

  for (const FInventoryItemInstance &Item : OwnedItems) {
    TotalWeight += FMath::Max(0.0f, Item.Weight);
  }

  return TotalWeight;
}

TArray<FInventoryItemViewData> UPlayerArmoryComponent::GetAllInventoryItems() const {
  TArray<FInventoryItemViewData> ItemViews;
  ItemViews.Reserve(OwnedItems.Num());

  for (const FInventoryItemInstance &Item : OwnedItems) {
    ItemViews.Add(BuildItemViewData(Item));
  }

  return ItemViews;
}

TArray<FInventoryItemViewData> UPlayerArmoryComponent::GetStorageGridItems() const {
  TArray<FInventoryItemViewData> ItemViews;

  for (const FInventoryItemInstance &Item : OwnedItems) {
    if (Item.Container == EInventoryItemContainer::StorageGrid) {
      ItemViews.Add(BuildItemViewData(Item));
    }
  }

  ItemViews.Sort([](const FInventoryItemViewData &Left,
                    const FInventoryItemViewData &Right) {
    return Left.GridPosition.Y == Right.GridPosition.Y
               ? Left.GridPosition.X < Right.GridPosition.X
               : Left.GridPosition.Y < Right.GridPosition.Y;
  });

  return ItemViews;
}

bool UPlayerArmoryComponent::GetItemViewData(
    FGuid ItemId, FInventoryItemViewData &OutItemViewData) const {
  const FInventoryItemInstance *Item = FindItemById(ItemId);
  if (!Item) {
    return false;
  }

  OutItemViewData = BuildItemViewData(*Item);
  return true;
}

bool UPlayerArmoryComponent::GetLoadoutItemViewData(
    EWeaponLoadoutSlot Slot, FInventoryItemViewData &OutItemViewData) const {
  const FInventoryItemInstance *Item = FindItemById(GetLoadoutItemId(Slot));
  if (!Item) {
    return false;
  }

  OutItemViewData = BuildItemViewData(*Item);
  return true;
}

FGuid UPlayerArmoryComponent::GetLoadoutItemId(EWeaponLoadoutSlot Slot) const {
  const int32 SlotIndex = ProjectWeaponLoadout::ToIndex(static_cast<uint8>(Slot));
  return LoadoutItemIds.IsValidIndex(SlotIndex) ? LoadoutItemIds[SlotIndex] : FGuid();
}

bool UPlayerArmoryComponent::CanMoveItemToGrid(FGuid ItemId, FIntPoint GridPosition,
                                               bool bRotated) const {
  const FInventoryItemInstance *Item = FindItemById(ItemId);
  if (!Item) {
    return false;
  }

  FInventoryGridPlacement Placement;
  Placement.TopLeft = GridPosition;
  Placement.bRotated = bRotated;
  return CanPlaceInGrid(*Item, Placement, ItemId);
}

bool UPlayerArmoryComponent::MoveItemToGrid(FGuid ItemId, FIntPoint GridPosition,
                                            bool bRotated) {
  FInventoryItemInstance *Item = FindItemById(ItemId);
  if (!Item) {
    return false;
  }

  FInventoryGridPlacement Placement;
  Placement.TopLeft = GridPosition;
  Placement.bRotated = bRotated;
  return MoveItemToGridInternal(*Item, Placement);
}

bool UPlayerArmoryComponent::CanMoveItemToLoadout(FGuid ItemId,
                                                  EWeaponLoadoutSlot Slot) const {
  const FInventoryItemInstance *Item = FindItemById(ItemId);
  if (!Item || !IsItemAllowedInLoadout(*Item, Slot)) {
    return false;
  }

  const FGuid ExistingItemId = GetLoadoutItemId(Slot);
  return !ExistingItemId.IsValid() || ExistingItemId == ItemId;
}

bool UPlayerArmoryComponent::MoveItemToLoadout(FGuid ItemId,
                                               EWeaponLoadoutSlot Slot) {
  FInventoryItemInstance *Item = FindItemById(ItemId);
  if (!Item) {
    return false;
  }

  if (Item->Container == EInventoryItemContainer::LoadoutSlot &&
      Item->LoadoutSlot == Slot) {
    return ActiveSlot == Slot ? true : SetActiveSlot(Slot);
  }

  if (!CanMoveItemToLoadout(ItemId, Slot)) {
    return false;
  }

  return MoveItemToLoadoutInternal(*Item, Slot);
}

bool UPlayerArmoryComponent::CanRotateItem(FGuid ItemId) const {
  const FInventoryItemInstance *Item = FindItemById(ItemId);
  return Item && Item->bCanRotate &&
         GetSanitizedFootprint(*Item, false) != GetSanitizedFootprint(*Item, true);
}

bool UPlayerArmoryComponent::IsItemInStorageGrid(FGuid ItemId) const {
  const FInventoryItemInstance *Item = FindItemById(ItemId);
  return Item && Item->Container == EInventoryItemContainer::StorageGrid;
}

bool UPlayerArmoryComponent::IsItemRotated(FGuid ItemId) const {
  const FInventoryItemInstance *Item = FindItemById(ItemId);
  return Item ? Item->GridPlacement.bRotated : false;
}

FIntPoint UPlayerArmoryComponent::GetItemFootprint(FGuid ItemId,
                                                   bool bRotated) const {
  const FInventoryItemInstance *Item = FindItemById(ItemId);
  return Item ? GetSanitizedFootprint(*Item, bRotated) : FIntPoint(1, 1);
}

void UPlayerArmoryComponent::HandlePawnCurrencyChanged(
    UCurrencyComponent *CurrencyComponent, int32 CurrentCurrency,
    int32 DeltaCurrency) {
  CachedCurrency = FMath::Max(0, CurrentCurrency);

  if (DeltaCurrency != 0) {
    BroadcastArmoryChanged();
  }
}

void UPlayerArmoryComponent::EnsureLoadoutItemIdsInitialized() {
  if (LoadoutItemIds.Num() == ProjectWeaponLoadout::SlotCount) {
    return;
  }

  LoadoutItemIds.SetNum(ProjectWeaponLoadout::SlotCount);
}

void UPlayerArmoryComponent::BroadcastArmoryChanged() {
  OnArmoryChanged.Broadcast(this);
}

void UPlayerArmoryComponent::BindCurrencyComponent(
    UCurrencyComponent *CurrencyComponent) {
  if (!CurrencyComponent) {
    return;
  }

  CurrencyComponent->OnCurrencyChanged.AddDynamic(
      this, &UPlayerArmoryComponent::HandlePawnCurrencyChanged);
  CachedCurrency = CurrencyComponent->GetCurrency();
}

void UPlayerArmoryComponent::UnbindCurrencyComponent() {
  if (UCurrencyComponent *CurrencyComponent = BoundCurrencyComponent.Get()) {
    CurrencyComponent->OnCurrencyChanged.RemoveDynamic(
        this, &UPlayerArmoryComponent::HandlePawnCurrencyChanged);
  }
}

void UPlayerArmoryComponent::SyncRuntimeWeaponStateFromBoundPawn() {
  if (!BoundCombatComponent.IsValid()) {
    BoundCombatComponent = BoundPawn.IsValid()
                               ? BoundPawn->FindComponentByClass<UCombatComponent>()
                               : nullptr;
  }

  if (!BoundCombatComponent.IsValid()) {
    return;
  }

  for (int32 SlotIndex = 0; SlotIndex < LoadoutItemIds.Num(); ++SlotIndex) {
    FInventoryItemInstance *AssignedItem = FindItemById(LoadoutItemIds[SlotIndex]);
    if (!AssignedItem) {
      continue;
    }

    if (AWeaponBase *Weapon = BoundCombatComponent->GetWeaponInSlot(
            static_cast<EWeaponLoadoutSlot>(SlotIndex))) {
      const FWeaponAmmoSaveData AmmoSaveData = Weapon->GetAmmoSaveData();
      AssignedItem->CurrentAmmoInMagazine = AmmoSaveData.AmmoInMagazine;
      AssignedItem->ReserveAmmo = AmmoSaveData.ReserveAmmo;
    }
  }
}

bool UPlayerArmoryComponent::IsWeaponAllowedInSlot(
    TSubclassOf<AWeaponBase> WeaponClass, EWeaponLoadoutSlot Slot) const {
  if (!WeaponClass) {
    return false;
  }

  const AWeaponBase *WeaponCDO = WeaponClass->GetDefaultObject<AWeaponBase>();
  if (!WeaponCDO) {
    return false;
  }

  switch (WeaponCDO->GetEquipGroup()) {
  case EWeaponEquipGroup::Primary:
    return Slot == EWeaponLoadoutSlot::Slot1Primary ||
           Slot == EWeaponLoadoutSlot::Slot2Secondary;

  case EWeaponEquipGroup::Auxiliary:
    return Slot == EWeaponLoadoutSlot::Slot3Auxiliary;

  default:
    return false;
  }
}

bool UPlayerArmoryComponent::IsItemAllowedInLoadout(
    const FInventoryItemInstance &Item, EWeaponLoadoutSlot Slot) const {
  if (Item.ItemKind != EInventoryItemKind::Weapon || !Item.WeaponClass) {
    return false;
  }

  return IsWeaponAllowedInSlot(Item.WeaponClass, Slot);
}

EWeaponLoadoutSlot UPlayerArmoryComponent::FindFallbackActiveSlot() const {
  static const EWeaponLoadoutSlot PrioritySlots[] = {
      EWeaponLoadoutSlot::Slot1Primary, EWeaponLoadoutSlot::Slot2Secondary,
      EWeaponLoadoutSlot::Slot3Auxiliary};

  for (const EWeaponLoadoutSlot CandidateSlot : PrioritySlots) {
    if (IsSlotOccupied(CandidateSlot)) {
      return CandidateSlot;
    }
  }

  return EWeaponLoadoutSlot::Slot1Primary;
}

FInventoryItemInstance *
UPlayerArmoryComponent::FindItemById(const FGuid &ItemId) {
  if (!ItemId.IsValid()) {
    return nullptr;
  }

  for (FInventoryItemInstance &Item : OwnedItems) {
    if (Item.ItemId == ItemId) {
      return &Item;
    }
  }

  return nullptr;
}

const FInventoryItemInstance *
UPlayerArmoryComponent::FindItemById(const FGuid &ItemId) const {
  if (!ItemId.IsValid()) {
    return nullptr;
  }

  for (const FInventoryItemInstance &Item : OwnedItems) {
    if (Item.ItemId == ItemId) {
      return &Item;
    }
  }

  return nullptr;
}

FInventoryItemInstance *UPlayerArmoryComponent::FindWeaponItem(
    TSubclassOf<AWeaponBase> WeaponClass) {
  if (!WeaponClass) {
    return nullptr;
  }

  for (FInventoryItemInstance &Item : OwnedItems) {
    if (Item.ItemKind == EInventoryItemKind::Weapon &&
        Item.WeaponClass == WeaponClass) {
      return &Item;
    }
  }

  return nullptr;
}

const FInventoryItemInstance *UPlayerArmoryComponent::FindWeaponItem(
    TSubclassOf<AWeaponBase> WeaponClass) const {
  if (!WeaponClass) {
    return nullptr;
  }

  for (const FInventoryItemInstance &Item : OwnedItems) {
    if (Item.ItemKind == EInventoryItemKind::Weapon &&
        Item.WeaponClass == WeaponClass) {
      return &Item;
    }
  }

  return nullptr;
}

FIntPoint UPlayerArmoryComponent::GetSanitizedFootprint(
    const FInventoryItemInstance &Item, bool bRotated) const {
  FIntPoint Footprint(FMath::Max(1, Item.Footprint.X),
                      FMath::Max(1, Item.Footprint.Y));
  if (bRotated) {
    Swap(Footprint.X, Footprint.Y);
  }

  return Footprint;
}

FInventoryItemViewData
UPlayerArmoryComponent::BuildItemViewData(const FInventoryItemInstance &Item) const {
  FInventoryItemViewData ViewData;
  ViewData.ItemId = Item.ItemId;
  ViewData.ItemKind = Item.ItemKind;
  ViewData.Container = Item.Container;
  ViewData.WeaponClass = Item.WeaponClass;
  ViewData.DisplayName = Item.DisplayName;
  ViewData.Icon = Item.Icon;
  ViewData.Weight = FMath::Max(0.0f, Item.Weight);
  ViewData.BaseFootprint = GetSanitizedFootprint(Item, false);
  ViewData.bCanRotate = Item.bCanRotate;
  ViewData.bIsRotated = Item.GridPlacement.bRotated;
  ViewData.OccupiedFootprint =
      GetSanitizedFootprint(Item, Item.GridPlacement.bRotated);
  ViewData.GridPosition = Item.Container == EInventoryItemContainer::StorageGrid
                              ? Item.GridPlacement.TopLeft
                              : FIntPoint(-1, -1);
  ViewData.LoadoutSlot = Item.LoadoutSlot;
  return ViewData;
}

bool UPlayerArmoryComponent::BuildWeaponItemInstance(
    TSubclassOf<AWeaponBase> WeaponClass, FInventoryItemInstance &OutItem) const {
  if (!WeaponClass) {
    return false;
  }

  const AWeaponBase *WeaponCDO = WeaponClass->GetDefaultObject<AWeaponBase>();
  if (!WeaponCDO) {
    return false;
  }

  OutItem.ItemId = FGuid::NewGuid();
  OutItem.ItemKind = EInventoryItemKind::Weapon;
  OutItem.Container = EInventoryItemContainer::StorageGrid;
  OutItem.WeaponClass = WeaponClass;
  OutItem.DisplayName = WeaponCDO->GetWeaponDisplayName();
  OutItem.Icon = WeaponCDO->GetWeaponIcon();
  OutItem.Weight = FMath::Max(0.0f, WeaponCDO->GetInventoryWeight());
  OutItem.Footprint = WeaponCDO->GetInventoryFootprint();
  OutItem.Footprint.X = FMath::Max(1, OutItem.Footprint.X);
  OutItem.Footprint.Y = FMath::Max(1, OutItem.Footprint.Y);
  OutItem.bCanRotate = WeaponCDO->CanRotateInInventory();
  OutItem.GridPlacement = FInventoryGridPlacement();
  OutItem.LoadoutSlot = EWeaponLoadoutSlot::Slot1Primary;
  OutItem.CurrentAmmoInMagazine = WeaponCDO->GetAmmoInMagazine();
  OutItem.ReserveAmmo = WeaponCDO->GetReserveAmmo();
  return OutItem.ItemId.IsValid();
}

bool UPlayerArmoryComponent::BuildInventoryItemFromSaveData(
    const FArmoryItemSaveData &ItemSaveData, FInventoryItemInstance &OutItem) const {
  UClass *LoadedWeaponClass = ItemSaveData.WeaponClass.Get();
  TSubclassOf<AWeaponBase> WeaponClass = LoadedWeaponClass;
  if (!WeaponClass) {
    UE_LOG(LogProject, Warning,
           TEXT("PlayerArmoryComponent: failed to resolve weapon class for saved item '%s'"),
           *ItemSaveData.ItemId.ToString());
    return false;
  }

  if (!BuildWeaponItemInstance(WeaponClass, OutItem)) {
    return false;
  }

  OutItem.ItemId = ItemSaveData.ItemId.IsValid() ? ItemSaveData.ItemId : FGuid::NewGuid();
  OutItem.Container = ItemSaveData.Container;
  OutItem.GridPlacement = ItemSaveData.GridPlacement;
  OutItem.LoadoutSlot = ItemSaveData.LoadoutSlot;
  OutItem.CurrentAmmoInMagazine = FMath::Max(0, ItemSaveData.AmmoData.AmmoInMagazine);
  OutItem.ReserveAmmo = FMath::Max(0, ItemSaveData.AmmoData.ReserveAmmo);
  return OutItem.ItemId.IsValid();
}

bool UPlayerArmoryComponent::CanPlaceInGrid(
    const FInventoryItemInstance &Item, const FInventoryGridPlacement &Placement,
    const FGuid &IgnoredItemId, const FGuid &AdditionalIgnoredItemId) const {
  const int32 GridWidth = GetStorageGridWidth();
  const int32 GridHeight = GetStorageGridHeight();
  const FIntPoint DefaultFootprint = GetSanitizedFootprint(Item, false);
  const FIntPoint RotatedFootprint = GetSanitizedFootprint(Item, true);
  if (Placement.bRotated && DefaultFootprint != RotatedFootprint && !Item.bCanRotate) {
    return false;
  }

  const FIntPoint Footprint = Placement.bRotated ? RotatedFootprint : DefaultFootprint;

  if (Placement.TopLeft.X < 0 || Placement.TopLeft.Y < 0 ||
      Footprint.X <= 0 || Footprint.Y <= 0 ||
      Placement.TopLeft.X + Footprint.X > GridWidth ||
      Placement.TopLeft.Y + Footprint.Y > GridHeight) {
    return false;
  }

  TArray<bool> OccupiedMask;
  BuildOccupiedMask(OccupiedMask, IgnoredItemId, AdditionalIgnoredItemId);

  for (int32 Y = 0; Y < Footprint.Y; ++Y) {
    for (int32 X = 0; X < Footprint.X; ++X) {
      const FIntPoint CurrentCell(Placement.TopLeft.X + X, Placement.TopLeft.Y + Y);
      const int32 CellIndex = MakeGridCellIndex(CurrentCell, GridWidth);
      if (!OccupiedMask.IsValidIndex(CellIndex) || OccupiedMask[CellIndex]) {
        return false;
      }
    }
  }

  return true;
}

bool UPlayerArmoryComponent::FindFirstFit(
    const FInventoryItemInstance &Item, FInventoryGridPlacement &OutPlacement,
    const FGuid &AdditionalIgnoredItemId) const {
  TArray<bool> RotationStates;
  RotationStates.Add(Item.GridPlacement.bRotated);
  if (Item.bCanRotate &&
      GetSanitizedFootprint(Item, false) != GetSanitizedFootprint(Item, true)) {
    RotationStates.Add(!Item.GridPlacement.bRotated);
  }

  for (bool bRotated : RotationStates) {
    const FIntPoint Footprint = GetSanitizedFootprint(Item, bRotated);
    for (int32 Y = 0; Y <= GetStorageGridHeight() - Footprint.Y; ++Y) {
      for (int32 X = 0; X <= GetStorageGridWidth() - Footprint.X; ++X) {
        FInventoryGridPlacement Placement;
        Placement.TopLeft = FIntPoint(X, Y);
        Placement.bRotated = bRotated;
        if (CanPlaceInGrid(Item, Placement, Item.ItemId, AdditionalIgnoredItemId)) {
          OutPlacement = Placement;
          return true;
        }
      }
    }
  }

  return false;
}

void UPlayerArmoryComponent::BuildOccupiedMask(
    TArray<bool> &OutMask, const FGuid &IgnoredItemId,
    const FGuid &AdditionalIgnoredItemId) const {
  const int32 GridWidth = GetStorageGridWidth();
  const int32 GridHeight = GetStorageGridHeight();
  OutMask.Init(false, GridWidth * GridHeight);

  for (const FInventoryItemInstance &Item : OwnedItems) {
    if (Item.Container != EInventoryItemContainer::StorageGrid ||
        Item.ItemId == IgnoredItemId || Item.ItemId == AdditionalIgnoredItemId) {
      continue;
    }

    const FIntPoint Footprint =
        GetSanitizedFootprint(Item, Item.GridPlacement.bRotated);
    for (int32 Y = 0; Y < Footprint.Y; ++Y) {
      for (int32 X = 0; X < Footprint.X; ++X) {
        const FIntPoint Cell(Item.GridPlacement.TopLeft.X + X,
                             Item.GridPlacement.TopLeft.Y + Y);
        if (Cell.X < 0 || Cell.Y < 0 || Cell.X >= GridWidth || Cell.Y >= GridHeight) {
          continue;
        }

        const int32 CellIndex = MakeGridCellIndex(Cell, GridWidth);
        if (OutMask.IsValidIndex(CellIndex)) {
          OutMask[CellIndex] = true;
        }
      }
    }
  }
}

bool UPlayerArmoryComponent::MoveItemToGridInternal(
    FInventoryItemInstance &Item, const FInventoryGridPlacement &Placement,
    const FGuid &AdditionalIgnoredItemId) {
  if (!CanPlaceInGrid(Item, Placement, Item.ItemId, AdditionalIgnoredItemId)) {
    return false;
  }

  const bool bWasInLoadout = Item.Container == EInventoryItemContainer::LoadoutSlot;
  if (bWasInLoadout) {
    SyncRuntimeWeaponStateFromBoundPawn();
  }

  if (bWasInLoadout) {
    const int32 PreviousSlotIndex =
        ProjectWeaponLoadout::ToIndex(static_cast<uint8>(Item.LoadoutSlot));
    if (LoadoutItemIds.IsValidIndex(PreviousSlotIndex) &&
        LoadoutItemIds[PreviousSlotIndex] == Item.ItemId) {
      LoadoutItemIds[PreviousSlotIndex] = FGuid();
    }
  }

  Item.Container = EInventoryItemContainer::StorageGrid;
  Item.GridPlacement = Placement;

  if (bWasInLoadout) {
    if (BoundPawn.IsValid()) {
      ApplyStateToBoundPawn();
    } else {
      BroadcastArmoryChanged();
    }
  } else {
    BroadcastArmoryChanged();
  }

  return true;
}

bool UPlayerArmoryComponent::MoveItemToLoadoutInternal(
    FInventoryItemInstance &Item, EWeaponLoadoutSlot Slot) {
  if (!IsItemAllowedInLoadout(Item, Slot)) {
    return false;
  }

  SyncRuntimeWeaponStateFromBoundPawn();

  const int32 TargetSlotIndex = ProjectWeaponLoadout::ToIndex(static_cast<uint8>(Slot));
  if (!LoadoutItemIds.IsValidIndex(TargetSlotIndex)) {
    return false;
  }

  const FGuid ExistingItemId = LoadoutItemIds[TargetSlotIndex];
  if (ExistingItemId.IsValid() && ExistingItemId != Item.ItemId) {
    return false;
  }

  if (Item.Container == EInventoryItemContainer::LoadoutSlot) {
    const int32 PreviousSlotIndex =
        ProjectWeaponLoadout::ToIndex(static_cast<uint8>(Item.LoadoutSlot));
    if (LoadoutItemIds.IsValidIndex(PreviousSlotIndex) &&
        LoadoutItemIds[PreviousSlotIndex] == Item.ItemId) {
      LoadoutItemIds[PreviousSlotIndex] = FGuid();
    }
  }

  const bool bHadActiveSlotBeforeMove = IsSlotOccupied(ActiveSlot);
  LoadoutItemIds[TargetSlotIndex] = Item.ItemId;
  Item.Container = EInventoryItemContainer::LoadoutSlot;
  Item.LoadoutSlot = Slot;

  if (!bHadActiveSlotBeforeMove || ActiveSlot == Slot) {
    ActiveSlot = Slot;
  }

  if (BoundPawn.IsValid()) {
    ApplyStateToBoundPawn();
  } else {
    BroadcastArmoryChanged();
  }
  return true;
}

bool UPlayerArmoryComponent::RotatePlacement(
    const FInventoryItemInstance &Item, FInventoryGridPlacement &Placement) const {
  if (!Item.bCanRotate) {
    return false;
  }

  const FIntPoint DefaultFootprint = GetSanitizedFootprint(Item, false);
  const FIntPoint RotatedFootprint = GetSanitizedFootprint(Item, true);
  if (DefaultFootprint == RotatedFootprint) {
    return false;
  }

  Placement.bRotated = !Placement.bRotated;
  return true;
}

bool UPlayerArmoryComponent::CanRelocateLoadoutOccupantToStorage(
    EWeaponLoadoutSlot Slot, const FGuid &IgnoredItemId) const {
  const FInventoryItemInstance *Occupant = FindItemById(GetLoadoutItemId(Slot));
  if (!Occupant) {
    return true;
  }

  FInventoryGridPlacement Placement;
  return FindFirstFit(*Occupant, Placement, IgnoredItemId);
}

bool UPlayerArmoryComponent::RelocateLoadoutOccupantToStorage(
    EWeaponLoadoutSlot Slot, const FGuid &IgnoredItemId) {
  FInventoryItemInstance *Occupant = FindItemById(GetLoadoutItemId(Slot));
  if (!Occupant) {
    return true;
  }

  FInventoryGridPlacement Placement;
  if (!FindFirstFit(*Occupant, Placement, IgnoredItemId)) {
    return false;
  }

  return MoveItemToGridInternal(*Occupant, Placement, IgnoredItemId);
}
