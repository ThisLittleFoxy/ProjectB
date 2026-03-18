// Copyright Epic Games, Inc. All Rights Reserved.

#include "UI/Armory/InventoryItemTooltipWidgetBase.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "UI/Armory/PlayerInventoryWidgetBase.h"

#define LOCTEXT_NAMESPACE "InventoryItemTooltipWidgetBase"

void UInventoryItemTooltipWidgetBase::NativeConstruct() {
  Super::NativeConstruct();

  CacheNamedWidgets();
  ApplyTooltipState();
}

void UInventoryItemTooltipWidgetBase::SetupTooltip(
    UPlayerInventoryWidgetBase *InInventoryRoot,
    const FInventoryItemViewData &InItemData) {
  InventoryRoot = InInventoryRoot;
  ItemData = InItemData;
  ApplyTooltipState();
}

void UInventoryItemTooltipWidgetBase::ApplyTooltipState() {
  CacheNamedWidgets();

  if (CachedIconImage) {
    CachedIconImage->SetBrushFromTexture(ItemData.Icon, true);
  }

  if (CachedTitleText) {
    CachedTitleText->SetText(ItemData.DisplayName);
  }

  if (CachedDetailsText) {
    const FText FootprintText =
        InventoryRoot ? InventoryRoot->GetInventoryItemFootprintText(ItemData)
                      : FText::GetEmpty();
    CachedDetailsText->SetText(FText::Format(
        LOCTEXT("TooltipDetailsText", "Size: {0}\nWeight: {1}"),
        FootprintText, FText::AsNumber(ItemData.Weight)));
  }
}

void UInventoryItemTooltipWidgetBase::CacheNamedWidgets() {
  if (bWidgetsCached) {
    return;
  }

  CachedIconImage = Cast<UImage>(GetWidgetFromName(TEXT("Image_Icon")));
  CachedTitleText = Cast<UTextBlock>(GetWidgetFromName(TEXT("Text_Title")));
  CachedDetailsText =
      Cast<UTextBlock>(GetWidgetFromName(TEXT("Text_Details")));
  bWidgetsCached = true;
}

#undef LOCTEXT_NAMESPACE
