// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "ProjectCharacter.generated.h"

class UCameraComponent;
class UCombatComponent;
class UCurrencyComponent;
class UHealthComponent;
class UInteractionComponent;

/**
 * Native base class for the playable character.
 *
 * Blueprints should keep meshes, animation blueprints, montages, and authored
 * defaults here, while gameplay behavior lives in this C++ class and reusable
 * components.
 */
UCLASS(Blueprintable)
class PROJECT_API AProjectCharacter : public ACharacter {
  GENERATED_BODY()

public:
  AProjectCharacter();

  virtual void GetLifetimeReplicatedProps(
      TArray<FLifetimeProperty> &OutLifetimeProps) const override;

  UFUNCTION(BlueprintPure, Category = "Character|Components")
  UCameraComponent *GetFirstPersonCameraComponent() const {
    return FirstPersonCameraComponent;
  }

  UFUNCTION(BlueprintPure, Category = "Character|Components")
  USkeletalMeshComponent *GetFirstPersonMeshComponent() const {
    return FirstPersonMeshComponent;
  }

  UFUNCTION(BlueprintPure, Category = "Character|Components")
  UCombatComponent *GetCombatComponent() const { return CombatComponent; }

  UFUNCTION(BlueprintPure, Category = "Character|Components")
  UInteractionComponent *GetInteractionComponent() const {
    return InteractionComponent;
  }

  UFUNCTION(BlueprintPure, Category = "Character|Components")
  UHealthComponent *GetHealthComponent() const { return HealthComponent; }

  UFUNCTION(BlueprintPure, Category = "Character|Components")
  UCurrencyComponent *GetCurrencyComponent() const { return CurrencyComponent; }

  UFUNCTION(BlueprintCallable, Category = "Character|Movement")
  void RequestStartSprint();

  UFUNCTION(BlueprintCallable, Category = "Character|Movement")
  void RequestStopSprint();

  UFUNCTION(BlueprintCallable, Category = "Character|Interaction")
  void RequestInteract();

  UFUNCTION(BlueprintCallable, Category = "Character|Combat")
  void RequestStartFire();

  UFUNCTION(BlueprintCallable, Category = "Character|Combat")
  void RequestStopFire();

  UFUNCTION(BlueprintCallable, Category = "Character|Combat")
  bool Reload();

  UFUNCTION(BlueprintCallable, Category = "Character|Combat")
  void StartScope();

  UFUNCTION(BlueprintCallable, Category = "Character|Combat")
  void StopScope();

  UFUNCTION(BlueprintCallable, Category = "Character|Combat")
  bool EquipNextWeapon();

  UFUNCTION(BlueprintCallable, Category = "Character|Combat")
  bool EquipPreviousWeapon();

  UFUNCTION(BlueprintPure, Category = "Character|Movement")
  bool WantsToSprint() const { return bWantsToSprint; }

protected:
  virtual void BeginPlay() override;

  UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Character|Camera",
            meta = (AllowPrivateAccess = "true"))
  TObjectPtr<UCameraComponent> FirstPersonCameraComponent;

  UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Character|Mesh",
            meta = (AllowPrivateAccess = "true"))
  TObjectPtr<USkeletalMeshComponent> FirstPersonMeshComponent;

  UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Character|Combat",
            meta = (AllowPrivateAccess = "true"))
  TObjectPtr<UCombatComponent> CombatComponent;

  UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Character|Interaction",
            meta = (AllowPrivateAccess = "true"))
  TObjectPtr<UInteractionComponent> InteractionComponent;

  UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Character|Health",
            meta = (AllowPrivateAccess = "true"))
  TObjectPtr<UHealthComponent> HealthComponent;

  UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Character|Currency",
            meta = (AllowPrivateAccess = "true"))
  TObjectPtr<UCurrencyComponent> CurrencyComponent;

  UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Character|Camera",
            meta = (ClampMin = "1.0", ClampMax = "179.0"))
  float DefaultFieldOfView = 90.0f;

  UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Character|Movement",
            meta = (ClampMin = "0.0"))
  float WalkSpeed = 500.0f;

  UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Character|Movement",
            meta = (ClampMin = "0.0"))
  float SprintSpeed = 800.0f;

  UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Character|Movement")
  bool bCanSprint = true;

  UPROPERTY(ReplicatedUsing = OnRep_WantsToSprint, VisibleInstanceOnly,
            BlueprintReadOnly, Category = "Character|Movement")
  bool bWantsToSprint = false;

private:
  UFUNCTION(Server, Reliable)
  void ServerSetWantsToSprint(bool bNewWantsToSprint);

  UFUNCTION(Server, Reliable)
  void ServerEquipWeaponSlot(int32 SlotIndex);

  UFUNCTION()
  void OnRep_WantsToSprint();

  void SetWantsToSprint(bool bNewWantsToSprint);
  void ApplyMovementSpeed();
};
