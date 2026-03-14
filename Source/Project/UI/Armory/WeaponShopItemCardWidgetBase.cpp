// Copyright Epic Games, Inc. All Rights Reserved.

#include "UI/Armory/WeaponShopItemCardWidgetBase.h"
#include "Combat/WeaponBase.h"
#include "Inventory/PlayerArmoryComponent.h"
#include "UI/Armory/WeaponShopWidgetBase.h"

void UWeaponShopItemCardWidgetBase::SetOfferConfiguration(
    TSubclassOf<AWeaponBase> WeaponClass, int32 Price, FText DisplayNameOverride,
    UTexture2D *IconOverride) {
  ConfiguredWeaponClass = WeaponClass;
  bOverridePrice = true;
  ConfiguredPrice = FMath::Max(0, Price);
  ConfiguredDisplayNameOverride = DisplayNameOverride;
  ConfiguredIconOverride = IconOverride;

  RequestWidgetRefresh();
}

void UWeaponShopItemCardWidgetBase::SetOfferConfigurationFromWeaponDefaults(
    TSubclassOf<AWeaponBase> WeaponClass, FText DisplayNameOverride,
    UTexture2D *IconOverride) {
  ConfiguredWeaponClass = WeaponClass;
  bOverridePrice = false;
  ConfiguredPrice = 0;
  ConfiguredDisplayNameOverride = DisplayNameOverride;
  ConfiguredIconOverride = IconOverride;

  RequestWidgetRefresh();
}

int32 UWeaponShopItemCardWidgetBase::GetResolvedPrice() const {
  return bOverridePrice ? FMath::Max(0, ConfiguredPrice)
                        : GetWeaponShopPrice(ConfiguredWeaponClass);
}

FText UWeaponShopItemCardWidgetBase::GetConfiguredPriceText() const {
  return GetResolvedPriceText();
}

FText UWeaponShopItemCardWidgetBase::GetResolvedPriceText() const {
  return FText::AsNumber(GetResolvedPrice());
}

FText UWeaponShopItemCardWidgetBase::GetResolvedDisplayName() const {
  return ConfiguredDisplayNameOverride.IsEmpty()
             ? GetWeaponDisplayName(ConfiguredWeaponClass)
             : ConfiguredDisplayNameOverride;
}

UTexture2D *UWeaponShopItemCardWidgetBase::GetResolvedIcon() const {
  return ConfiguredIconOverride ? ConfiguredIconOverride.Get()
                                : GetWeaponIcon(ConfiguredWeaponClass);
}

EWeaponEquipGroup UWeaponShopItemCardWidgetBase::GetResolvedEquipGroup() const {
  return GetWeaponEquipGroup(ConfiguredWeaponClass);
}

bool UWeaponShopItemCardWidgetBase::IsConfiguredWeaponOwned() const {
  if (const UPlayerArmoryComponent *ArmoryComponent = GetArmoryComponent()) {
    return ArmoryComponent->HasOwnedWeapon(ConfiguredWeaponClass);
  }

  return false;
}

bool UWeaponShopItemCardWidgetBase::IsConfiguredWeaponAffordable() const {
  return GetCurrentMoney() >= GetResolvedPrice();
}

bool UWeaponShopItemCardWidgetBase::IsPurchaseBlockedByPendingAssignment() const {
  if (const UWeaponShopWidgetBase *ShopWidget = ResolveOwningShopWidget()) {
    return ShopWidget->HasPendingSlotAssignment();
  }

  return false;
}

bool UWeaponShopItemCardWidgetBase::CanPurchaseConfiguredWeapon() const {
  if (const UWeaponShopWidgetBase *ShopWidget = ResolveOwningShopWidget()) {
    return !ShopWidget->HasPendingSlotAssignment() &&
           ShopWidget->CanPurchaseWeapon(ConfiguredWeaponClass, GetResolvedPrice());
  }

  return false;
}

FText UWeaponShopItemCardWidgetBase::GetPurchaseButtonText() const {
  if (IsConfiguredWeaponOwned()) {
    return FText::FromString(TEXT("Owned"));
  }

  if (IsPurchaseBlockedByPendingAssignment()) {
    return FText::FromString(TEXT("Assign First"));
  }

  if (!IsConfiguredWeaponAffordable()) {
    return FText::FromString(TEXT("Not Enough"));
  }

  return FText::FromString(TEXT("Buy"));
}

FText UWeaponShopItemCardWidgetBase::GetStatusText() const {
  if (IsConfiguredWeaponOwned()) {
    return FText::FromString(TEXT("Owned"));
  }

  if (IsPurchaseBlockedByPendingAssignment()) {
    return FText::FromString(TEXT("Select a slot for the previous purchase"));
  }

  if (!IsConfiguredWeaponAffordable()) {
    return FText::FromString(TEXT("Not enough coins"));
  }

  return FText::GetEmpty();
}

bool UWeaponShopItemCardWidgetBase::TryPurchaseConfiguredWeapon() {
  if (UWeaponShopWidgetBase *ShopWidget = ResolveOwningShopWidget()) {
    return ShopWidget->PurchaseWeapon(ConfiguredWeaponClass, GetResolvedPrice());
  }

  return false;
}

UWeaponShopWidgetBase *UWeaponShopItemCardWidgetBase::ResolveOwningShopWidget() const {
  return GetTypedOuter<UWeaponShopWidgetBase>();
}
