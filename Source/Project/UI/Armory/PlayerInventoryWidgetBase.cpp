// Copyright Epic Games, Inc. All Rights Reserved.

#include "UI/Armory/PlayerInventoryWidgetBase.h"
#include "Controllers/MainPlayerController.h"
#include "Inventory/PlayerArmoryComponent.h"

TArray<TSubclassOf<AWeaponBase>> UPlayerInventoryWidgetBase::GetOwnedWeapons() const {
  if (const UPlayerArmoryComponent *ArmoryComponent = GetArmoryComponent()) {
    return ArmoryComponent->GetOwnedWeapons();
  }

  return {};
}

TSubclassOf<AWeaponBase> UPlayerInventoryWidgetBase::GetAssignedWeaponForSlot(
    EWeaponLoadoutSlot LoadoutSlot) const {
  if (const UPlayerArmoryComponent *ArmoryComponent = GetArmoryComponent()) {
    return ArmoryComponent->GetAssignedWeaponForSlot(LoadoutSlot);
  }

  return nullptr;
}

bool UPlayerInventoryWidgetBase::IsSlotOccupied(
    EWeaponLoadoutSlot LoadoutSlot) const {
  if (const UPlayerArmoryComponent *ArmoryComponent = GetArmoryComponent()) {
    return ArmoryComponent->IsSlotOccupied(LoadoutSlot);
  }

  return false;
}

bool UPlayerInventoryWidgetBase::CanAssignWeaponToSlot(
    TSubclassOf<AWeaponBase> WeaponClass,
    EWeaponLoadoutSlot LoadoutSlot) const {
  if (const UPlayerArmoryComponent *ArmoryComponent = GetArmoryComponent()) {
    return ArmoryComponent->CanAssignWeaponToSlot(WeaponClass, LoadoutSlot);
  }

  return false;
}

bool UPlayerInventoryWidgetBase::AssignWeaponToSlot(
    TSubclassOf<AWeaponBase> WeaponClass,
    EWeaponLoadoutSlot LoadoutSlot) {
  if (UPlayerArmoryComponent *ArmoryComponent = GetArmoryComponent()) {
    return ArmoryComponent->AssignWeaponToSlot(WeaponClass, LoadoutSlot);
  }

  return false;
}

void UPlayerInventoryWidgetBase::ClearSlot(EWeaponLoadoutSlot LoadoutSlot) {
  if (UPlayerArmoryComponent *ArmoryComponent = GetArmoryComponent()) {
    ArmoryComponent->ClearSlot(LoadoutSlot);
  }
}

bool UPlayerInventoryWidgetBase::SetActiveSlot(EWeaponLoadoutSlot LoadoutSlot) {
  if (UPlayerArmoryComponent *ArmoryComponent = GetArmoryComponent()) {
    return ArmoryComponent->SetActiveSlot(LoadoutSlot);
  }

  return false;
}

EWeaponLoadoutSlot UPlayerInventoryWidgetBase::GetActiveSlot() const {
  if (const UPlayerArmoryComponent *ArmoryComponent = GetArmoryComponent()) {
    return ArmoryComponent->GetActiveSlot();
  }

  return EWeaponLoadoutSlot::Slot1Primary;
}

void UPlayerInventoryWidgetBase::CloseInventory() {
  if (AMainPlayerController *MainPlayerController = ResolveMainPlayerController()) {
    MainPlayerController->CloseInventory();
  }
}
