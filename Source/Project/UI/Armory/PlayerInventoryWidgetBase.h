// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Inventory/InventoryItemTypes.h"
#include "UI/Armory/ArmoryWidgetBase.h"
#include "PlayerInventoryWidgetBase.generated.h"

class AWeaponBase;
class UBorder;
class UButton;
class UCanvasPanel;
class UDragDropOperation;
class UInventoryItemWidgetBase;
class UInventoryItemTooltipWidgetBase;
class ULoadoutSlotWidgetBase;
class USizeBox;
class UTextBlock;
class UUniformGridPanel;
class UUserWidget;
class UWidget;

UCLASS(BlueprintType, Blueprintable)
class PROJECT_API UPlayerInventoryWidgetBase : public UArmoryWidgetBase {
  GENERATED_BODY()

public:
  virtual void NativeConstruct() override;
  virtual void NativeDestruct() override;
  virtual FReply NativeOnPreviewKeyDown(const FGeometry &InGeometry,
                                        const FKeyEvent &InKeyEvent) override;
  virtual bool NativeOnDragOver(const FGeometry &InGeometry,
                                const FDragDropEvent &InDragDropEvent,
                                UDragDropOperation *InOperation) override;
  virtual void NativeOnDragLeave(const FDragDropEvent &InDragDropEvent,
                                 UDragDropOperation *InOperation) override;
  virtual bool NativeOnDrop(const FGeometry &InGeometry,
                            const FDragDropEvent &InDragDropEvent,
                            UDragDropOperation *InOperation) override;

  UFUNCTION(BlueprintPure, Category = "Inventory")
  TArray<TSubclassOf<AWeaponBase>> GetOwnedWeapons() const;

  UFUNCTION(BlueprintPure, Category = "Inventory")
  TSubclassOf<AWeaponBase>
  GetAssignedWeaponForSlot(EWeaponLoadoutSlot LoadoutSlot) const;

  UFUNCTION(BlueprintPure, Category = "Inventory")
  bool IsSlotOccupied(EWeaponLoadoutSlot LoadoutSlot) const;

  UFUNCTION(BlueprintPure, Category = "Inventory")
  bool CanAssignWeaponToSlot(TSubclassOf<AWeaponBase> WeaponClass,
                             EWeaponLoadoutSlot LoadoutSlot) const;

  UFUNCTION(BlueprintCallable, Category = "Inventory")
  bool AssignWeaponToSlot(TSubclassOf<AWeaponBase> WeaponClass,
                          EWeaponLoadoutSlot LoadoutSlot);

  UFUNCTION(BlueprintCallable, Category = "Inventory")
  void ClearSlot(EWeaponLoadoutSlot LoadoutSlot);

  UFUNCTION(BlueprintCallable, Category = "Inventory")
  bool SetActiveSlot(EWeaponLoadoutSlot LoadoutSlot);

  UFUNCTION(BlueprintPure, Category = "Inventory")
  EWeaponLoadoutSlot GetActiveSlot() const;

  UFUNCTION(BlueprintPure, Category = "Inventory")
  int32 GetStorageGridWidth() const;

  UFUNCTION(BlueprintPure, Category = "Inventory")
  int32 GetStorageGridHeight() const;

  UFUNCTION(BlueprintPure, Category = "Inventory")
  int32 GetTotalGridCells() const;

  UFUNCTION(BlueprintPure, Category = "Inventory")
  int32 GetUsedGridCells() const;

  UFUNCTION(BlueprintPure, Category = "Inventory")
  float GetUsedWeight() const;

  UFUNCTION(BlueprintPure, Category = "Inventory")
  TArray<FInventoryItemViewData> GetAllInventoryItems() const;

  UFUNCTION(BlueprintPure, Category = "Inventory")
  TArray<FInventoryItemViewData> GetStorageGridItems() const;

  UFUNCTION(BlueprintPure, Category = "Inventory|Layout")
  FVector2D GetStorageGridPixelSize(float CellSize) const;

  UFUNCTION(BlueprintPure, Category = "Inventory|Layout")
  int32 GetConfiguredStorageGridColumns() const {
    return FMath::Max(1, StorageGridColumns);
  }

  UFUNCTION(BlueprintPure, Category = "Inventory|Layout")
  int32 GetConfiguredStorageGridRows() const {
    return FMath::Max(1, StorageGridRows);
  }

  UFUNCTION(BlueprintPure, Category = "Inventory|Layout")
  FVector2D GetGridCellCanvasPosition(FIntPoint GridCell, float CellSize) const;

  UFUNCTION(BlueprintPure, Category = "Inventory|Layout")
  FVector2D GetInventoryItemCanvasPosition(
      const FInventoryItemViewData &ItemViewData, float CellSize) const;

  UFUNCTION(BlueprintPure, Category = "Inventory|Layout")
  FVector2D GetInventoryItemCanvasSize(
      const FInventoryItemViewData &ItemViewData, float CellSize) const;

  UFUNCTION(BlueprintPure, Category = "Inventory|Layout")
  FVector2D GetDraggedItemCanvasSize(float CellSize) const;

  UFUNCTION(BlueprintPure, Category = "Inventory|Layout")
  FText GetInventoryItemFootprintText(
      const FInventoryItemViewData &ItemViewData) const;

  UFUNCTION(BlueprintPure, Category = "Inventory|Layout")
  FText GetInventoryItemGridPositionText(
      const FInventoryItemViewData &ItemViewData) const;

  UFUNCTION(BlueprintPure, Category = "Inventory|Layout")
  FText GetInventoryItemTooltipText(
      const FInventoryItemViewData &ItemViewData) const;

  UFUNCTION(BlueprintPure, Category = "Inventory|Tooltip")
  float GetInventoryItemTooltipHoverDelay() const {
    return FMath::Max(0.0f, InventoryItemTooltipHoverDelay);
  }

  UFUNCTION(BlueprintPure, Category = "Inventory|Layout")
  bool TryGetGridCellFromLocalPosition(FVector2D LocalPosition, float CellSize,
                                       FIntPoint &OutGridCell) const;

  UFUNCTION(BlueprintPure, Category = "Inventory")
  bool GetItemViewData(FGuid ItemId, FInventoryItemViewData &OutItemViewData) const;

  UFUNCTION(BlueprintPure, Category = "Inventory")
  bool GetLoadoutItemViewData(EWeaponLoadoutSlot LoadoutSlot,
                              FInventoryItemViewData &OutItemViewData) const;

  UFUNCTION(BlueprintPure, Category = "Inventory")
  FGuid GetLoadoutItemId(EWeaponLoadoutSlot LoadoutSlot) const;

  UFUNCTION(BlueprintPure, Category = "Inventory")
  TArray<FGuid> GetLoadoutItemIds() const;

  UFUNCTION(BlueprintPure, Category = "Inventory")
  bool CanMoveItemToGrid(FGuid ItemId, FIntPoint GridPosition,
                         bool bRotated) const;

  UFUNCTION(BlueprintCallable, Category = "Inventory")
  bool MoveItemToGrid(FGuid ItemId, FIntPoint GridPosition, bool bRotated);

  UFUNCTION(BlueprintPure, Category = "Inventory")
  bool CanMoveItemToLoadout(FGuid ItemId, EWeaponLoadoutSlot LoadoutSlot) const;

  UFUNCTION(BlueprintCallable, Category = "Inventory")
  bool MoveItemToLoadout(FGuid ItemId, EWeaponLoadoutSlot LoadoutSlot);

  UFUNCTION(BlueprintPure, Category = "Inventory")
  bool CanRotateItem(FGuid ItemId) const;

  UFUNCTION(BlueprintPure, Category = "Inventory")
  bool IsItemInStorageGrid(FGuid ItemId) const;

  UFUNCTION(BlueprintPure, Category = "Inventory")
  bool IsItemRotated(FGuid ItemId) const;

  UFUNCTION(BlueprintPure, Category = "Inventory")
  FIntPoint GetItemFootprint(FGuid ItemId, bool bRotated) const;

  UFUNCTION(BlueprintCallable, Category = "Inventory")
  void BeginItemDrag(FGuid ItemId);

  UFUNCTION(BlueprintCallable, Category = "Inventory")
  void SetHoveredItem(FGuid ItemId);

  UFUNCTION(BlueprintCallable, Category = "Inventory")
  void ClearHoveredItem(FGuid ItemId);

  UFUNCTION(BlueprintCallable, Category = "Inventory")
  void EndItemDrag();

  UFUNCTION(BlueprintCallable, Category = "Inventory")
  bool ToggleDraggedItemRotation();

  UFUNCTION(BlueprintPure, Category = "Inventory")
  FGuid GetDraggedItemId() const { return DraggedItemId; }

  UFUNCTION(BlueprintPure, Category = "Inventory")
  bool HasDraggedItem() const { return DraggedItemId.IsValid(); }

  UFUNCTION(BlueprintPure, Category = "Inventory")
  bool IsDraggedItemRotated() const { return bDraggedItemRotated; }

  UFUNCTION(BlueprintPure, Category = "Inventory")
  FIntPoint GetDraggedItemFootprint() const;

  UFUNCTION(BlueprintPure, Category = "Inventory")
  bool CanMoveDraggedItemToGrid(FIntPoint GridPosition) const;

  UFUNCTION(BlueprintCallable, Category = "Inventory")
  bool MoveDraggedItemToGrid(FIntPoint GridPosition);

  UFUNCTION(BlueprintPure, Category = "Inventory")
  bool CanMoveDraggedItemToLoadout(EWeaponLoadoutSlot LoadoutSlot) const;

  UFUNCTION(BlueprintCallable, Category = "Inventory")
  bool MoveDraggedItemToLoadout(EWeaponLoadoutSlot LoadoutSlot);

  UFUNCTION(BlueprintCallable, Category = "Inventory")
  void CloseInventory();

  UInventoryItemWidgetBase *
  CreateInventoryItemWidget(const FInventoryItemViewData &ItemViewData,
                            bool bIsDragVisual) const;

  UInventoryItemTooltipWidgetBase *CreateInventoryItemTooltipWidget(
      const FInventoryItemViewData &ItemViewData) const;

protected:
  virtual void HandleWidgetRefreshRequested() override;

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inventory|Layout",
            meta = (ClampMin = "1.0"))
  float DefaultCellSize = 72.0f;

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inventory|Layout",
            meta = (ClampMin = "1"))
  int32 StorageGridColumns = 8;

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inventory|Layout",
            meta = (ClampMin = "1"))
  int32 StorageGridRows = 6;

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inventory|Layout",
            meta = (ClampMin = "0.0"))
  float GridCellInset = 1.0f;

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inventory|Layout")
  FLinearColor GridCellFillColor = FLinearColor(1.0f, 1.0f, 1.0f, 0.03f);

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inventory|Layout")
  FLinearColor GridCellOutlineColor = FLinearColor(1.0f, 1.0f, 1.0f, 0.08f);

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inventory|Layout")
  FLinearColor PreviewValidColor = FLinearColor(0.15f, 0.8f, 0.35f, 0.35f);

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inventory|Layout")
  FLinearColor PreviewInvalidColor = FLinearColor(0.9f, 0.2f, 0.2f, 0.35f);

  UPROPERTY(EditAnywhere, Category = "Inventory|Layout")
  TSubclassOf<UInventoryItemWidgetBase> InventoryItemWidgetClass;

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inventory|Tooltip",
            meta = (ClampMin = "0.0"))
  float InventoryItemTooltipHoverDelay = 0.35f;

  UPROPERTY(EditAnywhere, Category = "Inventory|Tooltip")
  TSubclassOf<UInventoryItemTooltipWidgetBase> InventoryItemTooltipWidgetClass;

private:
  UFUNCTION()
  void HandleCloseButtonClicked();

  void InitializeNamedWidgets();
  void EnsurePreviewBorderCreated();
  void RebuildInventoryView();
  void UpdateGridWidgetSize();
  void RebuildGridBackground();
  void RefreshStats();
  void RefreshLoadoutSlots();
  void RefreshGridItems();
  bool RotateHoveredItemInPlace();
  void HideGridPreview();
  void ShowGridPreview(FIntPoint GridCell, bool bIsValidDrop);
  TSubclassOf<UInventoryItemWidgetBase> ResolveInventoryItemWidgetClass() const;
  TSubclassOf<UInventoryItemTooltipWidgetBase>
  ResolveInventoryItemTooltipWidgetClass() const;

  UPROPERTY(Transient)
  TObjectPtr<UTextBlock> CachedWeightText;

  UPROPERTY(Transient)
  TObjectPtr<UTextBlock> CachedCellsText;

  UPROPERTY(Transient)
  TObjectPtr<UButton> CachedCloseButton;

  UPROPERTY(Transient)
  TObjectPtr<ULoadoutSlotWidgetBase> CachedLoadoutSlot1;

  UPROPERTY(Transient)
  TObjectPtr<ULoadoutSlotWidgetBase> CachedLoadoutSlot2;

  UPROPERTY(Transient)
  TObjectPtr<ULoadoutSlotWidgetBase> CachedLoadoutSlot3;

  UPROPERTY(Transient)
  TObjectPtr<USizeBox> CachedGridSizeBox;

  UPROPERTY(Transient)
  TObjectPtr<UUniformGridPanel> CachedGridBackgroundPanel;

  UPROPERTY(Transient)
  TObjectPtr<UCanvasPanel> CachedGridItemsCanvas;

  UPROPERTY(Transient)
  TObjectPtr<UCanvasPanel> CachedGridPreviewCanvas;

  UPROPERTY(Transient)
  TObjectPtr<UBorder> CachedGridPreviewBorder;

  UPROPERTY(Transient)
  bool bNamedWidgetsInitialized = false;

  UPROPERTY(Transient)
  FGuid DraggedItemId;

  UPROPERTY(Transient)
  bool bDraggedItemRotated = false;

  UPROPERTY(Transient)
  FGuid HoveredItemId;
};
