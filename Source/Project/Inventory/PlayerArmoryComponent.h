// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Combat/WeaponLoadoutTypes.h"
#include "Components/ActorComponent.h"
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

  UFUNCTION(BlueprintPure, Category = "Armory|Currency")
  int32 GetCurrency() const { return CachedCurrency; }

  UFUNCTION(BlueprintPure, Category = "Armory|Shop")
  bool CanPurchaseWeapon(TSubclassOf<AWeaponBase> WeaponClass, int32 Price) const;

  UFUNCTION(BlueprintCallable, Category = "Armory|Shop")
  bool PurchaseWeapon(TSubclassOf<AWeaponBase> WeaponClass, int32 Price);

  UFUNCTION(BlueprintPure, Category = "Armory|Inventory")
  bool HasOwnedWeapon(TSubclassOf<AWeaponBase> WeaponClass) const;

  UFUNCTION(BlueprintPure, Category = "Armory|Inventory")
  TArray<TSubclassOf<AWeaponBase>> GetOwnedWeapons() const {
    return OwnedWeaponClasses;
  }

  UFUNCTION(BlueprintPure, Category = "Armory|Inventory")
  TSubclassOf<AWeaponBase> GetAssignedWeaponForSlot(EWeaponLoadoutSlot Slot) const;

  UFUNCTION(BlueprintPure, Category = "Armory|Inventory")
  TArray<TSubclassOf<AWeaponBase>> GetAssignedWeaponsBySlot() const {
    return SlotAssignments;
  }

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

  UPROPERTY(Transient)
  TArray<TSubclassOf<AWeaponBase>> OwnedWeaponClasses;

  UPROPERTY(Transient)
  TArray<TSubclassOf<AWeaponBase>> SlotAssignments;

  UPROPERTY(Transient)
  EWeaponLoadoutSlot ActiveSlot = EWeaponLoadoutSlot::Slot1Primary;

  void EnsureSlotAssignmentsInitialized();
  void BroadcastArmoryChanged();
  void BindCurrencyComponent(UCurrencyComponent *CurrencyComponent);
  void UnbindCurrencyComponent();
  bool IsWeaponAllowedInSlot(TSubclassOf<AWeaponBase> WeaponClass,
                             EWeaponLoadoutSlot Slot) const;
  EWeaponLoadoutSlot FindFallbackActiveSlot() const;
};
