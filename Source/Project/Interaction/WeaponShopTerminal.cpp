// Copyright Epic Games, Inc. All Rights Reserved.

#include "Interaction/WeaponShopTerminal.h"
#include "Controllers/MainPlayerController.h"
#include "GameFramework/Character.h"

AWeaponShopTerminal::AWeaponShopTerminal() {
  bReplicates = true;
  SetReplicateMovement(false);

  InteractionName = ShopDisplayName;
  InteractionAction = ShopActionText;
}

bool AWeaponShopTerminal::OnInteract_Implementation(ACharacter *PlayerCharacter) {
  if (!PlayerCharacter) {
    return false;
  }

  if (AMainPlayerController *MainPlayerController =
          Cast<AMainPlayerController>(PlayerCharacter->GetController())) {
    MainPlayerController->OpenWeaponShop(this);
    return true;
  }

  return false;
}

FText AWeaponShopTerminal::GetInteractionName_Implementation() const {
  return ShopDisplayName;
}

FText AWeaponShopTerminal::GetInteractionAction_Implementation() const {
  return ShopActionText;
}
