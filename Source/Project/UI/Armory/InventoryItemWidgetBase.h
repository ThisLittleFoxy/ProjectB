// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Blueprint/UserWidget.h"
#include "Inventory/InventoryItemTypes.h"
#include "TimerManager.h"
#include "InventoryItemWidgetBase.generated.h"

class UBorder;
class UDragDropOperation;
class UImage;
class UPlayerInventoryWidgetBase;
class UTextBlock;

UCLASS(BlueprintType, Blueprintable)
class PROJECT_API UInventoryItemWidgetBase : public UUserWidget {
  GENERATED_BODY()

public:
  virtual void NativeConstruct() override;
  virtual void NativeDestruct() override;
  virtual void NativeOnMouseEnter(const FGeometry &InGeometry,
                                  const FPointerEvent &InMouseEvent) override;
  virtual void NativeOnMouseLeave(const FPointerEvent &InMouseEvent) override;
  virtual FReply NativeOnMouseButtonDown(
      const FGeometry &InGeometry,
      const FPointerEvent &InMouseEvent) override;
  virtual void NativeOnDragDetected(const FGeometry &InGeometry,
                                    const FPointerEvent &InMouseEvent,
                                    UDragDropOperation *&OutOperation) override;
  virtual void NativeOnDragCancelled(const FDragDropEvent &InDragDropEvent,
                                     UDragDropOperation *InOperation) override;

  UFUNCTION(BlueprintCallable, Category = "Inventory")
  void SetupItem(UPlayerInventoryWidgetBase *InInventoryRoot,
                 const FInventoryItemViewData &InItemData, float InCellSize,
                 bool bInIsDragVisual = false);

protected:
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inventory",
            meta = (ExposeOnSpawn = "true"))
  TObjectPtr<UPlayerInventoryWidgetBase> InventoryRoot;

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inventory",
            meta = (ExposeOnSpawn = "true"))
  FInventoryItemViewData ItemData;

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inventory|Layout",
            meta = (ClampMin = "1.0", ExposeOnSpawn = "true"))
  float CellSize = 72.0f;

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inventory",
            meta = (ExposeOnSpawn = "true"))
  bool bIsDragVisual = false;

private:
  void ApplyItemVisualState();
  void CacheNamedWidgets();
  void BeginTooltipDelay();
  void ShowTooltip();
  void ClearTooltip();

  UPROPERTY(Transient)
  TObjectPtr<UImage> CachedIconImage;

  UPROPERTY(Transient)
  TObjectPtr<UTextBlock> CachedNameText;

  UPROPERTY(Transient)
  TObjectPtr<UTextBlock> CachedSizeText;

  UPROPERTY(Transient)
  TObjectPtr<UBorder> CachedRotateHintBorder;

  FTimerHandle TooltipDelayTimerHandle;
  bool bWidgetsCached = false;
};
