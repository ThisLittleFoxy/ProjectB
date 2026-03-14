// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "UI/Armory/ArmoryWidgetBase.h"
#include "WeaponShopItemCardWidgetBase.generated.h"

class AWeaponBase;
class UTexture2D;
class UWeaponShopWidgetBase;

UCLASS(BlueprintType, Blueprintable)
class PROJECT_API UWeaponShopItemCardWidgetBase : public UArmoryWidgetBase {
  GENERATED_BODY()

public:
  UFUNCTION(BlueprintCallable, Category = "Shop Item")
  void SetOfferConfiguration(TSubclassOf<AWeaponBase> WeaponClass, int32 Price,
                             FText DisplayNameOverride = FText::GetEmpty(),
                             UTexture2D *IconOverride = nullptr);

  UFUNCTION(BlueprintCallable, Category = "Shop Item")
  void SetOfferConfigurationFromWeaponDefaults(
      TSubclassOf<AWeaponBase> WeaponClass,
      FText DisplayNameOverride = FText::GetEmpty(),
      UTexture2D *IconOverride = nullptr);

  UFUNCTION(BlueprintPure, Category = "Shop Item")
  TSubclassOf<AWeaponBase> GetConfiguredWeaponClass() const {
    return ConfiguredWeaponClass;
  }

  UFUNCTION(BlueprintPure, Category = "Shop Item")
  int32 GetConfiguredPrice() const { return ConfiguredPrice; }

  UFUNCTION(BlueprintPure, Category = "Shop Item")
  int32 GetResolvedPrice() const;

  UFUNCTION(BlueprintPure, Category = "Shop Item")
  FText GetConfiguredPriceText() const;

  UFUNCTION(BlueprintPure, Category = "Shop Item")
  FText GetResolvedPriceText() const;

  UFUNCTION(BlueprintPure, Category = "Shop Item")
  FText GetResolvedDisplayName() const;

  UFUNCTION(BlueprintPure, Category = "Shop Item")
  UTexture2D *GetResolvedIcon() const;

  UFUNCTION(BlueprintPure, Category = "Shop Item")
  EWeaponEquipGroup GetResolvedEquipGroup() const;

  UFUNCTION(BlueprintPure, Category = "Shop Item")
  bool IsConfiguredWeaponOwned() const;

  UFUNCTION(BlueprintPure, Category = "Shop Item")
  bool IsConfiguredWeaponAffordable() const;

  UFUNCTION(BlueprintPure, Category = "Shop Item")
  bool IsPurchaseBlockedByPendingAssignment() const;

  UFUNCTION(BlueprintPure, Category = "Shop Item")
  bool CanPurchaseConfiguredWeapon() const;

  UFUNCTION(BlueprintPure, Category = "Shop Item")
  FText GetPurchaseButtonText() const;

  UFUNCTION(BlueprintPure, Category = "Shop Item")
  FText GetStatusText() const;

  UFUNCTION(BlueprintCallable, Category = "Shop Item")
  bool TryPurchaseConfiguredWeapon();

protected:
  UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Shop Item")
  TSubclassOf<AWeaponBase> ConfiguredWeaponClass;

  UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Shop Item")
  bool bOverridePrice = false;

  UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Shop Item",
            meta = (ClampMin = "0", EditCondition = "bOverridePrice"))
  int32 ConfiguredPrice = 0;

  UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Shop Item")
  FText ConfiguredDisplayNameOverride;

  UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Shop Item")
  TObjectPtr<UTexture2D> ConfiguredIconOverride = nullptr;

private:
  UWeaponShopWidgetBase *ResolveOwningShopWidget() const;
};
