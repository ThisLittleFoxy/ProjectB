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

  if (UCharacterMovementComponent *MovementComponent = GetCharacterMovement()) {
    MovementComponent->MaxWalkSpeed = WalkSpeed;
  }

  if (FirstPersonCameraComponent) {
    FirstPersonCameraComponent->SetFieldOfView(DefaultFieldOfView);
  }
}

void AProjectCharacter::RequestStartSprint() {
  if (!bCanSprint) {
    return;
  }

  bWantsToSprint = true;

  if (UCharacterMovementComponent *MovementComponent = GetCharacterMovement()) {
    MovementComponent->MaxWalkSpeed = SprintSpeed;
  }
}

void AProjectCharacter::RequestStopSprint() {
  bWantsToSprint = false;

  if (UCharacterMovementComponent *MovementComponent = GetCharacterMovement()) {
    MovementComponent->MaxWalkSpeed = WalkSpeed;
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
  return CombatComponent ? CombatComponent->EquipNextWeapon() : false;
}

bool AProjectCharacter::EquipPreviousWeapon() {
  return CombatComponent ? CombatComponent->EquipPreviousWeapon() : false;
}
