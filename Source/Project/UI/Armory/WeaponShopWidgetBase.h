// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Engine/Texture2D.h"
#include "UI/Armory/ArmoryWidgetBase.h"
#include "WeaponShopWidgetBase.generated.h"

class AWeaponShopTerminal;
class AWeaponBase;
class UTexture2D;

USTRUCT(BlueprintType)
struct PROJECT_API FWeaponShopOfferViewData {
  GENERATED_BODY()

  UPROPERTY(BlueprintReadOnly, Category = "Shop")
  TSubclassOf<AWeaponBase> WeaponClass;

  UPROPERTY(BlueprintReadOnly, Category = "Shop")
  int32 Price = 0;

  UPROPERTY(BlueprintReadOnly, Category = "Shop")
  int32 SortOrder = 0;

  UPROPERTY(BlueprintReadOnly, Category = "Shop")
  FText DisplayName;

  UPROPERTY(BlueprintReadOnly, Category = "Shop")
  TObjectPtr<UTexture2D> Icon = nullptr;

  UPROPERTY(BlueprintReadOnly, Category = "Shop")
  EWeaponEquipGroup EquipGroup = EWeaponEquipGroup::Primary;

  UPROPERTY(BlueprintReadOnly, Category = "Shop")
  bool bOwned = false;

  UPROPERTY(BlueprintReadOnly, Category = "Shop")
  bool bAffordable = false;

  UPROPERTY(BlueprintReadOnly, Category = "Shop")
  bool bCanPurchase = false;
};

UCLASS(BlueprintType, Blueprintable)
class PROJECT_API UWeaponShopWidgetBase : public UArmoryWidgetBase {
  GENERATED_BODY()

public:
  UFUNCTION(BlueprintCallable, Category = "Shop")
  void SetShopTerminal(AWeaponShopTerminal *NewTerminal);

  UFUNCTION(BlueprintPure, Category = "Shop")
  AWeaponShopTerminal *GetShopTerminal() const { return ShopTerminal.Get(); }

  UFUNCTION(BlueprintPure, Category = "Shop")
  TArray<FWeaponShopOffer> GetShopOffers() const;

  UFUNCTION(BlueprintPure, Category = "Shop")
  TArray<FWeaponShopOfferViewData> GetShopOfferViewData() const;

  UFUNCTION(BlueprintPure, Category = "Shop")
  bool HasPendingSlotAssignment() const;

  UFUNCTION(BlueprintPure, Category = "Shop")
  TSubclassOf<AWeaponBase> GetPendingAssignmentWeaponClass() const {
    return PendingAssignmentWeaponClass;
  }

  UFUNCTION(BlueprintPure, Category = "Shop")
  FText GetPendingAssignmentWeaponDisplayName() const;

  UFUNCTION(BlueprintPure, Category = "Shop")
  UTexture2D *GetPendingAssignmentWeaponIcon() const;

  UFUNCTION(BlueprintPure, Category = "Shop")
  EWeaponEquipGroup GetPendingAssignmentWeaponEquipGroup() const;

  UFUNCTION(BlueprintPure, Category = "Shop")
  TArray<EWeaponLoadoutSlot> GetPendingAssignmentCompatibleSlots() const;

  UFUNCTION(BlueprintPure, Category = "Shop")
  bool CanPurchaseWeapon(TSubclassOf<AWeaponBase> WeaponClass, int32 Price) const;

  UFUNCTION(BlueprintPure, Category = "Shop")
  bool CanAssignPendingWeaponToSlot(EWeaponLoadoutSlot LoadoutSlot) const;

  UFUNCTION(BlueprintPure, Category = "Shop")
  bool IsPurchaseFlowBlocked() const;

  UFUNCTION(BlueprintCallable, Category = "Shop")
  bool PurchaseWeapon(TSubclassOf<AWeaponBase> WeaponClass, int32 Price);

  UFUNCTION(BlueprintCallable, Category = "Shop")
  bool AssignPendingWeaponToSlot(EWeaponLoadoutSlot LoadoutSlot);

  UFUNCTION(BlueprintCallable, Category = "Shop")
  void SkipPendingAssignment();

  UFUNCTION(BlueprintCallable, Category = "Shop")
  void CloseShop();

private:
  UPROPERTY(Transient)
  TWeakObjectPtr<AWeaponShopTerminal> ShopTerminal;

  UPROPERTY(Transient)
  TSubclassOf<AWeaponBase> PendingAssignmentWeaponClass;
};
