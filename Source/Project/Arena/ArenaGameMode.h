// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Arena/ArenaTypes.h"
#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "TimerManager.h"
#include "ArenaGameMode.generated.h"

class AArenaGameState;
class AArenaPlayerState;
class AActor;
class AController;
class UThreatAIComponent;

UCLASS(BlueprintType, Blueprintable)
class PROJECT_API AArenaGameMode : public AGameModeBase {
  GENERATED_BODY()

public:
  AArenaGameMode();

  virtual void BeginPlay() override;
  virtual void PostLogin(APlayerController *NewPlayer) override;
  virtual void RestartPlayer(AController *NewPlayer) override;

  UFUNCTION(BlueprintPure, Category = "Arena|Config")
  EArenaRunMode GetDefaultRunMode() const { return DefaultRunMode; }

  UFUNCTION(BlueprintPure, Category = "Arena|Config")
  int32 GetDefaultTotalWaves() const { return DefaultTotalWaves; }

  UFUNCTION(BlueprintPure, Category = "Arena|Run")
  FArenaRunState BuildInitialRunState() const;

  UFUNCTION(BlueprintCallable, Category = "Arena|Ready")
  bool SetPlayerReady(APlayerController *PlayerController, bool bReady);

  UFUNCTION(BlueprintPure, Category = "Arena|Ready")
  bool AreAllPlayersReady() const;

  UFUNCTION(BlueprintCallable, Category = "Arena|Run")
  bool StartArenaRun();

  UFUNCTION(BlueprintCallable, Category = "Arena|Rewards")
  bool ReportArenaThreatKilled(AActor *ThreatActor,
                               AController *KillerController,
                               int32 RewardCurrency);

  UFUNCTION(BlueprintCallable, Category = "Arena|Players")
  bool ReportArenaPlayerDied(AController *PlayerController,
                             AActor *PlayerActor);

  UFUNCTION(BlueprintCallable, Category = "Arena|Debug")
  bool InitializeArenaRun();

  UFUNCTION(BlueprintCallable, Category = "Arena|Debug")
  bool SetArenaPhaseForDebug(EArenaPhase NewPhase);

  UFUNCTION(BlueprintCallable, Category = "Arena|Debug")
  bool AdvanceArenaPhaseForDebug();

  UFUNCTION(BlueprintCallable, Category = "Arena|Debug")
  bool StartCombatWaveForDebug(int32 WaveNumber, int32 SpawnedThreats);

  UFUNCTION(BlueprintCallable, Category = "Arena|Debug")
  bool CompleteWaveForDebug(bool bSucceeded = true);

  UFUNCTION(BlueprintCallable, Category = "Arena|Debug")
  bool FinishArenaRunForDebug(EArenaRunResult Result);

  UFUNCTION(BlueprintCallable, Category = "Arena|Debug")
  void StartDebugAutoPhaseFlow();

  UFUNCTION(BlueprintCallable, Category = "Arena|Debug")
  void StopDebugAutoPhaseFlow();

protected:
  UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Arena|Config")
  EArenaRunMode DefaultRunMode = EArenaRunMode::Solo;

  UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Arena|Config",
            meta = (ClampMin = "1"))
  int32 DefaultTotalWaves = 6;

  UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Arena|Wave")
  TSubclassOf<AActor> ThreatClass;

  UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Arena|Threat AI")
  bool bAttachDefaultThreatAIOnSpawn = true;

  UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Arena|Threat AI",
            meta = (EditCondition = "bAttachDefaultThreatAIOnSpawn"))
  TSubclassOf<UThreatAIComponent> DefaultThreatAIComponentClass;

  UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Arena|Wave",
            meta = (ClampMin = "0"))
  int32 ThreatsPerWave = 5;

  UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Arena|Wave",
            meta = (ClampMin = "0.0"))
  float ArenaCountdownDuration = 3.0f;

  UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Arena|Wave",
            meta = (ClampMin = "0.0"))
  float ArenaIntermissionDuration = 5.0f;

  UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Arena|Ready",
            meta = (ClampMin = "1"))
  int32 MinimumReadyPlayersToStart = 1;

  UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Arena|Debug")
  bool bEnableDebugLogging = true;

  UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Arena|Debug")
  bool bAutoRunDebugPhaseFlow = false;

  UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Arena|Debug",
            meta = (ClampMin = "0.0"))
  float DebugPhaseInitialDelay = 4.0f;

  UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Arena|Debug",
            meta = (ClampMin = "0.1"))
  float DebugPhaseStepInterval = 2.0f;

  UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Arena|Debug",
            meta = (ClampMin = "0"))
  int32 DebugWaveThreatCount = 5;

  UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Arena|Debug")
  bool bSeedDebugPlayerStatsOnLogin = true;

  UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Arena|Debug",
            meta = (ClampMin = "0"))
  int32 DebugSeedSpendableCurrency = 50000;

  UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Arena|Debug")
  bool bRunDebugCurrencyTestOnFirstLogin = false;

  UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Arena|Debug")
  bool bOffsetDebugPlayerSpawnsInPIE = true;

  UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Arena|Debug",
            meta = (ClampMin = "0.0", EditCondition = "bOffsetDebugPlayerSpawnsInPIE"))
  float DebugPlayerSpawnSpacing = 240.0f;

  UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Arena|Spawns")
  FName LobbyPlayerSpawnTag = TEXT("LobbyPlayerSpawn");

  UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Arena|Spawns")
  FName ArenaPlayerSpawnTag = TEXT("ArenaPlayerSpawn");

  UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Arena|Spawns")
  FName ThreatSpawnPointTag = TEXT("ThreatSpawnPoint");

private:
  FTimerHandle DebugPhaseFlowTimerHandle;
  FTimerHandle ArenaCountdownTimerHandle;
  FTimerHandle ArenaIntermissionTimerHandle;
  int32 DebugPhaseFlowStep = 0;
  bool bArenaRunInitialized = false;
  bool bDebugCurrencyTestHasRun = false;

  AArenaGameState *GetArenaGameState() const;
  bool PublishArenaRunState(const FArenaRunState &NewRunState) const;
  void ApplyPhaseFlags(FArenaRunState &RunState) const;
  void ResetPlayerRunStats() const;
  void ResetPlayerReadyStates() const;
  void ResetPlayerAliveStates() const;
  void ClearPlayerArenaRuntimeRewards(AArenaPlayerState *PlayerState) const;
  void StartNextArenaWaveFromCountdown();
  void StartNextArenaWaveFromIntermission();
  void ScheduleArenaIntermissionAdvance(const FArenaRunState &RunState);
  bool StartArenaCombatWave(int32 WaveNumber, int32 RequestedThreats,
                            bool bSpawnThreats, const TCHAR *Context);
  int32 SpawnArenaThreatsForWave(int32 RequestedThreats);
  void EnsureThreatAIComponent(AActor *ThreatActor) const;
  bool ResolveThreatSpawnTransform(int32 SpawnIndex,
                                   FTransform &OutTransform) const;
  bool HasAliveArenaPlayers() const;
  void DisableArenaPlayerUntilWaveComplete(AController *PlayerController,
                                           AActor *PlayerActor) const;
  bool BeginSpectatingAliveTeammate(AController *PlayerController) const;
  int32 ReviveDeadArenaPlayersAtCheckpoint();
  void ClearArenaRuntimeTimers();
  bool ReturnArenaRunToLobby(const TCHAR *Reason);
  void TryStartArenaRunIfReady();
  void RunDebugCurrencyTestIfNeeded(APlayerController *NewPlayer);
  void SeedDebugPlayerStats(APlayerController *NewPlayer) const;
  void SeedDebugPlayerStatsForExistingPlayers();
  void ApplyDebugPlayerSpawnOffset(AController *NewPlayer) const;
  void RegisterPlayerRespawnCheckpoint(AController *PlayerController);
  bool RespawnArenaPlayerAtCheckpoint(AController *PlayerController,
                                      AActor *PlayerActor) const;
  void TeleportAllArenaPlayersToTaggedSpawns(FName SpawnTag,
                                             const TCHAR *Context);
  bool TeleportArenaPlayerToTaggedSpawn(AController *PlayerController,
                                        FName SpawnTag,
                                        const TCHAR *Context);
  bool ResolveTaggedPlayerSpawnTransform(AController *PlayerController,
                                         FName SpawnTag,
                                         FTransform &OutTransform) const;
  bool TeleportPlayerActor(AController *PlayerController, AActor *PlayerActor,
                           const FTransform &DestinationTransform,
                           const TCHAR *Context) const;
  int32 GetArenaPlayerIndex(AController *PlayerController) const;
  void HandleDebugPhaseFlowStep();
  void LogArenaRunState(const TCHAR *Context,
                        const FArenaRunState &RunState) const;

  TMap<TWeakObjectPtr<AArenaPlayerState>, FTransform>
      PlayerRespawnCheckpointTransforms;
};
