// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "UI/Armory/ArmoryWidgetBase.h"
#include "PlayerInventoryWidgetBase.generated.h"

class AWeaponBase;

UCLASS(BlueprintType, Blueprintable)
class PROJECT_API UPlayerInventoryWidgetBase : public UArmoryWidgetBase {
  GENERATED_BODY()

public:
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

  UFUNCTION(BlueprintCallable, Category = "Inventory")
  void CloseInventory();
};
