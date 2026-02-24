// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Components/ActorComponent.h"
#include "Engine/EngineTypes.h"
#include "HitZoneComponent.generated.h"

UENUM(BlueprintType)
enum class EHitZone : uint8 {
  Unknown UMETA(DisplayName = "Unknown"),
  Head UMETA(DisplayName = "Head"),
  Torso UMETA(DisplayName = "Torso"),
  Limb UMETA(DisplayName = "Limb")
};

USTRUCT(BlueprintType)
struct FHitZoneRule {
  GENERATED_BODY()

  UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "HitZone")
  EHitZone Zone = EHitZone::Unknown;

  UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "HitZone")
  TArray<FName> BoneNames;

  UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "HitZone|Deprecated",
            meta = (ClampMin = "0.0", DeprecatedProperty,
                    DeprecationMessage = "Damage multipliers are now defined per weapon"))
  float DamageMultiplier = 1.0f;
};

UCLASS(ClassGroup = (Combat), meta = (BlueprintSpawnableComponent))
class PROJECT_API UHitZoneComponent : public UActorComponent {
  GENERATED_BODY()

public:
  UHitZoneComponent();

  UFUNCTION(BlueprintPure, Category = "HitZone")
  EHitZone ResolveHitZone(const FHitResult &Hit) const;

  UFUNCTION(BlueprintPure, Category = "HitZone",
            meta = (DeprecatedFunction,
                    DeprecationMessage = "Use ResolveHitZone and per-weapon zone multipliers"))
  float ResolveDamageMultiplier(const FHitResult &Hit) const;

protected:
  UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "HitZone")
  TArray<FHitZoneRule> ZoneRules;

  UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "HitZone|Deprecated",
            meta = (ClampMin = "0.0", DeprecatedProperty,
                    DeprecationMessage = "Damage multipliers are now defined per weapon"))
  float DefaultDamageMultiplier = 1.0f;
};
