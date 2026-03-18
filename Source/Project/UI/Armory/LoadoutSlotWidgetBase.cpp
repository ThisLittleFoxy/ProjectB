// Copyright Epic Games, Inc. All Rights Reserved.

#include "UI/Armory/LoadoutSlotWidgetBase.h"
#include "Blueprint/DragDropOperation.h"
#include "Blueprint/WidgetBlueprintLibrary.h"
#include "Components/Border.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "InputCoreTypes.h"
#include "UI/Armory/InventoryItemWidgetBase.h"
#include "UI/Armory/PlayerInventoryWidgetBase.h"

void ULoadoutSlotWidgetBase::NativeConstruct() {
  Super::NativeConstruct();

  CacheNamedWidgets();
  UpdateSlotFromInventory();
}

FReply ULoadoutSlotWidgetBase::NativeOnMouseButtonDown(
    const FGeometry &InGeometry, const FPointerEvent &InMouseEvent) {
  if (bHasItem && InventoryRoot) {
    InventoryRoot->SetActiveSlot(SlotType);
    return UWidgetBlueprintLibrary::DetectDragIfPressed(
               InMouseEvent, this, EKeys::LeftMouseButton)
        .NativeReply;
  }

  return Super::NativeOnMouseButtonDown(InGeometry, InMouseEvent);
}

void ULoadoutSlotWidgetBase::NativeOnDragDetected(
    const FGeometry &InGeometry, const FPointerEvent &InMouseEvent,
    UDragDropOperation *&OutOperation) {
  Super::NativeOnDragDetected(InGeometry, InMouseEvent, OutOperation);

  if (!bHasItem || !InventoryRoot || !SlotItemData.ItemId.IsValid()) {
    return;
  }

  InventoryRoot->BeginItemDrag(SlotItemData.ItemId);

  UDragDropOperation *Operation = UWidgetBlueprintLibrary::CreateDragDropOperation(
      UDragDropOperation::StaticClass());
  if (!Operation) {
    return;
  }

  if (UInventoryItemWidgetBase *DragVisual =
          InventoryRoot->CreateInventoryItemWidget(SlotItemData, true)) {
    Operation->DefaultDragVisual = DragVisual;
  }

  Operation->Pivot = EDragPivot::MouseDown;
  OutOperation = Operation;
}

void ULoadoutSlotWidgetBase::NativeOnDragCancelled(
    const FDragDropEvent &InDragDropEvent, UDragDropOperation *InOperation) {
  Super::NativeOnDragCancelled(InDragDropEvent, InOperation);

  if (InventoryRoot) {
    InventoryRoot->EndItemDrag();
  }
}

void ULoadoutSlotWidgetBase::NativeOnDragEnter(
    const FGeometry &InGeometry, const FDragDropEvent &InDragDropEvent,
    UDragDropOperation *InOperation) {
  Super::NativeOnDragEnter(InGeometry, InDragDropEvent, InOperation);

  const bool bCanDrop =
      InventoryRoot && InventoryRoot->CanMoveDraggedItemToLoadout(SlotType);
  UpdateHoverState(true, bCanDrop);
}

void ULoadoutSlotWidgetBase::NativeOnDragLeave(
    const FDragDropEvent &InDragDropEvent, UDragDropOperation *InOperation) {
  Super::NativeOnDragLeave(InDragDropEvent, InOperation);
  UpdateHoverState(false, false);
}

bool ULoadoutSlotWidgetBase::NativeOnDrop(
    const FGeometry &InGeometry, const FDragDropEvent &InDragDropEvent,
    UDragDropOperation *InOperation) {
  UpdateHoverState(false, false);

  if (!InventoryRoot) {
    return false;
  }

  return InventoryRoot->MoveDraggedItemToLoadout(SlotType);
}

void ULoadoutSlotWidgetBase::InitializeSlotWidget(
    UPlayerInventoryWidgetBase *InInventoryRoot, EWeaponLoadoutSlot InSlotType) {
  InventoryRoot = InInventoryRoot;
  SlotType = InSlotType;
  UpdateSlotFromInventory();
}

void ULoadoutSlotWidgetBase::UpdateSlotFromInventory() {
  CacheNamedWidgets();

  const FText SlotDisplayName =
      InventoryRoot ? InventoryRoot->GetLoadoutSlotDisplayName(SlotType)
                    : FText::FromString(TEXT("Slot"));

  bHasItem =
      InventoryRoot && InventoryRoot->GetLoadoutItemViewData(SlotType, SlotItemData);

  if (CachedLabelText) {
    CachedLabelText->SetText(bHasItem ? FText::Format(
                                            FText::FromString(TEXT("{0}\n{1}")),
                                            SlotDisplayName,
                                            SlotItemData.DisplayName)
                                      : SlotDisplayName);
  }

  if (CachedIconImage) {
    CachedIconImage->SetBrushFromTexture(bHasItem ? SlotItemData.Icon : nullptr, true);
    CachedIconImage->SetVisibility(bHasItem ? ESlateVisibility::Visible
                                            : ESlateVisibility::Collapsed);
  }

  if (CachedActiveMarkerBorder) {
    const bool bIsActiveSlot =
        bHasItem && InventoryRoot && InventoryRoot->GetActiveSlot() == SlotType;
    CachedActiveMarkerBorder->SetVisibility(bIsActiveSlot
                                                ? ESlateVisibility::Visible
                                                : ESlateVisibility::Hidden);
  }

  UpdateHoverState(false, false);
}

void ULoadoutSlotWidgetBase::CacheNamedWidgets() {
  if (bWidgetsCached) {
    return;
  }

  CachedIconImage = Cast<UImage>(GetWidgetFromName(TEXT("Image_Icon")));
  CachedLabelText = Cast<UTextBlock>(GetWidgetFromName(TEXT("Text_Label")));
  CachedActiveMarkerBorder =
      Cast<UBorder>(GetWidgetFromName(TEXT("Border_ActiveMarker")));
  CachedHoverMarkerBorder =
      Cast<UBorder>(GetWidgetFromName(TEXT("Border_HoverMarker")));
  bWidgetsCached = true;
}

void ULoadoutSlotWidgetBase::UpdateHoverState(bool bVisible, bool bIsValidDrop) {
  CacheNamedWidgets();

  if (!CachedHoverMarkerBorder) {
    return;
  }

  CachedHoverMarkerBorder->SetVisibility(bVisible ? ESlateVisibility::Visible
                                                  : ESlateVisibility::Hidden);
  if (bVisible) {
    CachedHoverMarkerBorder->SetBrushColor(bIsValidDrop ? ValidDropColor
                                                        : InvalidDropColor);
  }
}
