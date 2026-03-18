// Copyright Epic Games, Inc. All Rights Reserved.

#include "UI/Armory/PlayerInventoryWidgetBase.h"
#include "Blueprint/DragDropOperation.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/TextBlock.h"
#include "Controllers/MainPlayerController.h"
#include "InputCoreTypes.h"
#include "Inventory/PlayerArmoryComponent.h"
#include "UI/Armory/InventoryItemWidgetBase.h"
#include "UI/Armory/LoadoutSlotWidgetBase.h"

#define LOCTEXT_NAMESPACE "PlayerInventoryWidgetBase"

void UPlayerInventoryWidgetBase::NativeConstruct() {
  Super::NativeConstruct();

  InitializeNamedWidgets();
  SetKeyboardFocus();
}

void UPlayerInventoryWidgetBase::NativeDestruct() {
  if (CachedCloseButton) {
    CachedCloseButton->OnClicked.RemoveDynamic(
        this, &UPlayerInventoryWidgetBase::HandleCloseButtonClicked);
  }

  Super::NativeDestruct();
}

FReply UPlayerInventoryWidgetBase::NativeOnPreviewKeyDown(
    const FGeometry &InGeometry, const FKeyEvent &InKeyEvent) {
  if (InKeyEvent.GetKey() == EKeys::R && ToggleDraggedItemRotation()) {
    return FReply::Handled();
  }

  return Super::NativeOnPreviewKeyDown(InGeometry, InKeyEvent);
}

bool UPlayerInventoryWidgetBase::NativeOnDragOver(
    const FGeometry &InGeometry, const FDragDropEvent &InDragDropEvent,
    UDragDropOperation *InOperation) {
  InitializeNamedWidgets();

  if (!HasDraggedItem() || !CachedGridBoundsWidget) {
    HideGridPreview();
    return Super::NativeOnDragOver(InGeometry, InDragDropEvent, InOperation);
  }

  FIntPoint GridCell;
  const FVector2D LocalPosition = CachedGridBoundsWidget->GetCachedGeometry().AbsoluteToLocal(
      InDragDropEvent.GetScreenSpacePosition());
  if (!TryGetGridCellFromLocalPosition(LocalPosition, DefaultCellSize, GridCell)) {
    HideGridPreview();
    return false;
  }

  const bool bCanMove = CanMoveDraggedItemToGrid(GridCell);
  ShowGridPreview(GridCell, bCanMove);
  return bCanMove;
}

void UPlayerInventoryWidgetBase::NativeOnDragLeave(
    const FDragDropEvent &InDragDropEvent, UDragDropOperation *InOperation) {
  HideGridPreview();
  Super::NativeOnDragLeave(InDragDropEvent, InOperation);
}

bool UPlayerInventoryWidgetBase::NativeOnDrop(
    const FGeometry &InGeometry, const FDragDropEvent &InDragDropEvent,
    UDragDropOperation *InOperation) {
  InitializeNamedWidgets();

  if (!HasDraggedItem() || !CachedGridBoundsWidget) {
    HideGridPreview();
    return false;
  }

  FIntPoint GridCell;
  const FVector2D LocalPosition = CachedGridBoundsWidget->GetCachedGeometry().AbsoluteToLocal(
      InDragDropEvent.GetScreenSpacePosition());
  if (!TryGetGridCellFromLocalPosition(LocalPosition, DefaultCellSize, GridCell)) {
    HideGridPreview();
    return false;
  }

  const bool bMoved = MoveDraggedItemToGrid(GridCell);
  HideGridPreview();
  return bMoved;
}

TArray<TSubclassOf<AWeaponBase>> UPlayerInventoryWidgetBase::GetOwnedWeapons() const {
  if (const UPlayerArmoryComponent *ArmoryComponent = GetArmoryComponent()) {
    return ArmoryComponent->GetOwnedWeapons();
  }

  return {};
}

TSubclassOf<AWeaponBase> UPlayerInventoryWidgetBase::GetAssignedWeaponForSlot(
    EWeaponLoadoutSlot LoadoutSlot) const {
  if (const UPlayerArmoryComponent *ArmoryComponent = GetArmoryComponent()) {
    return ArmoryComponent->GetAssignedWeaponForSlot(LoadoutSlot);
  }

  return nullptr;
}

bool UPlayerInventoryWidgetBase::IsSlotOccupied(
    EWeaponLoadoutSlot LoadoutSlot) const {
  if (const UPlayerArmoryComponent *ArmoryComponent = GetArmoryComponent()) {
    return ArmoryComponent->IsSlotOccupied(LoadoutSlot);
  }

  return false;
}

bool UPlayerInventoryWidgetBase::CanAssignWeaponToSlot(
    TSubclassOf<AWeaponBase> WeaponClass,
    EWeaponLoadoutSlot LoadoutSlot) const {
  if (const UPlayerArmoryComponent *ArmoryComponent = GetArmoryComponent()) {
    return ArmoryComponent->CanAssignWeaponToSlot(WeaponClass, LoadoutSlot);
  }

  return false;
}

bool UPlayerInventoryWidgetBase::AssignWeaponToSlot(
    TSubclassOf<AWeaponBase> WeaponClass,
    EWeaponLoadoutSlot LoadoutSlot) {
  if (UPlayerArmoryComponent *ArmoryComponent = GetArmoryComponent()) {
    return ArmoryComponent->AssignWeaponToSlot(WeaponClass, LoadoutSlot);
  }

  return false;
}

void UPlayerInventoryWidgetBase::ClearSlot(EWeaponLoadoutSlot LoadoutSlot) {
  if (UPlayerArmoryComponent *ArmoryComponent = GetArmoryComponent()) {
    ArmoryComponent->ClearSlot(LoadoutSlot);
  }
}

bool UPlayerInventoryWidgetBase::SetActiveSlot(EWeaponLoadoutSlot LoadoutSlot) {
  if (UPlayerArmoryComponent *ArmoryComponent = GetArmoryComponent()) {
    return ArmoryComponent->SetActiveSlot(LoadoutSlot);
  }

  return false;
}

EWeaponLoadoutSlot UPlayerInventoryWidgetBase::GetActiveSlot() const {
  if (const UPlayerArmoryComponent *ArmoryComponent = GetArmoryComponent()) {
    return ArmoryComponent->GetActiveSlot();
  }

  return EWeaponLoadoutSlot::Slot1Primary;
}

int32 UPlayerInventoryWidgetBase::GetStorageGridWidth() const {
  if (const UPlayerArmoryComponent *ArmoryComponent = GetArmoryComponent()) {
    return ArmoryComponent->GetStorageGridWidth();
  }

  return 0;
}

int32 UPlayerInventoryWidgetBase::GetStorageGridHeight() const {
  if (const UPlayerArmoryComponent *ArmoryComponent = GetArmoryComponent()) {
    return ArmoryComponent->GetStorageGridHeight();
  }

  return 0;
}

int32 UPlayerInventoryWidgetBase::GetTotalGridCells() const {
  if (const UPlayerArmoryComponent *ArmoryComponent = GetArmoryComponent()) {
    return ArmoryComponent->GetTotalGridCells();
  }

  return 0;
}

int32 UPlayerInventoryWidgetBase::GetUsedGridCells() const {
  if (const UPlayerArmoryComponent *ArmoryComponent = GetArmoryComponent()) {
    return ArmoryComponent->GetUsedGridCells();
  }

  return 0;
}

float UPlayerInventoryWidgetBase::GetUsedWeight() const {
  if (const UPlayerArmoryComponent *ArmoryComponent = GetArmoryComponent()) {
    return ArmoryComponent->GetUsedWeight();
  }

  return 0.0f;
}

TArray<FInventoryItemViewData> UPlayerInventoryWidgetBase::GetAllInventoryItems() const {
  if (const UPlayerArmoryComponent *ArmoryComponent = GetArmoryComponent()) {
    return ArmoryComponent->GetAllInventoryItems();
  }

  return {};
}

TArray<FInventoryItemViewData> UPlayerInventoryWidgetBase::GetStorageGridItems() const {
  if (const UPlayerArmoryComponent *ArmoryComponent = GetArmoryComponent()) {
    return ArmoryComponent->GetStorageGridItems();
  }

  return {};
}

FVector2D
UPlayerInventoryWidgetBase::GetStorageGridPixelSize(float CellSize) const {
  const float SanitizedCellSize = FMath::Max(0.0f, CellSize);
  return FVector2D(GetStorageGridWidth() * SanitizedCellSize,
                   GetStorageGridHeight() * SanitizedCellSize);
}

FVector2D UPlayerInventoryWidgetBase::GetGridCellCanvasPosition(
    FIntPoint GridCell, float CellSize) const {
  const float SanitizedCellSize = FMath::Max(0.0f, CellSize);
  return FVector2D(GridCell.X * SanitizedCellSize, GridCell.Y * SanitizedCellSize);
}

FVector2D UPlayerInventoryWidgetBase::GetInventoryItemCanvasPosition(
    const FInventoryItemViewData &ItemViewData, float CellSize) const {
  return GetGridCellCanvasPosition(ItemViewData.GridPosition, CellSize);
}

FVector2D UPlayerInventoryWidgetBase::GetInventoryItemCanvasSize(
    const FInventoryItemViewData &ItemViewData, float CellSize) const {
  const float SanitizedCellSize = FMath::Max(0.0f, CellSize);
  const int32 Width = FMath::Max(1, ItemViewData.OccupiedFootprint.X);
  const int32 Height = FMath::Max(1, ItemViewData.OccupiedFootprint.Y);
  return FVector2D(Width * SanitizedCellSize, Height * SanitizedCellSize);
}

FVector2D
UPlayerInventoryWidgetBase::GetDraggedItemCanvasSize(float CellSize) const {
  FInventoryItemViewData DraggedItemViewData;
  if (GetItemViewData(DraggedItemId, DraggedItemViewData)) {
    DraggedItemViewData.OccupiedFootprint = GetDraggedItemFootprint();
    DraggedItemViewData.bIsRotated = bDraggedItemRotated;
    return GetInventoryItemCanvasSize(DraggedItemViewData, CellSize);
  }

  const FIntPoint DraggedFootprint = GetDraggedItemFootprint();
  const float SanitizedCellSize = FMath::Max(0.0f, CellSize);
  return FVector2D(FMath::Max(1, DraggedFootprint.X) * SanitizedCellSize,
                   FMath::Max(1, DraggedFootprint.Y) * SanitizedCellSize);
}

FText UPlayerInventoryWidgetBase::GetInventoryItemFootprintText(
    const FInventoryItemViewData &ItemViewData) const {
  return FText::Format(
      NSLOCTEXT("PlayerInventoryWidgetBase", "InventoryItemFootprintText",
                "{0}x{1}"),
      FText::AsNumber(FMath::Max(1, ItemViewData.OccupiedFootprint.X)),
      FText::AsNumber(FMath::Max(1, ItemViewData.OccupiedFootprint.Y)));
}

FText UPlayerInventoryWidgetBase::GetInventoryItemGridPositionText(
    const FInventoryItemViewData &ItemViewData) const {
  return FText::Format(
      NSLOCTEXT("PlayerInventoryWidgetBase", "InventoryItemGridPositionText",
                "{0}, {1}"),
      FText::AsNumber(ItemViewData.GridPosition.X),
      FText::AsNumber(ItemViewData.GridPosition.Y));
}

bool UPlayerInventoryWidgetBase::TryGetGridCellFromLocalPosition(
    FVector2D LocalPosition, float CellSize, FIntPoint &OutGridCell) const {
  OutGridCell = FIntPoint(-1, -1);

  if (CellSize <= KINDA_SMALL_NUMBER) {
    return false;
  }

  const FVector2D GridPixelSize = GetStorageGridPixelSize(CellSize);
  if (LocalPosition.X < 0.0f || LocalPosition.Y < 0.0f ||
      LocalPosition.X >= GridPixelSize.X || LocalPosition.Y >= GridPixelSize.Y) {
    return false;
  }

  OutGridCell = FIntPoint(FMath::FloorToInt(LocalPosition.X / CellSize),
                          FMath::FloorToInt(LocalPosition.Y / CellSize));
  return true;
}

bool UPlayerInventoryWidgetBase::GetItemViewData(
    FGuid ItemId, FInventoryItemViewData &OutItemViewData) const {
  if (const UPlayerArmoryComponent *ArmoryComponent = GetArmoryComponent()) {
    return ArmoryComponent->GetItemViewData(ItemId, OutItemViewData);
  }

  return false;
}

bool UPlayerInventoryWidgetBase::GetLoadoutItemViewData(
    EWeaponLoadoutSlot LoadoutSlot, FInventoryItemViewData &OutItemViewData) const {
  if (const UPlayerArmoryComponent *ArmoryComponent = GetArmoryComponent()) {
    return ArmoryComponent->GetLoadoutItemViewData(LoadoutSlot, OutItemViewData);
  }

  return false;
}

FGuid UPlayerInventoryWidgetBase::GetLoadoutItemId(
    EWeaponLoadoutSlot LoadoutSlot) const {
  if (const UPlayerArmoryComponent *ArmoryComponent = GetArmoryComponent()) {
    return ArmoryComponent->GetLoadoutItemId(LoadoutSlot);
  }

  return FGuid();
}

TArray<FGuid> UPlayerInventoryWidgetBase::GetLoadoutItemIds() const {
  if (const UPlayerArmoryComponent *ArmoryComponent = GetArmoryComponent()) {
    return ArmoryComponent->GetLoadoutItemIds();
  }

  return {};
}

bool UPlayerInventoryWidgetBase::CanMoveItemToGrid(
    FGuid ItemId, FIntPoint GridPosition, bool bRotated) const {
  if (const UPlayerArmoryComponent *ArmoryComponent = GetArmoryComponent()) {
    return ArmoryComponent->CanMoveItemToGrid(ItemId, GridPosition, bRotated);
  }

  return false;
}

bool UPlayerInventoryWidgetBase::MoveItemToGrid(FGuid ItemId, FIntPoint GridPosition,
                                                bool bRotated) {
  if (UPlayerArmoryComponent *ArmoryComponent = GetArmoryComponent()) {
    return ArmoryComponent->MoveItemToGrid(ItemId, GridPosition, bRotated);
  }

  return false;
}

bool UPlayerInventoryWidgetBase::CanMoveItemToLoadout(
    FGuid ItemId, EWeaponLoadoutSlot LoadoutSlot) const {
  if (const UPlayerArmoryComponent *ArmoryComponent = GetArmoryComponent()) {
    return ArmoryComponent->CanMoveItemToLoadout(ItemId, LoadoutSlot);
  }

  return false;
}

bool UPlayerInventoryWidgetBase::MoveItemToLoadout(
    FGuid ItemId, EWeaponLoadoutSlot LoadoutSlot) {
  if (UPlayerArmoryComponent *ArmoryComponent = GetArmoryComponent()) {
    return ArmoryComponent->MoveItemToLoadout(ItemId, LoadoutSlot);
  }

  return false;
}

bool UPlayerInventoryWidgetBase::CanRotateItem(FGuid ItemId) const {
  if (const UPlayerArmoryComponent *ArmoryComponent = GetArmoryComponent()) {
    return ArmoryComponent->CanRotateItem(ItemId);
  }

  return false;
}

bool UPlayerInventoryWidgetBase::IsItemInStorageGrid(FGuid ItemId) const {
  if (const UPlayerArmoryComponent *ArmoryComponent = GetArmoryComponent()) {
    return ArmoryComponent->IsItemInStorageGrid(ItemId);
  }

  return false;
}

bool UPlayerInventoryWidgetBase::IsItemRotated(FGuid ItemId) const {
  if (const UPlayerArmoryComponent *ArmoryComponent = GetArmoryComponent()) {
    return ArmoryComponent->IsItemRotated(ItemId);
  }

  return false;
}

FIntPoint UPlayerInventoryWidgetBase::GetItemFootprint(FGuid ItemId,
                                                       bool bRotated) const {
  if (const UPlayerArmoryComponent *ArmoryComponent = GetArmoryComponent()) {
    return ArmoryComponent->GetItemFootprint(ItemId, bRotated);
  }

  return FIntPoint(1, 1);
}

void UPlayerInventoryWidgetBase::BeginItemDrag(FGuid ItemId) {
  DraggedItemId = FGuid();
  bDraggedItemRotated = false;

  if (!ItemId.IsValid()) {
    return;
  }

  FInventoryItemViewData ItemViewData;
  if (!GetItemViewData(ItemId, ItemViewData)) {
    return;
  }

  DraggedItemId = ItemId;
  bDraggedItemRotated = ItemViewData.bIsRotated;
}

void UPlayerInventoryWidgetBase::EndItemDrag() {
  DraggedItemId = FGuid();
  bDraggedItemRotated = false;
}

bool UPlayerInventoryWidgetBase::ToggleDraggedItemRotation() {
  if (!DraggedItemId.IsValid() || !CanRotateItem(DraggedItemId)) {
    return false;
  }

  bDraggedItemRotated = !bDraggedItemRotated;
  return true;
}

FIntPoint UPlayerInventoryWidgetBase::GetDraggedItemFootprint() const {
  return DraggedItemId.IsValid() ? GetItemFootprint(DraggedItemId, bDraggedItemRotated)
                                 : FIntPoint(1, 1);
}

bool UPlayerInventoryWidgetBase::CanMoveDraggedItemToGrid(FIntPoint GridPosition) const {
  return DraggedItemId.IsValid() &&
         CanMoveItemToGrid(DraggedItemId, GridPosition, bDraggedItemRotated);
}

bool UPlayerInventoryWidgetBase::MoveDraggedItemToGrid(FIntPoint GridPosition) {
  if (!DraggedItemId.IsValid()) {
    return false;
  }

  const bool bMoved =
      MoveItemToGrid(DraggedItemId, GridPosition, bDraggedItemRotated);
  if (bMoved) {
    EndItemDrag();
  }

  return bMoved;
}

bool UPlayerInventoryWidgetBase::CanMoveDraggedItemToLoadout(
    EWeaponLoadoutSlot LoadoutSlot) const {
  return DraggedItemId.IsValid() &&
         CanMoveItemToLoadout(DraggedItemId, LoadoutSlot);
}

bool UPlayerInventoryWidgetBase::MoveDraggedItemToLoadout(
    EWeaponLoadoutSlot LoadoutSlot) {
  if (!DraggedItemId.IsValid()) {
    return false;
  }

  const bool bMoved = MoveItemToLoadout(DraggedItemId, LoadoutSlot);
  if (bMoved) {
    EndItemDrag();
  }

  return bMoved;
}

void UPlayerInventoryWidgetBase::RebuildInventoryView() {
  InitializeNamedWidgets();
  RefreshLoadoutSlots();
  RefreshStats();
  RefreshGridItems();
  HideGridPreview();
}

void UPlayerInventoryWidgetBase::CloseInventory() {
  if (AMainPlayerController *MainPlayerController = ResolveMainPlayerController()) {
    MainPlayerController->CloseInventory();
  }
}

UInventoryItemWidgetBase *UPlayerInventoryWidgetBase::CreateInventoryItemWidget(
    const FInventoryItemViewData &ItemViewData, bool bIsDragVisual) const {
  const TSubclassOf<UInventoryItemWidgetBase> WidgetClass =
      ResolveInventoryItemWidgetClass();
  if (!WidgetClass || !GetOwningPlayer()) {
    return nullptr;
  }

  UInventoryItemWidgetBase *ItemWidget =
      CreateWidget<UInventoryItemWidgetBase>(GetOwningPlayer(), WidgetClass);
  if (ItemWidget) {
    ItemWidget->SetupItem(const_cast<UPlayerInventoryWidgetBase *>(this), ItemViewData,
                          DefaultCellSize, bIsDragVisual);
  }

  return ItemWidget;
}

void UPlayerInventoryWidgetBase::HandleWidgetRefreshRequested() {
  RebuildInventoryView();
}

void UPlayerInventoryWidgetBase::HandleCloseButtonClicked() { CloseInventory(); }

void UPlayerInventoryWidgetBase::InitializeNamedWidgets() {
  CachedWeightText = Cast<UTextBlock>(GetWidgetFromName(TEXT("Text_Weight")));
  CachedCellsText = Cast<UTextBlock>(GetWidgetFromName(TEXT("Text_Cells")));
  CachedCloseButton = Cast<UButton>(GetWidgetFromName(TEXT("Button_Close")));
  CachedLoadoutSlot1 =
      Cast<ULoadoutSlotWidgetBase>(GetWidgetFromName(TEXT("LoadoutSlot_1")));
  CachedLoadoutSlot2 =
      Cast<ULoadoutSlotWidgetBase>(GetWidgetFromName(TEXT("LoadoutSlot_2")));
  CachedLoadoutSlot3 =
      Cast<ULoadoutSlotWidgetBase>(GetWidgetFromName(TEXT("LoadoutSlot_3")));
  CachedGridBoundsWidget = GetWidgetFromName(TEXT("SizeBox_Grid"));
  CachedGridItemsCanvas =
      Cast<UCanvasPanel>(GetWidgetFromName(TEXT("CanvasPanel_GridItems")));
  CachedGridPreviewCanvas =
      Cast<UCanvasPanel>(GetWidgetFromName(TEXT("CanvasPanel_GridPreview")));
  CachedGridPreviewBorder =
      Cast<UBorder>(GetWidgetFromName(TEXT("Border_GridPreview")));

  EnsurePreviewBorderCreated();

  if (CachedCloseButton &&
      !CachedCloseButton->OnClicked.IsAlreadyBound(
          this, &UPlayerInventoryWidgetBase::HandleCloseButtonClicked)) {
    CachedCloseButton->OnClicked.AddDynamic(
        this, &UPlayerInventoryWidgetBase::HandleCloseButtonClicked);
  }

  bNamedWidgetsInitialized = true;
}

void UPlayerInventoryWidgetBase::EnsurePreviewBorderCreated() {
  if (CachedGridPreviewBorder || !CachedGridPreviewCanvas || !WidgetTree) {
    return;
  }

  CachedGridPreviewBorder = WidgetTree->ConstructWidget<UBorder>(
      UBorder::StaticClass(), TEXT("Border_GridPreview_Auto"));
  CachedGridPreviewBorder->SetVisibility(ESlateVisibility::Hidden);
  CachedGridPreviewCanvas->AddChildToCanvas(CachedGridPreviewBorder);
}

void UPlayerInventoryWidgetBase::RefreshStats() {
  if (CachedWeightText) {
    CachedWeightText->SetText(FText::Format(
        LOCTEXT("InventoryWeightText", "Weight: {0}"),
        FText::AsNumber(GetUsedWeight())));
  }

  if (CachedCellsText) {
    CachedCellsText->SetText(FText::Format(
        LOCTEXT("InventoryCellsText", "Cells: {0} / {1}"),
        FText::AsNumber(GetUsedGridCells()),
        FText::AsNumber(GetTotalGridCells())));
  }
}

void UPlayerInventoryWidgetBase::RefreshLoadoutSlots() {
  if (CachedLoadoutSlot1) {
    CachedLoadoutSlot1->InitializeSlotWidget(this,
                                             EWeaponLoadoutSlot::Slot1Primary);
  }

  if (CachedLoadoutSlot2) {
    CachedLoadoutSlot2->InitializeSlotWidget(
        this, EWeaponLoadoutSlot::Slot2Secondary);
  }

  if (CachedLoadoutSlot3) {
    CachedLoadoutSlot3->InitializeSlotWidget(
        this, EWeaponLoadoutSlot::Slot3Auxiliary);
  }
}

void UPlayerInventoryWidgetBase::RefreshGridItems() {
  if (!CachedGridItemsCanvas) {
    return;
  }

  CachedGridItemsCanvas->ClearChildren();

  for (const FInventoryItemViewData &ItemViewData : GetStorageGridItems()) {
    UInventoryItemWidgetBase *ItemWidget =
        CreateInventoryItemWidget(ItemViewData, false);
    if (!ItemWidget) {
      continue;
    }

    if (UCanvasPanelSlot *CanvasSlot =
            CachedGridItemsCanvas->AddChildToCanvas(ItemWidget)) {
      CanvasSlot->SetAutoSize(false);
      CanvasSlot->SetPosition(
          GetInventoryItemCanvasPosition(ItemViewData, DefaultCellSize));
      CanvasSlot->SetSize(
          GetInventoryItemCanvasSize(ItemViewData, DefaultCellSize));
    }
  }
}

void UPlayerInventoryWidgetBase::HideGridPreview() {
  if (CachedGridPreviewBorder) {
    CachedGridPreviewBorder->SetVisibility(ESlateVisibility::Hidden);
  }
}

void UPlayerInventoryWidgetBase::ShowGridPreview(FIntPoint GridCell,
                                                 bool bIsValidDrop) {
  EnsurePreviewBorderCreated();
  if (!CachedGridPreviewBorder) {
    return;
  }

  if (UCanvasPanelSlot *CanvasSlot =
          Cast<UCanvasPanelSlot>(CachedGridPreviewBorder->Slot)) {
    CanvasSlot->SetAutoSize(false);
    CanvasSlot->SetPosition(GetGridCellCanvasPosition(GridCell, DefaultCellSize));
    CanvasSlot->SetSize(GetDraggedItemCanvasSize(DefaultCellSize));
  }

  CachedGridPreviewBorder->SetBrushColor(bIsValidDrop ? PreviewValidColor
                                                      : PreviewInvalidColor);
  CachedGridPreviewBorder->SetVisibility(ESlateVisibility::Visible);
}

TSubclassOf<UInventoryItemWidgetBase>
UPlayerInventoryWidgetBase::ResolveInventoryItemWidgetClass() const {
  if (InventoryItemWidgetClass) {
    return InventoryItemWidgetClass;
  }

  return LoadClass<UInventoryItemWidgetBase>(
      nullptr,
      TEXT(
          "/Game/UI/Inventory_Asset/Assets/WBP_InventoryItem.WBP_InventoryItem_C"));
}

#undef LOCTEXT_NAMESPACE
