// Copyright Epic Games, Inc. All Rights Reserved.

#include "Combat/HitZoneComponent.h"
#include "Project.h"

UHitZoneComponent::UHitZoneComponent() { PrimaryComponentTick.bCanEverTick = false; }

EHitZone UHitZoneComponent::ResolveHitZone(const FHitResult &Hit) const {
  const FName HitBoneName = Hit.BoneName;
  if (HitBoneName.IsNone()) {
    return EHitZone::Unknown;
  }

  for (const FHitZoneRule &Rule : ZoneRules) {
    if (Rule.BoneNames.Contains(HitBoneName)) {
      return Rule.Zone;
    }
  }

  UE_LOG(LogProject, Verbose,
         TEXT("HitZoneComponent: no rule for bone '%s' on '%s'"),
         *HitBoneName.ToString(), *GetNameSafe(GetOwner()));
  return EHitZone::Unknown;
}

float UHitZoneComponent::ResolveDamageMultiplier(const FHitResult &Hit) const {
  const FName HitBoneName = Hit.BoneName;
  if (HitBoneName.IsNone()) {
    return DefaultDamageMultiplier;
  }

  for (const FHitZoneRule &Rule : ZoneRules) {
    if (Rule.BoneNames.Contains(HitBoneName)) {
      return FMath::Max(0.0f, Rule.DamageMultiplier);
    }
  }

  UE_LOG(LogProject, Verbose,
         TEXT("HitZoneComponent: using default multiplier for bone '%s' on '%s'"),
         *HitBoneName.ToString(), *GetNameSafe(GetOwner()));
  return DefaultDamageMultiplier;
}
