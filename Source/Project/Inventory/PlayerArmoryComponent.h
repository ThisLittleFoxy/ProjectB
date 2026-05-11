// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Combat/WeaponLoadoutTypes.h"
#include "Components/ActorComponent.h"
#include "Inventory/InventoryItemTypes.h"
#include "Save/ProjectSaveTypes.h"
#include "PlayerArmoryComponent.generated.h"

class APawn;
class AWeaponBase;
class UCombatComponent;
class UCurrencyComponent;
class UPlayerArmoryComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FArmoryChangedSignature,
                                            UPlayerArmoryComponent *,
                                            ArmoryComponent);

UCLASS(ClassGroup = (Inventory), meta = (BlueprintSpawnableComponent))
class PROJECT_API UPlayerArmoryComponent : public UActorComponent {
  GENERATED_BODY()

public:
  UPlayerArmoryComponent();

  virtual void BeginPlay() override;

  UPROPERTY(BlueprintAssignable, Category = "Armory|Events")
  FArmoryChangedSignature OnArmoryChanged;

  UFUNCTION(BlueprintCallable, Category = "Armory|Session")
  void InitializeEmptySession(int32 InitialCurrency);

  UFUNCTION(BlueprintPure, Category = "Armory|Session")
  bool HasInitializedSessionState() const { return bHasInitializedSessionState; }

  UFUNCTION(BlueprintCallable, Category = "Armory|Session")
  void BindToPawn(APawn *NewPawn);

  UFUNCTION(BlueprintCallable, Category = "Armory|Session")
  void ApplyStateToBoundPawn();

  UFUNCTION(BlueprintCallable, Category = "Armory|Save")
  void CaptureSaveData(FPlayerSaveData &OutPlayerSaveData);

  UFUNCTION(BlueprintCallable, Category = "Armory|Save")
  void RestoreFromSaveData(const FPlayerSaveData &PlayerSaveData);

  UFUNCTION(BlueprintPure, Category = "Armory|Currency")
  int32 GetCurrency() const { return CachedCurrency; }

  UFUNCTION(BlueprintPure, Category = "Armory|Shop")
  bool CanPurchaseWeapon(TSubclassOf<AWeaponBase> WeaponClass, int32 Price) const;

  UFUNCTION(BlueprintCallable, Category = "Armory|Shop")
  bool PurchaseWeapon(TSubclassOf<AWeaponBase> WeaponClass, int32 Price);

  UFUNCTION(BlueprintCallable, Category = "Armory|Shop")
  bool GrantPurchasedWeapon(TSubclassOf<AWeaponBase> WeaponClass);

  UFUNCTION(BlueprintPure, Category = "Armory|Shop")
  bool CanStoreWeapon(TSubclassOf<AWeaponBase> WeaponClass) const;

  UFUNCTION(BlueprintPure, Category = "Armory|Inventory")
  bool HasOwnedWeapon(TSubclassOf<AWeaponBase> WeaponClass) const;

  UFUNCTION(BlueprintPure, Category = "Armory|Inventory")
  TArray<TSubclassOf<AWeaponBase>> GetOwnedWeapons() const;

  UFUNCTION(BlueprintPure, Category = "Armory|Inventory")
  TSubclassOf<AWeaponBase> GetAssignedWeaponForSlot(EWeaponLoadoutSlot Slot) const;

  UFUNCTION(BlueprintPure, Category = "Armory|Inventory")
  TArray<TSubclassOf<AWeaponBase>> GetAssignedWeaponsBySlot() const;

  UFUNCTION(BlueprintPure, Category = "Armory|Inventory")
  bool IsSlotOccupied(EWeaponLoadoutSlot Slot) const;

  UFUNCTION(BlueprintPure, Category = "Armory|Inventory")
  bool CanAssignWeaponToSlot(TSubclassOf<AWeaponBase> WeaponClass,
                             EWeaponLoadoutSlot Slot) const;

  UFUNCTION(BlueprintCallable, Category = "Armory|Inventory")
  bool AssignWeaponToSlot(TSubclassOf<AWeaponBase> WeaponClass,
                          EWeaponLoadoutSlot Slot);

  UFUNCTION(BlueprintCallable, Category = "Armory|Inventory")
  void ClearSlot(EWeaponLoadoutSlot Slot);

  UFUNCTION(BlueprintPure, Category = "Armory|Inventory")
  EWeaponLoadoutSlot GetActiveSlot() const { return ActiveSlot; }

  UFUNCTION(BlueprintCallable, Category = "Armory|Inventory")
  bool SetActiveSlot(EWeaponLoadoutSlot Slot);

  UFUNCTION(BlueprintPure, Category = "Armory|Inventory")
  int32 GetStorageGridWidth() const { return FMath::Max(1, StorageGridWidth); }

  UFUNCTION(BlueprintPure, Category = "Armory|Inventory")
  int32 GetStorageGridHeight() const { return FMath::Max(1, StorageGridHeight); }

  UFUNCTION(BlueprintCallable, Category = "Armory|Inventory")
  void SetStorageGridDimensions(int32 NewGridWidth, int32 NewGridHeight);

  UFUNCTION(BlueprintPure, Category = "Armory|Inventory")
  int32 GetTotalGridCells() const;

  UFUNCTION(BlueprintPure, Category = "Armory|Inventory")
  int32 GetUsedGridCells() const;

  UFUNCTION(BlueprintPure, Category = "Armory|Inventory")
  float GetUsedWeight() const;

  UFUNCTION(BlueprintPure, Category = "Armory|Inventory")
  TArray<FInventoryItemViewData> GetAllInventoryItems() const;

  UFUNCTION(BlueprintPure, Category = "Armory|Inventory")
  TArray<FInventoryItemViewData> GetStorageGridItems() const;

  UFUNCTION(BlueprintPure, Category = "Armory|Inventory")
  bool GetItemViewData(FGuid ItemId, FInventoryItemViewData &OutItemViewData) const;

  UFUNCTION(BlueprintPure, Category = "Armory|Inventory")
  bool GetLoadoutItemViewData(EWeaponLoadoutSlot Slot,
                              FInventoryItemViewData &OutItemViewData) const;

  UFUNCTION(BlueprintPure, Category = "Armory|Inventory")
  FGuid GetLoadoutItemId(EWeaponLoadoutSlot Slot) const;

  UFUNCTION(BlueprintPure, Category = "Armory|Inventory")
  TArray<FGuid> GetLoadoutItemIds() const { return LoadoutItemIds; }

  UFUNCTION(BlueprintPure, Category = "Armory|Inventory")
  bool CanMoveItemToGrid(FGuid ItemId, FIntPoint GridPosition,
                         bool bRotated) const;

  UFUNCTION(BlueprintCallable, Category = "Armory|Inventory")
  bool MoveItemToGrid(FGuid ItemId, FIntPoint GridPosition, bool bRotated);

  UFUNCTION(BlueprintPure, Category = "Armory|Inventory")
  bool CanMoveItemToLoadout(FGuid ItemId, EWeaponLoadoutSlot Slot) const;

  UFUNCTION(BlueprintCallable, Category = "Armory|Inventory")
  bool MoveItemToLoadout(FGuid ItemId, EWeaponLoadoutSlot Slot);

  UFUNCTION(BlueprintPure, Category = "Armory|Inventory")
  bool CanRotateItem(FGuid ItemId) const;

  UFUNCTION(BlueprintPure, Category = "Armory|Inventory")
  bool IsItemInStorageGrid(FGuid ItemId) const;

  UFUNCTION(BlueprintPure, Category = "Armory|Inventory")
  bool IsItemRotated(FGuid ItemId) const;

  UFUNCTION(BlueprintPure, Category = "Armory|Inventory")
  FIntPoint GetItemFootprint(FGuid ItemId, bool bRotated) const;

protected:
  UFUNCTION()
  void HandlePawnCurrencyChanged(UCurrencyComponent *CurrencyComponent,
                                 int32 CurrentCurrency, int32 DeltaCurrency);

private:
  UPROPERTY(Transient)
  TWeakObjectPtr<APawn> BoundPawn;

  UPROPERTY(Transient)
  TWeakObjectPtr<UCurrencyComponent> BoundCurrencyComponent;

  UPROPERTY(Transient)
  TWeakObjectPtr<UCombatComponent> BoundCombatComponent;

  UPROPERTY(Transient)
  bool bHasInitializedSessionState = false;

  UPROPERTY(Transient)
  int32 CachedCurrency = 0;

  UPROPERTY(EditDefaultsOnly, Category = "Armory|Inventory",
            meta = (ClampMin = "1"))
  int32 StorageGridWidth = 8;

  UPROPERTY(EditDefaultsOnly, Category = "Armory|Inventory",
            meta = (ClampMin = "1"))
  int32 StorageGridHeight = 6;

  UPROPERTY(Transient)
  TArray<FInventoryItemInstance> OwnedItems;

  UPROPERTY(Transient)
  TArray<FGuid> LoadoutItemIds;

  UPROPERTY(Transient)
  EWeaponLoadoutSlot ActiveSlot = EWeaponLoadoutSlot::Slot1Primary;

  void EnsureLoadoutItemIdsInitialized();
  void BroadcastArmoryChanged();
  void BindCurrencyComponent(UCurrencyComponent *CurrencyComponent);
  void UnbindCurrencyComponent();
  void SyncRuntimeWeaponStateFromBoundPawn();
  bool IsWeaponAllowedInSlot(TSubclassOf<AWeaponBase> WeaponClass,
                             EWeaponLoadoutSlot Slot) const;
  bool IsItemAllowedInLoadout(const FInventoryItemInstance &Item,
                              EWeaponLoadoutSlot Slot) const;
  EWeaponLoadoutSlot FindFallbackActiveSlot() const;
  FInventoryItemInstance *FindItemById(const FGuid &ItemId);
  const FInventoryItemInstance *FindItemById(const FGuid &ItemId) const;
  FInventoryItemInstance *FindWeaponItem(TSubclassOf<AWeaponBase> WeaponClass);
  const FInventoryItemInstance *
  FindWeaponItem(TSubclassOf<AWeaponBase> WeaponClass) const;
  FIntPoint GetSanitizedFootprint(const FInventoryItemInstance &Item,
                                  bool bRotated) const;
  FInventoryItemViewData
  BuildItemViewData(const FInventoryItemInstance &Item) const;
  bool BuildWeaponItemInstance(TSubclassOf<AWeaponBase> WeaponClass,
                               FInventoryItemInstance &OutItem) const;
  bool BuildInventoryItemFromSaveData(const FArmoryItemSaveData &ItemSaveData,
                                      FInventoryItemInstance &OutItem) const;
  bool CanPlaceInGrid(const FInventoryItemInstance &Item,
                      const FInventoryGridPlacement &Placement,
                      const FGuid &IgnoredItemId = FGuid(),
                      const FGuid &AdditionalIgnoredItemId = FGuid()) const;
  bool FindFirstFit(const FInventoryItemInstance &Item,
                    FInventoryGridPlacement &OutPlacement,
                    const FGuid &AdditionalIgnoredItemId = FGuid()) const;
  void BuildOccupiedMask(TArray<bool> &OutMask,
                         const FGuid &IgnoredItemId = FGuid(),
                         const FGuid &AdditionalIgnoredItemId = FGuid()) const;
  bool MoveItemToGridInternal(FInventoryItemInstance &Item,
                              const FInventoryGridPlacement &Placement,
                              const FGuid &AdditionalIgnoredItemId = FGuid());
  bool MoveItemToLoadoutInternal(FInventoryItemInstance &Item,
                                 EWeaponLoadoutSlot Slot);
  bool RotatePlacement(const FInventoryItemInstance &Item,
                       FInventoryGridPlacement &Placement) const;
  bool CanRelocateLoadoutOccupantToStorage(
      EWeaponLoadoutSlot Slot, const FGuid &IgnoredItemId = FGuid()) const;
  bool RelocateLoadoutOccupantToStorage(EWeaponLoadoutSlot Slot,
                                        const FGuid &IgnoredItemId = FGuid());
};
