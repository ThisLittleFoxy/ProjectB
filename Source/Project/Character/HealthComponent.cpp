// Copyright Epic Games, Inc. All Rights Reserved.

#include "Character/HealthComponent.h"
#include "Character/CurrencyComponent.h"
#include "Engine/Engine.h"
#include "GameFramework/Actor.h"
#include "GameFramework/Controller.h"
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

  const bool bWasAliveBeforeDamage = IsAlive();
  const float AppliedDamage = ApplyDamage(Damage);
  if (AppliedDamage > 0.0f) {
    UE_LOG(LogProject, Verbose,
           TEXT("HealthComponent: '%s' took %.2f damage (%.2f / %.2f)"),
           *GetNameSafe(DamagedActor), AppliedDamage, CurrentHealth, MaxHealth);

    if (bShowDamageOnScreen && GEngine) {
      const FString ScreenMessage = FString::Printf(
          TEXT("%s: -%.1f HP (%.1f / %.1f)"), *GetNameSafe(DamagedActor),
          AppliedDamage, CurrentHealth, MaxHealth);
      GEngine->AddOnScreenDebugMessage(
          -1, FMath::Max(0.0f, DamageScreenMessageDuration),
          DamageScreenMessageColor, ScreenMessage);
    }

    if (bWasAliveBeforeDamage && !IsAlive()) {
      GrantDeathCurrencyReward(InstigatedBy, DamageCauser);
    }
  }
}

void UHealthComponent::GrantDeathCurrencyReward(AController *InstigatedBy,
                                                AActor *DamageCauser) {
  if (!bGrantCurrencyOnDeath || CurrencyRewardOnDeath <= 0) {
    return;
  }

  AActor *RewardReceiver = nullptr;
  if (InstigatedBy) {
    RewardReceiver = InstigatedBy->GetPawn();
  }

  if (!RewardReceiver && DamageCauser) {
    RewardReceiver = DamageCauser->GetInstigator();
  }

  AActor *OwnerActor = GetOwner();
  if (!RewardReceiver || RewardReceiver == OwnerActor) {
    return;
  }

  UCurrencyComponent *CurrencyComponent =
      RewardReceiver->FindComponentByClass<UCurrencyComponent>();
  if (!CurrencyComponent) {
    return;
  }

  const int32 AddedCurrency = CurrencyComponent->AddCurrency(CurrencyRewardOnDeath);
  if (AddedCurrency > 0) {
    UE_LOG(LogProject, Log, TEXT("HealthComponent: '%s' granted +%d currency to '%s'"),
           *GetNameSafe(OwnerActor), AddedCurrency, *GetNameSafe(RewardReceiver));
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
