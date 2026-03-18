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
class ULoadoutSlotWidgetBase;
class UTextBlock;
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

protected:
  virtual void HandleWidgetRefreshRequested() override;

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inventory|Layout",
            meta = (ClampMin = "1.0"))
  float DefaultCellSize = 72.0f;

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inventory|Layout")
  FLinearColor PreviewValidColor = FLinearColor(0.15f, 0.8f, 0.35f, 0.35f);

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inventory|Layout")
  FLinearColor PreviewInvalidColor = FLinearColor(0.9f, 0.2f, 0.2f, 0.35f);

  UPROPERTY(EditAnywhere, Category = "Inventory|Layout")
  TSubclassOf<UInventoryItemWidgetBase> InventoryItemWidgetClass;

private:
  UFUNCTION()
  void HandleCloseButtonClicked();

  void InitializeNamedWidgets();
  void EnsurePreviewBorderCreated();
  void RebuildInventoryView();
  void RefreshStats();
  void RefreshLoadoutSlots();
  void RefreshGridItems();
  void HideGridPreview();
  void ShowGridPreview(FIntPoint GridCell, bool bIsValidDrop);
  TSubclassOf<UInventoryItemWidgetBase> ResolveInventoryItemWidgetClass() const;

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
  TObjectPtr<UWidget> CachedGridBoundsWidget;

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
};
