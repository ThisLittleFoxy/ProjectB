// Copyright Epic Games, Inc. All Rights Reserved.

#include "Character/HealthComponent.h"
#include "Arena/ArenaGameMode.h"
#include "Arena/ArenaGameState.h"
#include "Arena/ArenaPlayerState.h"
#include "Character/CurrencyComponent.h"
#include "Combat/WeaponBase.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "GameFramework/Controller.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Math/UnrealMathUtility.h"
#include "Net/UnrealNetwork.h"
#include "Project.h"

namespace {
bool IsPlayerPawn(const AActor *Actor) {
  const APawn *Pawn = Cast<APawn>(Actor);
  return Pawn && Pawn->GetPlayerState();
}

bool IsPlayerDamageInstigator(const AController *InstigatedBy,
                              const AActor *DamageCauser) {
  if (Cast<APlayerController>(InstigatedBy)) {
    return true;
  }

  const APawn *CauserInstigator =
      DamageCauser ? DamageCauser->GetInstigator() : nullptr;
  return CauserInstigator && CauserInstigator->GetPlayerState();
}

bool DamageCauserBypassesFriendlyFireRules(const AActor *DamageCauser) {
  const AWeaponBase *Weapon = Cast<AWeaponBase>(DamageCauser);
  if (!Weapon && DamageCauser) {
    Weapon = Cast<AWeaponBase>(DamageCauser->GetOwner());
  }
  return Weapon && Weapon->BypassesFriendlyFireRules();
}
} // namespace

UHealthComponent::UHealthComponent() {
  PrimaryComponentTick.bCanEverTick = false;
  SetIsReplicatedByDefault(true);
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

void UHealthComponent::GetLifetimeReplicatedProps(
    TArray<FLifetimeProperty> &OutLifetimeProps) const {
  Super::GetLifetimeReplicatedProps(OutLifetimeProps);

  DOREPLIFETIME(UHealthComponent, MaxHealth);
  DOREPLIFETIME(UHealthComponent, CurrentHealth);
}

float UHealthComponent::ApplyDamage(float DamageAmount) {
  if (DamageAmount <= 0.0f || CurrentHealth <= 0.0f) {
    return 0.0f;
  }

  if (const UWorld *World = GetWorld(); World && World->GetNetMode() == NM_Client) {
    UE_LOG(LogProject, Verbose,
           TEXT("HealthComponent: rejected client-local damage. Actor=%s Damage=%.2f"),
           *GetNameSafe(GetOwner()), DamageAmount);
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

void UHealthComponent::RestoreHealthFromSave(float SavedHealth) {
  AActor *OwnerActor = GetOwner();
  if (OwnerActor && SavedHealth > 0.0f) {
    OwnerActor->SetActorHiddenInGame(false);
    OwnerActor->SetActorEnableCollision(true);
    OwnerActor->SetCanBeDamaged(true);
  }

  SetHealth(SavedHealth);
}

float UHealthComponent::GetHealthPercent() const {
  if (MaxHealth <= KINDA_SMALL_NUMBER) {
    return 0.0f;
  }

  return CurrentHealth / MaxHealth;
}

void UHealthComponent::OnRep_CurrentHealth(float PreviousHealth) {
  const float DeltaHealth = CurrentHealth - PreviousHealth;
  OnHealthChanged.Broadcast(this, CurrentHealth, MaxHealth, DeltaHealth);

  const bool bIsOutOfHealth = CurrentHealth <= 0.0f;
  if (bIsOutOfHealth && !bOutOfHealthNotified) {
    bOutOfHealthNotified = true;
    OnOutOfHealth.Broadcast(this);
  } else if (!bIsOutOfHealth) {
    bOutOfHealthNotified = false;
  }
}

void UHealthComponent::OnRep_MaxHealth(float PreviousMaxHealth) {
  if (!FMath::IsNearlyEqual(MaxHealth, PreviousMaxHealth)) {
    OnHealthChanged.Broadcast(this, CurrentHealth, MaxHealth, 0.0f);
  }
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

  const bool bFriendlyPlayerDamage =
      IsPlayerPawn(DamagedActor) &&
      IsPlayerDamageInstigator(InstigatedBy, DamageCauser);
  if (bFriendlyPlayerDamage && !bAcceptFriendlyFireDamage &&
      !DamageCauserBypassesFriendlyFireRules(DamageCauser)) {
    UE_LOG(LogProject, Verbose,
           TEXT("HealthComponent: rejected friendly fire damage. Target=%s Instigator=%s Causer=%s Damage=%.2f"),
           *GetNameSafe(DamagedActor), *GetNameSafe(InstigatedBy),
           *GetNameSafe(DamageCauser), Damage);
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

  AController *RewardController = InstigatedBy;
  if (!RewardController && DamageCauser) {
    RewardController = DamageCauser->GetInstigatorController();
  }

  UWorld *World = GetWorld();
  if (World && World->GetGameState<AArenaGameState>()) {
    if (const APawn *OwnerPawn = Cast<APawn>(GetOwner())) {
      if (OwnerPawn->GetPlayerState<AArenaPlayerState>()) {
        return;
      }
    }

    if (AArenaGameMode *ArenaGameMode = World->GetAuthGameMode<AArenaGameMode>()) {
      ArenaGameMode->ReportArenaThreatKilled(GetOwner(), RewardController,
                                             CurrencyRewardOnDeath);
    }
    return;
  }

  AActor *RewardReceiver = nullptr;
  if (RewardController) {
    RewardReceiver = RewardController->GetPawn();
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

  if (HandleArenaPlayerDeath(OwnerActor)) {
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

bool UHealthComponent::HandleArenaPlayerDeath(AActor *OwnerActor) {
  UWorld *World = GetWorld();
  if (!World || !World->GetGameState<AArenaGameState>() || !OwnerActor) {
    return false;
  }

  APawn *OwnerPawn = Cast<APawn>(OwnerActor);
  AArenaPlayerState *ArenaPlayerState =
      OwnerPawn ? OwnerPawn->GetPlayerState<AArenaPlayerState>() : nullptr;
  if (!OwnerPawn || !ArenaPlayerState) {
    return false;
  }

  OwnerActor->SetCanBeDamaged(false);

  if (AArenaGameMode *ArenaGameMode = World->GetAuthGameMode<AArenaGameMode>()) {
    ArenaGameMode->ReportArenaPlayerDied(OwnerPawn->GetController(),
                                         OwnerActor);
  }

  if (!ArenaPlayerState->IsArenaAlive()) {
    SetHealth(MaxHealth);
    return true;
  }

  OwnerActor->SetCanBeDamaged(true);
  SetHealth(MaxHealth);
  return true;
}

float UHealthComponent::SetHealth(float NewHealth) {
  const float PreviousHealth = CurrentHealth;
  CurrentHealth = FMath::Clamp(NewHealth, 0.0f, MaxHealth);

  const float DeltaHealth = CurrentHealth - PreviousHealth;
  if (!FMath::IsNearlyZero(DeltaHealth)) {
    OnHealthChanged.Broadcast(this, CurrentHealth, MaxHealth, DeltaHealth);
    if (AActor *OwnerActor = GetOwner();
        OwnerActor && OwnerActor->HasAuthority()) {
      OwnerActor->ForceNetUpdate();
    }
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
