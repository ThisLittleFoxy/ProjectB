// Copyright Epic Games, Inc. All Rights Reserved.

#include "Character/ProjectCharacter.h"

#include "Camera/CameraComponent.h"
#include "Character/CurrencyComponent.h"
#include "Character/HealthComponent.h"
#include "Combat/CombatComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Interaction/InteractionComponent.h"
#include "Net/UnrealNetwork.h"

AProjectCharacter::AProjectCharacter() {
  PrimaryActorTick.bCanEverTick = false;

  bReplicates = true;
  SetReplicateMovement(true);

  GetCapsuleComponent()->InitCapsuleSize(42.0f, 96.0f);

  bUseControllerRotationPitch = false;
  bUseControllerRotationYaw = true;
  bUseControllerRotationRoll = false;

  if (UCharacterMovementComponent *MovementComponent = GetCharacterMovement()) {
    MovementComponent->MaxWalkSpeed = WalkSpeed;
    MovementComponent->JumpZVelocity = 500.0f;
    MovementComponent->AirControl = 0.35f;
  }

  FirstPersonCameraComponent =
      CreateDefaultSubobject<UCameraComponent>(TEXT("FirstPersonCamera"));
  FirstPersonCameraComponent->SetupAttachment(GetCapsuleComponent());
  FirstPersonCameraComponent->SetRelativeLocation(FVector(-10.0f, 0.0f, 64.0f));
  FirstPersonCameraComponent->bUsePawnControlRotation = true;

  FirstPersonMeshComponent =
      CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("FirstPersonMesh"));
  FirstPersonMeshComponent->SetupAttachment(FirstPersonCameraComponent);
  FirstPersonMeshComponent->SetOnlyOwnerSee(true);
  FirstPersonMeshComponent->bCastDynamicShadow = false;
  FirstPersonMeshComponent->CastShadow = false;

  if (USkeletalMeshComponent *CharacterMesh = GetMesh()) {
    CharacterMesh->SetOwnerNoSee(true);
  }

  CombatComponent =
      CreateDefaultSubobject<UCombatComponent>(TEXT("CombatComponent"));
  InteractionComponent =
      CreateDefaultSubobject<UInteractionComponent>(TEXT("InteractionComponent"));
  HealthComponent =
      CreateDefaultSubobject<UHealthComponent>(TEXT("HealthComponent"));
  CurrencyComponent =
      CreateDefaultSubobject<UCurrencyComponent>(TEXT("CurrencyComponent"));
}

void AProjectCharacter::BeginPlay() {
  Super::BeginPlay();

  ApplyMovementSpeed();

  if (FirstPersonCameraComponent) {
    FirstPersonCameraComponent->SetFieldOfView(DefaultFieldOfView);
  }
}

void AProjectCharacter::GetLifetimeReplicatedProps(
    TArray<FLifetimeProperty> &OutLifetimeProps) const {
  Super::GetLifetimeReplicatedProps(OutLifetimeProps);

  DOREPLIFETIME(AProjectCharacter, bWantsToSprint);
}

void AProjectCharacter::RequestStartSprint() {
  SetWantsToSprint(true);

  if (!HasAuthority()) {
    ServerSetWantsToSprint(true);
  }
}

void AProjectCharacter::RequestStopSprint() {
  SetWantsToSprint(false);

  if (!HasAuthority()) {
    ServerSetWantsToSprint(false);
  }
}

void AProjectCharacter::RequestInteract() {
  if (InteractionComponent) {
    InteractionComponent->TryInteract();
  }
}

void AProjectCharacter::RequestStartFire() {
  if (CombatComponent) {
    CombatComponent->StartFire();
  }
}

void AProjectCharacter::RequestStopFire() {
  if (CombatComponent) {
    CombatComponent->StopFire();
  }
}

bool AProjectCharacter::Reload() {
  return CombatComponent ? CombatComponent->Reload() : false;
}

void AProjectCharacter::StartScope() {
  if (CombatComponent) {
    CombatComponent->StartScope();
  }
}

void AProjectCharacter::StopScope() {
  if (CombatComponent) {
    CombatComponent->StopScope();
  }
}

bool AProjectCharacter::EquipNextWeapon() {
  if (!CombatComponent || !CombatComponent->EquipNextWeapon()) {
    return false;
  }

  if (!HasAuthority()) {
    ServerEquipWeaponSlot(CombatComponent->GetCurrentWeaponSlotIndex());
  }

  return true;
}

bool AProjectCharacter::EquipPreviousWeapon() {
  if (!CombatComponent || !CombatComponent->EquipPreviousWeapon()) {
    return false;
  }

  if (!HasAuthority()) {
    ServerEquipWeaponSlot(CombatComponent->GetCurrentWeaponSlotIndex());
  }

  return true;
}

void AProjectCharacter::ServerSetWantsToSprint_Implementation(
    bool bNewWantsToSprint) {
  SetWantsToSprint(bNewWantsToSprint);
}

void AProjectCharacter::ServerEquipWeaponSlot_Implementation(int32 SlotIndex) {
  if (CombatComponent) {
    CombatComponent->EquipWeaponSlot(SlotIndex);
  }
}

void AProjectCharacter::OnRep_WantsToSprint() {
  ApplyMovementSpeed();
}

void AProjectCharacter::SetWantsToSprint(bool bNewWantsToSprint) {
  const bool bNewSprintState = bCanSprint && bNewWantsToSprint;
  if (bWantsToSprint == bNewSprintState) {
    ApplyMovementSpeed();
    return;
  }

  bWantsToSprint = bNewSprintState;
  ApplyMovementSpeed();
}

void AProjectCharacter::ApplyMovementSpeed() {
  if (UCharacterMovementComponent *MovementComponent = GetCharacterMovement()) {
    MovementComponent->MaxWalkSpeed = bWantsToSprint ? SprintSpeed : WalkSpeed;
  }
}
