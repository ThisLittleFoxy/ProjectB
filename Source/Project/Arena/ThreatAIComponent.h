// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Components/ActorComponent.h"
#include "CoreMinimal.h"
#include "ThreatAIComponent.generated.h"

class APawn;

/**
 * Minimal server-authoritative threat brain for early arena waves.
 *
 * The component is intentionally actor-based instead of pawn-only so existing
 * Blueprint dummy actors can move and attack without being reparented.
 */
UCLASS(ClassGroup = (Arena), meta = (BlueprintSpawnableComponent))
class PROJECT_API UThreatAIComponent : public UActorComponent {
  GENERATED_BODY()

public:
  UThreatAIComponent();

  virtual void BeginPlay() override;
  virtual void TickComponent(float DeltaTime, ELevelTick TickType,
                             FActorComponentTickFunction *ThisTickFunction)
      override;

  UFUNCTION(BlueprintCallable, Category = "Arena|Threat AI")
  void SetThreatAIEnabled(bool bNewEnabled);

  UFUNCTION(BlueprintPure, Category = "Arena|Threat AI")
  bool IsThreatAIEnabled() const { return bThreatAIEnabled; }

  UFUNCTION(BlueprintPure, Category = "Arena|Threat AI")
  APawn *GetCurrentTargetPawn() const { return CurrentTargetPawn.Get(); }

protected:
  UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Arena|Threat AI")
  bool bThreatAIEnabled = true;

  UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Arena|Threat AI",
            meta = (ClampMin = "0.0"))
  float TargetRefreshInterval = 0.25f;

  UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Arena|Threat AI",
            meta = (ClampMin = "0.0"))
  float TargetAcquireRadius = 6000.0f;

  UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Arena|Threat AI",
            meta = (ClampMin = "0.0"))
  float MoveSpeed = 260.0f;

  UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Arena|Threat AI",
            meta = (ClampMin = "0.0"))
  float StopDistance = 115.0f;

  UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Arena|Threat AI",
            meta = (ClampMin = "0.0"))
  float AttackRange = 165.0f;

  UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Arena|Threat AI",
            meta = (ClampMin = "0.0"))
  float AttackDamage = 20.0f;

  UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Arena|Threat AI",
            meta = (ClampMin = "0.01"))
  float AttackInterval = 1.0f;

  UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Arena|Threat AI")
  bool bUseSweptMovement = true;

  UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Arena|Threat AI")
  bool bFaceTarget = true;

  UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Arena|Threat AI|Debug")
  bool bLogAttacks = true;

private:
  void RefreshTarget();
  APawn *FindBestTargetPawn() const;
  bool IsValidTargetPawn(const APawn *CandidatePawn) const;
  void MoveTowardTarget(APawn *TargetPawn, float DeltaTime) const;
  void TryAttackTarget(APawn *TargetPawn);

  UPROPERTY(Transient)
  TObjectPtr<APawn> CurrentTargetPawn;

  float TimeUntilTargetRefresh = 0.0f;
  float TimeSinceLastAttack = 0.0f;
};
