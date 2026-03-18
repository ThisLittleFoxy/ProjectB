// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Blueprint/UserWidget.h"
#include "Inventory/InventoryItemTypes.h"
#include "InventoryItemTooltipWidgetBase.generated.h"

class UImage;
class UPlayerInventoryWidgetBase;
class UTextBlock;

UCLASS(BlueprintType, Blueprintable)
class PROJECT_API UInventoryItemTooltipWidgetBase : public UUserWidget {
  GENERATED_BODY()

public:
  virtual void NativeConstruct() override;

  UFUNCTION(BlueprintCallable, Category = "Inventory|Tooltip")
  void SetupTooltip(UPlayerInventoryWidgetBase *InInventoryRoot,
                    const FInventoryItemViewData &InItemData);

private:
  void ApplyTooltipState();
  void CacheNamedWidgets();

  UPROPERTY(Transient)
  TObjectPtr<UPlayerInventoryWidgetBase> InventoryRoot;

  UPROPERTY(Transient)
  FInventoryItemViewData ItemData;

  UPROPERTY(Transient)
  TObjectPtr<UImage> CachedIconImage;

  UPROPERTY(Transient)
  TObjectPtr<UTextBlock> CachedTitleText;

  UPROPERTY(Transient)
  TObjectPtr<UTextBlock> CachedDetailsText;

  bool bWidgetsCached = false;
};
