// Copyright Epic Games, Inc. All Rights Reserved.

#include "UI/Armory/ArmoryWidgetBase.h"
#include "Arena/ArenaPlayerState.h"
#include "Combat/WeaponBase.h"
#include "Controllers/MainPlayerController.h"
#include "Inventory/PlayerArmoryComponent.h"

void UArmoryWidgetBase::NativeConstruct() {
  Super::NativeConstruct();

  RebindObservedArmoryComponent();
  NotifyWidgetRefreshRequested();
}

void UArmoryWidgetBase::NativeDestruct() {
  if (UPlayerArmoryComponent *ArmoryComponent = ObservedArmoryComponent.Get()) {
    ArmoryComponent->OnArmoryChanged.RemoveDynamic(
        this, &UArmoryWidgetBase::HandleArmoryChanged);
  }

  ObservedArmoryComponent.Reset();
  Super::NativeDestruct();
}

UPlayerArmoryComponent *UArmoryWidgetBase::GetArmoryComponent() const {
  if (const AMainPlayerController *MainPlayerController =
          ResolveMainPlayerController()) {
    return MainPlayerController->GetPlayerArmoryComponent();
  }

  return nullptr;
}

int32 UArmoryWidgetBase::GetCurrentMoney() const {
  if (const AMainPlayerController *MainPlayerController =
          ResolveMainPlayerController()) {
    if (const AArenaPlayerState *ArenaPlayerState =
            MainPlayerController->GetPlayerState<AArenaPlayerState>()) {
      return ArenaPlayerState->GetSpendableCurrency();
    }
  }

  if (const UPlayerArmoryComponent *ArmoryComponent = GetArmoryComponent()) {
    return ArmoryComponent->GetCurrency();
  }

  return 0;
}

void UArmoryWidgetBase::RequestWidgetRefresh() {
  RebindObservedArmoryComponent();
  NotifyWidgetRefreshRequested();
}

FText UArmoryWidgetBase::GetWeaponDisplayName(
    TSubclassOf<AWeaponBase> WeaponClass) const {
  if (const AWeaponBase *WeaponCDO = WeaponClass->GetDefaultObject<AWeaponBase>()) {
    return WeaponCDO->GetWeaponDisplayName();
  }

  return FText::GetEmpty();
}

UTexture2D *UArmoryWidgetBase::GetWeaponIcon(
    TSubclassOf<AWeaponBase> WeaponClass) const {
  if (const AWeaponBase *WeaponCDO = WeaponClass->GetDefaultObject<AWeaponBase>()) {
    return WeaponCDO->GetWeaponIcon();
  }

  return nullptr;
}

EWeaponEquipGroup UArmoryWidgetBase::GetWeaponEquipGroup(
    TSubclassOf<AWeaponBase> WeaponClass) const {
  if (const AWeaponBase *WeaponCDO = WeaponClass->GetDefaultObject<AWeaponBase>()) {
    return WeaponCDO->GetEquipGroup();
  }

  return EWeaponEquipGroup::Primary;
}

int32 UArmoryWidgetBase::GetWeaponShopPrice(
    TSubclassOf<AWeaponBase> WeaponClass) const {
  if (const AWeaponBase *WeaponCDO = WeaponClass->GetDefaultObject<AWeaponBase>()) {
    return FMath::Max(0, WeaponCDO->GetWeaponShopPrice());
  }

  return 0;
}

FText UArmoryWidgetBase::GetLoadoutSlotDisplayName(
    EWeaponLoadoutSlot LoadoutSlot) const {
  switch (LoadoutSlot) {
  case EWeaponLoadoutSlot::Slot1Primary:
    return FText::FromString(TEXT("Slot 1"));

  case EWeaponLoadoutSlot::Slot2Secondary:
    return FText::FromString(TEXT("Slot 2"));

  case EWeaponLoadoutSlot::Slot3Auxiliary:
    return FText::FromString(TEXT("Slot 3"));

  default:
    break;
  }

  return FText::FromString(TEXT("Slot"));
}

AMainPlayerController *UArmoryWidgetBase::ResolveMainPlayerController() const {
  return Cast<AMainPlayerController>(GetOwningPlayer());
}

void UArmoryWidgetBase::HandleWidgetRefreshRequested() {}

void UArmoryWidgetBase::NotifyWidgetRefreshRequested() {
  BP_OnWidgetRefreshRequested();
  HandleWidgetRefreshRequested();
}

void UArmoryWidgetBase::HandleArmoryChanged(
    UPlayerArmoryComponent *ArmoryComponent) {
  if (ArmoryComponent) {
    NotifyWidgetRefreshRequested();
  }
}

void UArmoryWidgetBase::RebindObservedArmoryComponent() {
  UPlayerArmoryComponent *ArmoryComponent = GetArmoryComponent();
  if (ObservedArmoryComponent.Get() == ArmoryComponent) {
    return;
  }

  if (UPlayerArmoryComponent *PreviousArmoryComponent =
          ObservedArmoryComponent.Get()) {
    PreviousArmoryComponent->OnArmoryChanged.RemoveDynamic(
        this, &UArmoryWidgetBase::HandleArmoryChanged);
  }

  ObservedArmoryComponent = ArmoryComponent;
  if (ArmoryComponent) {
    ArmoryComponent->OnArmoryChanged.AddDynamic(
        this, &UArmoryWidgetBase::HandleArmoryChanged);
  }
}
