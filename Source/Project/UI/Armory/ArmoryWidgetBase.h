// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Blueprint/UserWidget.h"
#include "Combat/WeaponLoadoutTypes.h"
#include "ArmoryWidgetBase.generated.h"

class AWeaponBase;
class AMainPlayerController;
class UPlayerArmoryComponent;
class UTexture2D;

UCLASS(BlueprintType, Blueprintable)
class PROJECT_API UArmoryWidgetBase : public UUserWidget {
  GENERATED_BODY()

public:
  virtual void NativeConstruct() override;
  virtual void NativeDestruct() override;

  UFUNCTION(BlueprintPure, Category = "Armory")
  UPlayerArmoryComponent *GetArmoryComponent() const;

  UFUNCTION(BlueprintPure, Category = "Armory")
  int32 GetCurrentMoney() const;

  UFUNCTION(BlueprintCallable, Category = "Armory")
  void RequestWidgetRefresh();

  UFUNCTION(BlueprintPure, Category = "Armory|Weapon")
  FText GetWeaponDisplayName(TSubclassOf<AWeaponBase> WeaponClass) const;

  UFUNCTION(BlueprintPure, Category = "Armory|Weapon")
  UTexture2D *GetWeaponIcon(TSubclassOf<AWeaponBase> WeaponClass) const;

  UFUNCTION(BlueprintPure, Category = "Armory|Weapon")
  EWeaponEquipGroup GetWeaponEquipGroup(TSubclassOf<AWeaponBase> WeaponClass) const;

  UFUNCTION(BlueprintPure, Category = "Armory|Weapon")
  int32 GetWeaponShopPrice(TSubclassOf<AWeaponBase> WeaponClass) const;

  UFUNCTION(BlueprintPure, Category = "Armory|Loadout")
  FText GetLoadoutSlotDisplayName(EWeaponLoadoutSlot LoadoutSlot) const;

protected:
  UFUNCTION(BlueprintImplementableEvent, Category = "Armory",
            meta = (DisplayName = "On Widget Refresh Requested"))
  void BP_OnWidgetRefreshRequested();

  AMainPlayerController *ResolveMainPlayerController() const;

  virtual void HandleWidgetRefreshRequested();

  void NotifyWidgetRefreshRequested();

private:
  UFUNCTION()
  void HandleArmoryChanged(UPlayerArmoryComponent *ArmoryComponent);

  void RebindObservedArmoryComponent();

  UPROPERTY(Transient)
  TWeakObjectPtr<UPlayerArmoryComponent> ObservedArmoryComponent;
};
