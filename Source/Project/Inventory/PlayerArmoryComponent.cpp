// Copyright Epic Games, Inc. All Rights Reserved.

#include "Inventory/PlayerArmoryComponent.h"
#include "Character/CurrencyComponent.h"
#include "Combat/CombatComponent.h"
#include "Combat/WeaponBase.h"
#include "GameFramework/Pawn.h"

namespace {
int32 GetSlotIndex(EWeaponLoadoutSlot Slot) {
  return static_cast<int32>(Slot);
}
} // namespace

UPlayerArmoryComponent::UPlayerArmoryComponent() {
  PrimaryComponentTick.bCanEverTick = false;
  EnsureSlotAssignmentsInitialized();
}

void UPlayerArmoryComponent::BeginPlay() {
  Super::BeginPlay();
  EnsureSlotAssignmentsInitialized();
}

void UPlayerArmoryComponent::InitializeEmptySession(int32 InitialCurrency) {
  EnsureSlotAssignmentsInitialized();

  bHasInitializedSessionState = true;
  CachedCurrency = FMath::Max(0, InitialCurrency);
  OwnedWeaponClasses.Reset();
  for (int32 SlotIndex = 0; SlotIndex < SlotAssignments.Num(); ++SlotIndex) {
    SlotAssignments[SlotIndex] = nullptr;
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
  for (int32 SlotIndex = 0; SlotIndex < SlotAssignments.Num(); ++SlotIndex) {
    const TSubclassOf<AWeaponBase> AssignedClass = SlotAssignments[SlotIndex];
    if (!AssignedClass) {
      continue;
    }

    BoundCombatComponent->SetWeaponForSlot(
        static_cast<EWeaponLoadoutSlot>(SlotIndex), AssignedClass);
  }

  const EWeaponLoadoutSlot DesiredActiveSlot =
      IsSlotOccupied(ActiveSlot) ? ActiveSlot : FindFallbackActiveSlot();
  if (IsSlotOccupied(DesiredActiveSlot)) {
    ActiveSlot = DesiredActiveSlot;
    BoundCombatComponent->SetActiveLoadoutSlot(DesiredActiveSlot);
  }

  BroadcastArmoryChanged();
}

bool UPlayerArmoryComponent::CanPurchaseWeapon(TSubclassOf<AWeaponBase> WeaponClass,
                                               int32 Price) const {
  if (!bHasInitializedSessionState || !WeaponClass || Price < 0) {
    return false;
  }

  return !HasOwnedWeapon(WeaponClass) && CachedCurrency >= Price;
}

bool UPlayerArmoryComponent::PurchaseWeapon(TSubclassOf<AWeaponBase> WeaponClass,
                                            int32 Price) {
  if (!CanPurchaseWeapon(WeaponClass, Price)) {
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

  OwnedWeaponClasses.Add(WeaponClass);
  BroadcastArmoryChanged();
  return true;
}

bool UPlayerArmoryComponent::HasOwnedWeapon(
    TSubclassOf<AWeaponBase> WeaponClass) const {
  if (!WeaponClass) {
    return false;
  }

  return OwnedWeaponClasses.Contains(WeaponClass);
}

TSubclassOf<AWeaponBase> UPlayerArmoryComponent::GetAssignedWeaponForSlot(
    EWeaponLoadoutSlot Slot) const {
  const int32 SlotIndex = GetSlotIndex(Slot);
  return SlotAssignments.IsValidIndex(SlotIndex) ? SlotAssignments[SlotIndex]
                                                 : nullptr;
}

bool UPlayerArmoryComponent::IsSlotOccupied(EWeaponLoadoutSlot Slot) const {
  return GetAssignedWeaponForSlot(Slot) != nullptr;
}

bool UPlayerArmoryComponent::CanAssignWeaponToSlot(
    TSubclassOf<AWeaponBase> WeaponClass, EWeaponLoadoutSlot Slot) const {
  if (!bHasInitializedSessionState || !HasOwnedWeapon(WeaponClass)) {
    return false;
  }

  return IsWeaponAllowedInSlot(WeaponClass, Slot);
}

bool UPlayerArmoryComponent::AssignWeaponToSlot(
    TSubclassOf<AWeaponBase> WeaponClass, EWeaponLoadoutSlot Slot) {
  if (!CanAssignWeaponToSlot(WeaponClass, Slot)) {
    return false;
  }

  EnsureSlotAssignmentsInitialized();

  const int32 TargetSlotIndex = GetSlotIndex(Slot);
  for (int32 SlotIndex = 0; SlotIndex < SlotAssignments.Num(); ++SlotIndex) {
    if (SlotAssignments[SlotIndex] == WeaponClass && SlotIndex != TargetSlotIndex) {
      SlotAssignments[SlotIndex] = nullptr;
      if (BoundCombatComponent.IsValid()) {
        BoundCombatComponent->ClearWeaponSlot(
            static_cast<EWeaponLoadoutSlot>(SlotIndex));
      }
    }
  }

  SlotAssignments[TargetSlotIndex] = WeaponClass;

  if (BoundCombatComponent.IsValid()) {
    BoundCombatComponent->SetWeaponForSlot(Slot, WeaponClass);
  }

  if (!IsSlotOccupied(ActiveSlot)) {
    SetActiveSlot(Slot);
  } else if (ActiveSlot == Slot && BoundCombatComponent.IsValid()) {
    BoundCombatComponent->SetActiveLoadoutSlot(Slot);
  }

  BroadcastArmoryChanged();
  return true;
}

void UPlayerArmoryComponent::ClearSlot(EWeaponLoadoutSlot Slot) {
  EnsureSlotAssignmentsInitialized();

  const int32 SlotIndex = GetSlotIndex(Slot);
  if (!SlotAssignments.IsValidIndex(SlotIndex) || !SlotAssignments[SlotIndex]) {
    return;
  }

  SlotAssignments[SlotIndex] = nullptr;

  if (BoundCombatComponent.IsValid()) {
    BoundCombatComponent->ClearWeaponSlot(Slot);
  }

  if (ActiveSlot == Slot) {
    const EWeaponLoadoutSlot FallbackSlot = FindFallbackActiveSlot();
    if (IsSlotOccupied(FallbackSlot)) {
      SetActiveSlot(FallbackSlot);
    }
  }

  BroadcastArmoryChanged();
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

void UPlayerArmoryComponent::HandlePawnCurrencyChanged(
    UCurrencyComponent *CurrencyComponent, int32 CurrentCurrency, int32 DeltaCurrency) {
  CachedCurrency = FMath::Max(0, CurrentCurrency);

  if (DeltaCurrency != 0) {
    BroadcastArmoryChanged();
  }
}

void UPlayerArmoryComponent::EnsureSlotAssignmentsInitialized() {
  if (SlotAssignments.Num() == ProjectWeaponLoadout::SlotCount) {
    return;
  }

  SlotAssignments.SetNumZeroed(ProjectWeaponLoadout::SlotCount);
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
