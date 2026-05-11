// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Arena/ArenaTypes.h"
#include "CoreMinimal.h"
#include "GameFramework/GameStateBase.h"
#include "ArenaGameState.generated.h"

UCLASS(BlueprintType, Blueprintable)
class PROJECT_API AArenaGameState : public AGameStateBase {
  GENERATED_BODY()

public:
  AArenaGameState();

  virtual void GetLifetimeReplicatedProps(
      TArray<FLifetimeProperty> &OutLifetimeProps) const override;

  void SetArenaRunState(const FArenaRunState &NewRunState);

  UFUNCTION(BlueprintPure, Category = "Arena|Run")
  FArenaRunState GetArenaRunState() const { return ArenaRunState; }

  UFUNCTION(BlueprintPure, Category = "Arena|Run")
  EArenaRunMode GetRunMode() const { return ArenaRunState.RunMode; }

  UFUNCTION(BlueprintPure, Category = "Arena|Run")
  EArenaPhase GetArenaPhase() const { return ArenaRunState.Phase; }

  UFUNCTION(BlueprintPure, Category = "Arena|Run")
  EArenaRunResult GetRunResult() const { return ArenaRunState.Result; }

  UFUNCTION(BlueprintPure, Category = "Arena|Run")
  float GetElapsedSeconds() const { return ArenaRunState.ElapsedSeconds; }

  UFUNCTION(BlueprintPure, Category = "Arena|Run")
  bool IsRunActive() const { return ArenaRunState.bRunActive; }

  UFUNCTION(BlueprintPure, Category = "Arena|Run")
  bool IsWaveActive() const { return ArenaRunState.bWaveActive; }

  UFUNCTION(BlueprintPure, Category = "Arena|Run")
  bool IsRunCompleted() const { return ArenaRunState.bRunCompleted; }

  UFUNCTION(BlueprintPure, Category = "Arena|Run")
  bool IsRunFailed() const { return ArenaRunState.bRunFailed; }

  UFUNCTION(BlueprintPure, Category = "Arena|Wave")
  FArenaWaveState GetArenaWaveState() const { return ArenaRunState.WaveState; }

  UFUNCTION(BlueprintPure, Category = "Arena|Wave")
  int32 GetCurrentWave() const { return ArenaRunState.WaveState.CurrentWave; }

  UFUNCTION(BlueprintPure, Category = "Arena|Wave")
  int32 GetTotalWaves() const { return ArenaRunState.WaveState.TotalWaves; }

  UFUNCTION(BlueprintPure, Category = "Arena|Wave")
  int32 GetAliveThreats() const { return ArenaRunState.WaveState.AliveThreats; }

  UFUNCTION(BlueprintPure, Category = "Arena|Wave")
  int32 GetSpawnedThreats() const { return ArenaRunState.WaveState.SpawnedThreats; }

  UFUNCTION(BlueprintImplementableEvent, Category = "Arena|Run",
            meta = (DisplayName = "On Arena Run State Changed"))
  void BP_OnArenaRunStateChanged(const FArenaRunState &NewRunState);

protected:
  UPROPERTY(ReplicatedUsing = OnRep_ArenaRunState, BlueprintReadOnly,
            Category = "Arena|Run", meta = (AllowPrivateAccess = "true"))
  FArenaRunState ArenaRunState;

private:
  UFUNCTION()
  void OnRep_ArenaRunState();
};
