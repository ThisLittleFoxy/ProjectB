// Copyright Epic Games, Inc. All Rights Reserved.

#include "UI/HUD/CrosshairWidgetBase.h"
#include "Character/HealthComponent.h"
#include "Combat/CombatComponent.h"
#include "GameFramework/Pawn.h"

int32 UCrosshairWidgetBase::GetAmmoMagazine() const {
  if (const UCombatComponent *CombatComp = ResolveCombatComponent()) {
    return CombatComp->GetAmmoInMagazine();
  }

  return 0;
}

int32 UCrosshairWidgetBase::GetAmmoReserve() const {
  if (const UCombatComponent *CombatComp = ResolveCombatComponent()) {
    return CombatComp->GetAmmoInReserve();
  }

  return 0;
}

float UCrosshairWidgetBase::GetCurrentHealth() const {
  if (const UHealthComponent *HealthComp = ResolveHealthComponent()) {
    return HealthComp->GetCurrentHealth();
  }

  return 0.0f;
}

float UCrosshairWidgetBase::GetMaxHealth() const {
  if (const UHealthComponent *HealthComp = ResolveHealthComponent()) {
    return HealthComp->GetMaxHealth();
  }

  return 0.0f;
}

float UCrosshairWidgetBase::GetHealthPercent() const {
  if (const UHealthComponent *HealthComp = ResolveHealthComponent()) {
    return HealthComp->GetHealthPercent();
  }

  return 0.0f;
}

bool UCrosshairWidgetBase::IsAlive() const {
  if (const UHealthComponent *HealthComp = ResolveHealthComponent()) {
    return HealthComp->IsAlive();
  }

  return false;
}

bool UCrosshairWidgetBase::IsScoping() const {
  if (const UCombatComponent *CombatComp = ResolveCombatComponent()) {
    return CombatComp->IsScoping();
  }

  return false;
}

bool UCrosshairWidgetBase::ShouldShowCrosshair() const {
  const UCombatComponent *CombatComp = ResolveCombatComponent();
  if (!CombatComp) {
    return false;
  }

  return CombatComp->GetCurrentWeapon() != nullptr && !CombatComp->IsScoping();
}

void UCrosshairWidgetBase::RefreshCachedComponents() const {
  APawn *OwningPawn = GetOwningPlayerPawn();
  if (!OwningPawn) {
    CachedOwningPawn.Reset();
    CachedCombatComponent.Reset();
    CachedHealthComponent.Reset();
    return;
  }

  if (CachedOwningPawn.Get() != OwningPawn) {
    CachedOwningPawn = OwningPawn;
    CachedCombatComponent = OwningPawn->FindComponentByClass<UCombatComponent>();
    CachedHealthComponent = OwningPawn->FindComponentByClass<UHealthComponent>();
    return;
  }

  if (!CachedCombatComponent.IsValid()) {
    CachedCombatComponent = OwningPawn->FindComponentByClass<UCombatComponent>();
  }

  if (!CachedHealthComponent.IsValid()) {
    CachedHealthComponent = OwningPawn->FindComponentByClass<UHealthComponent>();
  }
}

UCombatComponent *UCrosshairWidgetBase::ResolveCombatComponent() const {
  RefreshCachedComponents();

  return CachedCombatComponent.Get();
}

UHealthComponent *UCrosshairWidgetBase::ResolveHealthComponent() const {
  RefreshCachedComponents();

  return CachedHealthComponent.Get();
}
