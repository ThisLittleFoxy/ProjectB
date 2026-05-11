// Copyright Epic Games, Inc. All Rights Reserved.

#include "UI/Armory/WeaponShopWidgetBase.h"
#include "Combat/WeaponBase.h"
#include "Controllers/MainPlayerController.h"
#include "Interaction/WeaponShopTerminal.h"
#include "Inventory/PlayerArmoryComponent.h"

void UWeaponShopWidgetBase::SetShopTerminal(AWeaponShopTerminal *NewTerminal) {
  ShopTerminal = NewTerminal;
  PendingAssignmentWeaponClass = nullptr;
  NotifyWidgetRefreshRequested();
}

TArray<FWeaponShopOffer> UWeaponShopWidgetBase::GetShopOffers() const {
  if (!ShopTerminal.IsValid()) {
    return {};
  }

  TArray<FWeaponShopOffer> ShopOffers = ShopTerminal->GetShopOffers();
  ShopOffers.Sort([this](const FWeaponShopOffer &Left,
                         const FWeaponShopOffer &Right) {
    const int32 LeftResolvedPrice =
        Left.bOverridePrice ? Left.Price : GetWeaponShopPrice(Left.WeaponClass);
    const int32 RightResolvedPrice =
        Right.bOverridePrice ? Right.Price : GetWeaponShopPrice(Right.WeaponClass);
    return Left.SortOrder == Right.SortOrder
               ? LeftResolvedPrice < RightResolvedPrice
               : Left.SortOrder < Right.SortOrder;
  });
  return ShopOffers;
}

TArray<FWeaponShopOfferViewData> UWeaponShopWidgetBase::GetShopOfferViewData() const {
  TArray<FWeaponShopOfferViewData> ViewData;
  const TArray<FWeaponShopOffer> ShopOffers = GetShopOffers();
  ViewData.Reserve(ShopOffers.Num());

  const UPlayerArmoryComponent *ArmoryComponent = GetArmoryComponent();
  const int32 CurrentMoney = GetCurrentMoney();

  for (const FWeaponShopOffer &Offer : ShopOffers) {
    const int32 EffectivePrice =
        Offer.bOverridePrice ? Offer.Price : GetWeaponShopPrice(Offer.WeaponClass);
    FWeaponShopOfferViewData OfferViewData;
    OfferViewData.WeaponClass = Offer.WeaponClass;
    OfferViewData.Price = EffectivePrice;
    OfferViewData.SortOrder = Offer.SortOrder;
    OfferViewData.DisplayName = Offer.OverrideDisplayName.IsEmpty()
                                    ? GetWeaponDisplayName(Offer.WeaponClass)
                                    : Offer.OverrideDisplayName;
    OfferViewData.Icon = Offer.OverrideIcon.Get();
    if (!OfferViewData.Icon) {
      OfferViewData.Icon = GetWeaponIcon(Offer.WeaponClass);
    }
    OfferViewData.EquipGroup = GetWeaponEquipGroup(Offer.WeaponClass);
    OfferViewData.bOwned =
        ArmoryComponent && ArmoryComponent->HasOwnedWeapon(Offer.WeaponClass);
    OfferViewData.bAffordable = CurrentMoney >= EffectivePrice;
    OfferViewData.bFitsInInventory =
        ArmoryComponent && ArmoryComponent->CanStoreWeapon(Offer.WeaponClass);
    OfferViewData.bCanPurchase =
        ArmoryComponent &&
        ArmoryComponent->CanPurchaseWeapon(Offer.WeaponClass, EffectivePrice);

    ViewData.Add(OfferViewData);
  }

  return ViewData;
}

bool UWeaponShopWidgetBase::HasPendingSlotAssignment() const {
  return false;
}

FText UWeaponShopWidgetBase::GetPendingAssignmentWeaponDisplayName() const {
  return GetWeaponDisplayName(PendingAssignmentWeaponClass);
}

UTexture2D *UWeaponShopWidgetBase::GetPendingAssignmentWeaponIcon() const {
  return GetWeaponIcon(PendingAssignmentWeaponClass);
}

EWeaponEquipGroup UWeaponShopWidgetBase::GetPendingAssignmentWeaponEquipGroup() const {
  return GetWeaponEquipGroup(PendingAssignmentWeaponClass);
}

TArray<EWeaponLoadoutSlot>
UWeaponShopWidgetBase::GetPendingAssignmentCompatibleSlots() const {
  TArray<EWeaponLoadoutSlot> Slots;
  if (!PendingAssignmentWeaponClass) {
    return Slots;
  }

  switch (GetPendingAssignmentWeaponEquipGroup()) {
  case EWeaponEquipGroup::Primary:
    Slots.Add(EWeaponLoadoutSlot::Slot1Primary);
    Slots.Add(EWeaponLoadoutSlot::Slot2Secondary);
    break;

  case EWeaponEquipGroup::Auxiliary:
    Slots.Add(EWeaponLoadoutSlot::Slot3Auxiliary);
    break;

  default:
    break;
  }

  return Slots;
}

bool UWeaponShopWidgetBase::CanPurchaseWeapon(TSubclassOf<AWeaponBase> WeaponClass,
                                              int32 Price) const {
  if (const UPlayerArmoryComponent *ArmoryComponent = GetArmoryComponent()) {
    return WeaponClass && Price >= 0 && !ArmoryComponent->HasOwnedWeapon(WeaponClass) &&
           GetCurrentMoney() >= Price && ArmoryComponent->CanStoreWeapon(WeaponClass);
  }

  return false;
}

bool UWeaponShopWidgetBase::CanStoreWeapon(
    TSubclassOf<AWeaponBase> WeaponClass) const {
  if (const UPlayerArmoryComponent *ArmoryComponent = GetArmoryComponent()) {
    return ArmoryComponent->CanStoreWeapon(WeaponClass);
  }

  return false;
}

bool UWeaponShopWidgetBase::CanAssignPendingWeaponToSlot(
    EWeaponLoadoutSlot LoadoutSlot) const {
  return false;
}

bool UWeaponShopWidgetBase::IsPurchaseFlowBlocked() const {
  return false;
}

bool UWeaponShopWidgetBase::PurchaseWeapon(TSubclassOf<AWeaponBase> WeaponClass,
                                           int32 Price) {
  if (AMainPlayerController *MainPlayerController =
          ResolveMainPlayerController()) {
    if (MainPlayerController->RequestArenaPurchaseWeapon(
            ShopTerminal.Get(), WeaponClass, Price)) {
      PendingAssignmentWeaponClass = nullptr;
      NotifyWidgetRefreshRequested();
      return true;
    }
  }

  UPlayerArmoryComponent *ArmoryComponent = GetArmoryComponent();
  if (!ArmoryComponent || !ArmoryComponent->PurchaseWeapon(WeaponClass, Price)) {
    return false;
  }

  PendingAssignmentWeaponClass = nullptr;
  NotifyWidgetRefreshRequested();
  return true;
}

bool UWeaponShopWidgetBase::AssignPendingWeaponToSlot(
    EWeaponLoadoutSlot LoadoutSlot) {
  return false;
}

void UWeaponShopWidgetBase::SkipPendingAssignment() {
  PendingAssignmentWeaponClass = nullptr;
  NotifyWidgetRefreshRequested();
}

void UWeaponShopWidgetBase::CloseShop() {
  if (AMainPlayerController *MainPlayerController = ResolveMainPlayerController()) {
    MainPlayerController->CloseWeaponShop();
  }
}
