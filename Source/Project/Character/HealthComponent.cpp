// Copyright Epic Games, Inc. All Rights Reserved.

#include "Character/HealthComponent.h"
#include "GameFramework/Actor.h"
#include "Math/UnrealMathUtility.h"
#include "Project.h"

UHealthComponent::UHealthComponent() {
  PrimaryComponentTick.bCanEverTick = false;
}

void UHealthComponent::BeginPlay() {
  Super::BeginPlay();

  MaxHealth = FMath::Max(MaxHealth, 1.0f);

  if (bInitializeFromMaxHealthOnBeginPlay) {
    CurrentHealth = MaxHealth;
  } else {
    CurrentHealth = FMath::Clamp(CurrentHealth, 0.0f, MaxHealth);
  }

  bOutOfHealthNotified = CurrentHealth <= 0.0f;

  if (AActor *OwnerActor = GetOwner()) {
    OwnerActor->OnTakeAnyDamage.AddDynamic(
        this, &UHealthComponent::HandleOwnerTakeAnyDamage);
  }

  OnHealthChanged.Broadcast(this, CurrentHealth, MaxHealth, 0.0f);
}

float UHealthComponent::ApplyDamage(float DamageAmount) {
  if (DamageAmount <= 0.0f || CurrentHealth <= 0.0f) {
    return 0.0f;
  }

  const float DeltaHealth = SetHealth(CurrentHealth - DamageAmount);
  return FMath::Max(0.0f, -DeltaHealth);
}

float UHealthComponent::ApplyHealing(float HealAmount) {
  if (HealAmount <= 0.0f || CurrentHealth >= MaxHealth) {
    return 0.0f;
  }

  const float DeltaHealth = SetHealth(CurrentHealth + HealAmount);
  return FMath::Max(0.0f, DeltaHealth);
}

void UHealthComponent::RestoreFullHealth() { SetHealth(MaxHealth); }

float UHealthComponent::GetHealthPercent() const {
  if (MaxHealth <= KINDA_SMALL_NUMBER) {
    return 0.0f;
  }

  return CurrentHealth / MaxHealth;
}

void UHealthComponent::HandleOwnerTakeAnyDamage(AActor *DamagedActor, float Damage,
                                                const UDamageType *DamageType,
                                                AController *InstigatedBy,
                                                AActor *DamageCauser) {
  if (!DamagedActor || DamagedActor != GetOwner() || Damage <= 0.0f) {
    return;
  }

  if (!DamagedActor->CanBeDamaged()) {
    return;
  }

  const float AppliedDamage = ApplyDamage(Damage);
  if (AppliedDamage > 0.0f) {
    UE_LOG(LogProject, Verbose,
           TEXT("HealthComponent: '%s' took %.2f damage (%.2f / %.2f)"),
           *GetNameSafe(DamagedActor), AppliedDamage, CurrentHealth, MaxHealth);
  }
}

void UHealthComponent::HandleDeath() {
  AActor *OwnerActor = GetOwner();
  if (!OwnerActor) {
    return;
  }

  OwnerActor->SetCanBeDamaged(false);

  if (bHideActorOnDeath) {
    OwnerActor->SetActorEnableCollision(false);
    OwnerActor->SetActorHiddenInGame(true);
  }

  if (!bDestroyOwnerOnDeath) {
    return;
  }

  const float LifeSpan = FMath::Max(0.0f, DestroyDelayOnDeath);
  if (LifeSpan <= KINDA_SMALL_NUMBER) {
    OwnerActor->Destroy();
    return;
  }

  OwnerActor->SetLifeSpan(LifeSpan);
}

float UHealthComponent::SetHealth(float NewHealth) {
  const float PreviousHealth = CurrentHealth;
  CurrentHealth = FMath::Clamp(NewHealth, 0.0f, MaxHealth);

  const float DeltaHealth = CurrentHealth - PreviousHealth;
  if (!FMath::IsNearlyZero(DeltaHealth)) {
    OnHealthChanged.Broadcast(this, CurrentHealth, MaxHealth, DeltaHealth);
  }

  const bool bIsOutOfHealth = CurrentHealth <= 0.0f;
  if (bIsOutOfHealth && !bOutOfHealthNotified) {
    bOutOfHealthNotified = true;
    OnOutOfHealth.Broadcast(this);
    HandleDeath();
  } else if (!bIsOutOfHealth) {
    bOutOfHealthNotified = false;
  }

  return DeltaHealth;
}
