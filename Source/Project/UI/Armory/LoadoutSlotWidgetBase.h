// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Blueprint/UserWidget.h"
#include "Combat/WeaponLoadoutTypes.h"
#include "Inventory/InventoryItemTypes.h"
#include "LoadoutSlotWidgetBase.generated.h"

class UBorder;
class UDragDropOperation;
class UImage;
class UPlayerInventoryWidgetBase;
class UTextBlock;

UCLASS(BlueprintType, Blueprintable)
class PROJECT_API ULoadoutSlotWidgetBase : public UUserWidget {
  GENERATED_BODY()

public:
  virtual void NativeConstruct() override;
  virtual FReply NativeOnMouseButtonDown(
      const FGeometry &InGeometry,
      const FPointerEvent &InMouseEvent) override;
  virtual void NativeOnDragDetected(const FGeometry &InGeometry,
                                    const FPointerEvent &InMouseEvent,
                                    UDragDropOperation *&OutOperation) override;
  virtual void NativeOnDragCancelled(const FDragDropEvent &InDragDropEvent,
                                     UDragDropOperation *InOperation) override;
  virtual void NativeOnDragEnter(const FGeometry &InGeometry,
                                 const FDragDropEvent &InDragDropEvent,
                                 UDragDropOperation *InOperation) override;
  virtual void NativeOnDragLeave(const FDragDropEvent &InDragDropEvent,
                                 UDragDropOperation *InOperation) override;
  virtual bool NativeOnDrop(const FGeometry &InGeometry,
                            const FDragDropEvent &InDragDropEvent,
                            UDragDropOperation *InOperation) override;

  void InitializeSlotWidget(UPlayerInventoryWidgetBase *InInventoryRoot,
                            EWeaponLoadoutSlot InSlotType);

  void UpdateSlotFromInventory();

protected:
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inventory",
            meta = (ExposeOnSpawn = "true"))
  TObjectPtr<UPlayerInventoryWidgetBase> InventoryRoot;

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inventory",
            meta = (ExposeOnSpawn = "true"))
  EWeaponLoadoutSlot SlotType = EWeaponLoadoutSlot::Slot1Primary;

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inventory")
  FInventoryItemViewData SlotItemData;

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inventory")
  bool bHasItem = false;

  UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Inventory|Style")
  FLinearColor ValidDropColor = FLinearColor(0.15f, 0.85f, 0.35f, 0.9f);

  UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Inventory|Style")
  FLinearColor InvalidDropColor = FLinearColor(0.9f, 0.2f, 0.2f, 0.9f);

private:
  void CacheNamedWidgets();
  void UpdateHoverState(bool bVisible, bool bIsValidDrop);

  UPROPERTY(Transient)
  TObjectPtr<UImage> CachedIconImage;

  UPROPERTY(Transient)
  TObjectPtr<UTextBlock> CachedLabelText;

  UPROPERTY(Transient)
  TObjectPtr<UBorder> CachedActiveMarkerBorder;

  UPROPERTY(Transient)
  TObjectPtr<UBorder> CachedHoverMarkerBorder;

  bool bWidgetsCached = false;
};
