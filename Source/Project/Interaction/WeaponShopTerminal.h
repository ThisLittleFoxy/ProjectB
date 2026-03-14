// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Combat/WeaponLoadoutTypes.h"
#include "Interaction/InteractableActor.h"
#include "WeaponShopTerminal.generated.h"

class ACharacter;

UCLASS(Blueprintable)
class PROJECT_API AWeaponShopTerminal : public AInteractableActor {
  GENERATED_BODY()

public:
  AWeaponShopTerminal();

  virtual bool OnInteract_Implementation(ACharacter *PlayerCharacter) override;
  virtual FText GetInteractionName_Implementation() const override;
  virtual FText GetInteractionAction_Implementation() const override;

  UFUNCTION(BlueprintPure, Category = "Shop")
  TArray<FWeaponShopOffer> GetShopOffers() const { return ShopOffers; }

protected:
  UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Shop")
  FText ShopDisplayName = FText::FromString(TEXT("Weapon Shop"));

  UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Shop")
  FText ShopActionText = FText::FromString(TEXT("Open Shop"));

  UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Shop")
  TArray<FWeaponShopOffer> ShopOffers;
};
