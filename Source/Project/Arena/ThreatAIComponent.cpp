// Copyright Epic Games, Inc. All Rights Reserved.

#include "Arena/ThreatAIComponent.h"
#include "Arena/ArenaPlayerState.h"
#include "Character/HealthComponent.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "GameFramework/Controller.h"
#include "GameFramework/DamageType.h"
#include "GameFramework/GameStateBase.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerState.h"
#include "Kismet/GameplayStatics.h"
#include "Project.h"

UThreatAIComponent::UThreatAIComponent() {
  PrimaryComponentTick.bCanEverTick = true;
  PrimaryComponentTick.bStartWithTickEnabled = true;
  TimeSinceLastAttack = AttackInterval;
}

void UThreatAIComponent::BeginPlay() {
  Super::BeginPlay();

  if (AActor *OwnerActor = GetOwner()) {
    if (!OwnerActor->HasAuthority()) {
      SetComponentTickEnabled(false);
      return;
    }
  }

  TimeUntilTargetRefresh = 0.0f;
  TimeSinceLastAttack = AttackInterval;
}

void UThreatAIComponent::TickComponent(
    float DeltaTime, ELevelTick TickType,
    FActorComponentTickFunction *ThisTickFunction) {
  Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

  AActor *OwnerActor = GetOwner();
  if (!bThreatAIEnabled || !OwnerActor || !OwnerActor->HasAuthority() ||
      OwnerActor->IsActorBeingDestroyed() || OwnerActor->IsHidden()) {
    return;
  }

  TimeSinceLastAttack += DeltaTime;
  TimeUntilTargetRefresh -= DeltaTime;

  if (TimeUntilTargetRefresh <= 0.0f ||
      !IsValidTargetPawn(CurrentTargetPawn.Get())) {
    RefreshTarget();
  }

  APawn *TargetPawn = CurrentTargetPawn.Get();
  if (!IsValidTargetPawn(TargetPawn)) {
    return;
  }

  MoveTowardTarget(TargetPawn, DeltaTime);
  TryAttackTarget(TargetPawn);
}

void UThreatAIComponent::SetThreatAIEnabled(bool bNewEnabled) {
  bThreatAIEnabled = bNewEnabled;
  SetComponentTickEnabled(bThreatAIEnabled);
}

void UThreatAIComponent::RefreshTarget() {
  CurrentTargetPawn = FindBestTargetPawn();
  TimeUntilTargetRefresh = FMath::Max(0.01f, TargetRefreshInterval);
}

APawn *UThreatAIComponent::FindBestTargetPawn() const {
  const AActor *OwnerActor = GetOwner();
  const UWorld *World = GetWorld();
  const AGameStateBase *GameState = World ? World->GetGameState() : nullptr;
  if (!OwnerActor || !GameState) {
    return nullptr;
  }

  const FVector OwnerLocation = OwnerActor->GetActorLocation();
  const float MaxDistanceSquared =
      TargetAcquireRadius > 0.0f
          ? FMath::Square(TargetAcquireRadius)
          : TNumericLimits<float>::Max();

  APawn *BestPawn = nullptr;
  float BestDistanceSquared = MaxDistanceSquared;

  for (APlayerState *PlayerState : GameState->PlayerArray) {
    const AArenaPlayerState *ArenaPlayerState =
        Cast<AArenaPlayerState>(PlayerState);
    if (!ArenaPlayerState || !ArenaPlayerState->IsArenaAlive()) {
      continue;
    }

    const AController *Controller =
        Cast<AController>(ArenaPlayerState->GetOwner());
    APawn *CandidatePawn = Controller ? Controller->GetPawn() : nullptr;
    if (!IsValidTargetPawn(CandidatePawn)) {
      continue;
    }

    const float DistanceSquared =
        FVector::DistSquared2D(OwnerLocation, CandidatePawn->GetActorLocation());
    if (DistanceSquared <= BestDistanceSquared) {
      BestPawn = CandidatePawn;
      BestDistanceSquared = DistanceSquared;
    }
  }

  return BestPawn;
}

bool UThreatAIComponent::IsValidTargetPawn(const APawn *CandidatePawn) const {
  if (!CandidatePawn || CandidatePawn->IsActorBeingDestroyed() ||
      CandidatePawn->IsHidden() || !CandidatePawn->CanBeDamaged()) {
    return false;
  }

  const AArenaPlayerState *ArenaPlayerState =
      CandidatePawn->GetPlayerState<AArenaPlayerState>();
  if (!ArenaPlayerState || !ArenaPlayerState->IsArenaAlive()) {
    return false;
  }

  const UHealthComponent *HealthComponent =
      CandidatePawn->FindComponentByClass<UHealthComponent>();
  return HealthComponent && HealthComponent->IsAlive();
}

void UThreatAIComponent::MoveTowardTarget(APawn *TargetPawn,
                                          float DeltaTime) const {
  AActor *OwnerActor = GetOwner();
  if (!OwnerActor || !TargetPawn || MoveSpeed <= 0.0f || DeltaTime <= 0.0f) {
    return;
  }

  FVector OwnerLocation = OwnerActor->GetActorLocation();
  const FVector TargetLocation = TargetPawn->GetActorLocation();
  FVector ToTarget = TargetLocation - OwnerLocation;
  ToTarget.Z = 0.0f;

  const float Distance = ToTarget.Size();
  if (Distance <= KINDA_SMALL_NUMBER) {
    return;
  }

  const FVector Direction = ToTarget / Distance;

  if (bFaceTarget) {
    FRotator TargetRotation = Direction.Rotation();
    TargetRotation.Pitch = 0.0f;
    TargetRotation.Roll = 0.0f;
    OwnerActor->SetActorRotation(TargetRotation);
  }

  if (Distance <= StopDistance) {
    return;
  }

  const float MoveDistance = FMath::Min(MoveSpeed * DeltaTime, Distance);
  const FVector NewLocation = OwnerLocation + Direction * MoveDistance;
  OwnerActor->SetActorLocation(NewLocation, bUseSweptMovement);
}

void UThreatAIComponent::TryAttackTarget(APawn *TargetPawn) {
  const AActor *OwnerActor = GetOwner();
  if (!OwnerActor || !TargetPawn || AttackDamage <= 0.0f ||
      TimeSinceLastAttack < FMath::Max(0.01f, AttackInterval)) {
    return;
  }

  const float Distance =
      FVector::Dist2D(OwnerActor->GetActorLocation(), TargetPawn->GetActorLocation());
  if (Distance > AttackRange) {
    return;
  }

  TimeSinceLastAttack = 0.0f;
  UGameplayStatics::ApplyDamage(TargetPawn, AttackDamage, nullptr,
                                const_cast<AActor *>(OwnerActor),
                                UDamageType::StaticClass());

  if (bLogAttacks) {
    UE_LOG(LogProject, Display,
           TEXT("Arena threat attacked player. Threat=%s Target=%s Damage=%.2f Distance=%.1f"),
           *GetNameSafe(OwnerActor), *GetNameSafe(TargetPawn), AttackDamage,
           Distance);
  }
}
