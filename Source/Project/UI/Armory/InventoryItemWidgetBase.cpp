// Copyright Epic Games, Inc. All Rights Reserved.

#include "UI/Armory/InventoryItemWidgetBase.h"
#include "Blueprint/DragDropOperation.h"
#include "Blueprint/WidgetBlueprintLibrary.h"
#include "Components/Border.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Engine/World.h"
#include "InputCoreTypes.h"
#include "TimerManager.h"
#include "UI/Armory/InventoryItemTooltipWidgetBase.h"
#include "UI/Armory/PlayerInventoryWidgetBase.h"

void UInventoryItemWidgetBase::NativeConstruct() {
  Super::NativeConstruct();

  CacheNamedWidgets();
  ApplyItemVisualState();
}

void UInventoryItemWidgetBase::NativeDestruct() {
  ClearTooltip();
  Super::NativeDestruct();
}

void UInventoryItemWidgetBase::NativeOnMouseEnter(
    const FGeometry &InGeometry, const FPointerEvent &InMouseEvent) {
  Super::NativeOnMouseEnter(InGeometry, InMouseEvent);

  if (!bIsDragVisual && InventoryRoot) {
    InventoryRoot->SetHoveredItem(ItemData.ItemId);
    BeginTooltipDelay();
  }
}

void UInventoryItemWidgetBase::NativeOnMouseLeave(
    const FPointerEvent &InMouseEvent) {
  if (!bIsDragVisual && InventoryRoot) {
    InventoryRoot->ClearHoveredItem(ItemData.ItemId);
  }

  ClearTooltip();

  Super::NativeOnMouseLeave(InMouseEvent);
}

FReply UInventoryItemWidgetBase::NativeOnMouseButtonDown(
    const FGeometry &InGeometry, const FPointerEvent &InMouseEvent) {
  if (!bIsDragVisual && InventoryRoot && ItemData.ItemId.IsValid()) {
    return UWidgetBlueprintLibrary::DetectDragIfPressed(
               InMouseEvent, this, EKeys::LeftMouseButton)
        .NativeReply;
  }

  return Super::NativeOnMouseButtonDown(InGeometry, InMouseEvent);
}

void UInventoryItemWidgetBase::NativeOnDragDetected(
    const FGeometry &InGeometry, const FPointerEvent &InMouseEvent,
    UDragDropOperation *&OutOperation) {
  Super::NativeOnDragDetected(InGeometry, InMouseEvent, OutOperation);

  if (!InventoryRoot || !ItemData.ItemId.IsValid()) {
    return;
  }

  ClearTooltip();
  InventoryRoot->BeginItemDrag(ItemData.ItemId);

  UDragDropOperation *Operation = UWidgetBlueprintLibrary::CreateDragDropOperation(
      UDragDropOperation::StaticClass());
  if (!Operation) {
    return;
  }

  Operation->Pivot = EDragPivot::MouseDown;
  OutOperation = Operation;
}

void UInventoryItemWidgetBase::NativeOnDragCancelled(
    const FDragDropEvent &InDragDropEvent, UDragDropOperation *InOperation) {
  Super::NativeOnDragCancelled(InDragDropEvent, InOperation);

  if (InventoryRoot) {
    InventoryRoot->EndItemDrag();
  }
}

void UInventoryItemWidgetBase::SetupItem(
    UPlayerInventoryWidgetBase *InInventoryRoot,
    const FInventoryItemViewData &InItemData, float InCellSize,
    bool bInIsDragVisual) {
  InventoryRoot = InInventoryRoot;
  ItemData = InItemData;
  CellSize = FMath::Max(1.0f, InCellSize);
  bIsDragVisual = bInIsDragVisual;
  ApplyItemVisualState();
}

void UInventoryItemWidgetBase::ApplyItemVisualState() {
  CacheNamedWidgets();

  if (CachedIconImage) {
    CachedIconImage->SetBrushFromTexture(ItemData.Icon, true);
  }

  if (CachedNameText) {
    CachedNameText->SetText(ItemData.DisplayName);
  }

  if (CachedSizeText) {
    CachedSizeText->SetText(
        InventoryRoot ? InventoryRoot->GetInventoryItemFootprintText(ItemData)
                      : FText::GetEmpty());
  }

  if (CachedRotateHintBorder) {
    CachedRotateHintBorder->SetVisibility(ItemData.bCanRotate
                                              ? ESlateVisibility::Visible
                                              : ESlateVisibility::Collapsed);
  }

  ClearTooltip();

  SetRenderOpacity(bIsDragVisual ? 0.9f : 1.0f);
  SetVisibility(bIsDragVisual ? ESlateVisibility::SelfHitTestInvisible
                              : ESlateVisibility::Visible);
}

void UInventoryItemWidgetBase::CacheNamedWidgets() {
  if (bWidgetsCached) {
    return;
  }

  CachedIconImage = Cast<UImage>(GetWidgetFromName(TEXT("Image_Icon")));
  CachedNameText = Cast<UTextBlock>(GetWidgetFromName(TEXT("Text_Name")));
  CachedSizeText = Cast<UTextBlock>(GetWidgetFromName(TEXT("Text_Size")));
  CachedRotateHintBorder =
      Cast<UBorder>(GetWidgetFromName(TEXT("Border_RotateHint")));
  bWidgetsCached = true;
}

void UInventoryItemWidgetBase::BeginTooltipDelay() {
  ClearTooltip();

  if (bIsDragVisual || !InventoryRoot) {
    return;
  }

  if (UWorld *World = GetWorld()) {
    World->GetTimerManager().SetTimer(
        TooltipDelayTimerHandle, this, &UInventoryItemWidgetBase::ShowTooltip,
        InventoryRoot->GetInventoryItemTooltipHoverDelay(), false);
  }
}

void UInventoryItemWidgetBase::ShowTooltip() {
  if (bIsDragVisual || !InventoryRoot) {
    return;
  }

  if (UUserWidget *TooltipWidget =
          InventoryRoot->CreateInventoryItemTooltipWidget(ItemData)) {
    SetToolTip(TooltipWidget);
    return;
  }

  SetToolTipText(InventoryRoot->GetInventoryItemTooltipText(ItemData));
}

void UInventoryItemWidgetBase::ClearTooltip() {
  if (UWorld *World = GetWorld()) {
    World->GetTimerManager().ClearTimer(TooltipDelayTimerHandle);
  }

  SetToolTip(nullptr);
  SetToolTipText(FText::GetEmpty());
}
