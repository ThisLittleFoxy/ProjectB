// Copyright Epic Games, Inc. All Rights Reserved.

#include "Arena/ArenaGameMode.h"
#include "Arena/Debug/ArenaRuntimeCurrencyDebugTest.h"
#include "Arena/ArenaGameState.h"
#include "Arena/ArenaPlayerState.h"
#include "Arena/ThreatAIComponent.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Controller.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerState.h"
#include "Kismet/GameplayStatics.h"

DEFINE_LOG_CATEGORY_STATIC(LogArenaGameMode, Log, All);

AArenaGameMode::AArenaGameMode() {
  GameStateClass = AArenaGameState::StaticClass();
  PlayerStateClass = AArenaPlayerState::StaticClass();
  DefaultThreatAIComponentClass = UThreatAIComponent::StaticClass();
}

void AArenaGameMode::BeginPlay() {
  Super::BeginPlay();

  InitializeArenaRun();
  bArenaRunInitialized = true;
  SeedDebugPlayerStatsForExistingPlayers();

  if (bAutoRunDebugPhaseFlow) {
    StartDebugAutoPhaseFlow();
  }
}

void AArenaGameMode::PostLogin(APlayerController *NewPlayer) {
  Super::PostLogin(NewPlayer);

  if (AArenaPlayerState *ArenaPlayerState =
          NewPlayer ? NewPlayer->GetPlayerState<AArenaPlayerState>() : nullptr) {
    ArenaPlayerState->ResetArenaRunStats();
    ArenaPlayerState->SetArenaReady(false);
    ArenaPlayerState->SetArenaAlive(true);
  }

  if (bArenaRunInitialized) {
    RunDebugCurrencyTestIfNeeded(NewPlayer);
    SeedDebugPlayerStats(NewPlayer);
  }
}

void AArenaGameMode::RestartPlayer(AController *NewPlayer) {
  Super::RestartPlayer(NewPlayer);

  if (APlayerController *PlayerController = Cast<APlayerController>(NewPlayer)) {
    if (APawn *Pawn = PlayerController->GetPawn()) {
      Pawn->SetOwner(PlayerController);
      if (!Pawn->GetIsReplicated()) {
        Pawn->SetReplicates(true);
      }
      Pawn->SetReplicateMovement(true);
      if (!PlayerController->IsLocalController()) {
        Pawn->SetAutonomousProxy(true);
      }
      Pawn->ForceNetUpdate();
      PlayerController->ClientRestart(Pawn);

      UE_LOG(LogArenaGameMode, Display,
             TEXT("Restarted arena player. Controller=%s Pawn=%s Owner=%s Role=%d RemoteRole=%d NetMode=%s"),
             *GetNameSafe(PlayerController), *GetNameSafe(Pawn),
             *GetNameSafe(Pawn->GetOwner()),
             static_cast<int32>(Pawn->GetLocalRole()),
             static_cast<int32>(Pawn->GetRemoteRole()),
             ProjectArenaDebug::LexToString(GetNetMode()));
    }
  }

  ApplyDebugPlayerSpawnOffset(NewPlayer);
  RegisterPlayerRespawnCheckpoint(NewPlayer);
}

FArenaRunState AArenaGameMode::BuildInitialRunState() const {
  FArenaRunState InitialState;
  InitialState.RunMode = DefaultRunMode;
  InitialState.Phase = EArenaPhase::Lobby;
  InitialState.Result = EArenaRunResult::None;
  InitialState.ElapsedSeconds = 0.0f;
  InitialState.bRunActive = false;
  InitialState.bWaveActive = false;
  InitialState.bRunCompleted = false;
  InitialState.bRunFailed = false;
  InitialState.WaveState.CurrentWave = 0;
  InitialState.WaveState.TotalWaves = FMath::Max(1, DefaultTotalWaves);
  InitialState.WaveState.AliveThreats = 0;
  InitialState.WaveState.SpawnedThreats = 0;
  InitialState.WaveState.bWaveCompleted = false;
  InitialState.WaveState.bWaveFailed = false;
  return InitialState;
}

bool AArenaGameMode::InitializeArenaRun() {
  ClearArenaRuntimeTimers();
  ResetPlayerRunStats();
  ResetPlayerReadyStates();
  ResetPlayerAliveStates();
  return PublishArenaRunState(BuildInitialRunState());
}

bool AArenaGameMode::SetPlayerReady(APlayerController *PlayerController,
                                    bool bReady) {
  AArenaPlayerState *ArenaPlayerState =
      PlayerController ? PlayerController->GetPlayerState<AArenaPlayerState>()
                       : nullptr;
  if (!ArenaPlayerState) {
    return false;
  }

  ArenaPlayerState->SetArenaReady(bReady);

  if (bEnableDebugLogging) {
    UE_LOG(LogArenaGameMode, Display,
           TEXT("SetPlayerReady NetMode=%s Player=%s Ready=%s"),
           ProjectArenaDebug::LexToString(GetNetMode()),
           *ArenaPlayerState->GetPlayerName(),
           bReady ? TEXT("true") : TEXT("false"));
  }

  TryStartArenaRunIfReady();
  return true;
}

bool AArenaGameMode::AreAllPlayersReady() const {
  const AGameStateBase *BaseGameState = GameState;
  if (!BaseGameState) {
    return false;
  }

  int32 ReadyPlayers = 0;
  int32 ArenaPlayers = 0;
  for (const APlayerState *PlayerState : BaseGameState->PlayerArray) {
    const AArenaPlayerState *ArenaPlayerState =
        Cast<AArenaPlayerState>(PlayerState);
    if (!ArenaPlayerState) {
      continue;
    }

    ++ArenaPlayers;
    if (ArenaPlayerState->IsArenaReady()) {
      ++ReadyPlayers;
    }
  }

  const int32 RequiredPlayers = FMath::Max(1, MinimumReadyPlayersToStart);
  return ArenaPlayers >= RequiredPlayers && ReadyPlayers == ArenaPlayers;
}

bool AArenaGameMode::StartArenaRun() {
  AArenaGameState *ArenaGameState = GetArenaGameState();
  if (!ArenaGameState) {
    return false;
  }

  FArenaRunState NewRunState = ArenaGameState->GetArenaRunState();
  if (NewRunState.WaveState.TotalWaves <= 0) {
    NewRunState = BuildInitialRunState();
  }

  if (NewRunState.Phase != EArenaPhase::Lobby) {
    if (bEnableDebugLogging) {
      UE_LOG(LogArenaGameMode, Display,
             TEXT("StartArenaRun rejected. CurrentPhase=%s NetMode=%s"),
             ProjectArenaDebug::LexToString(NewRunState.Phase),
             ProjectArenaDebug::LexToString(GetNetMode()));
    }
    return false;
  }

  NewRunState.Phase = EArenaPhase::Countdown;
  NewRunState.Result = EArenaRunResult::None;
  ApplyPhaseFlags(NewRunState);
  ResetPlayerAliveStates();
  TeleportAllArenaPlayersToTaggedSpawns(ArenaPlayerSpawnTag,
                                        TEXT("StartArenaRun"));
  if (!PublishArenaRunState(NewRunState)) {
    return false;
  }

  UWorld *World = GetWorld();
  if (!World) {
    return false;
  }

  World->GetTimerManager().ClearTimer(ArenaCountdownTimerHandle);
  const float CountdownDelay = FMath::Max(0.0f, ArenaCountdownDuration);
  if (CountdownDelay <= KINDA_SMALL_NUMBER) {
    StartNextArenaWaveFromCountdown();
  } else {
    World->GetTimerManager().SetTimer(
        ArenaCountdownTimerHandle, this,
        &AArenaGameMode::StartNextArenaWaveFromCountdown, CountdownDelay, false);
  }

  UE_LOG(LogArenaGameMode, Display,
         TEXT("Arena run countdown started. Delay=%.2f ThreatsPerWave=%d ThreatClass=%s"),
         CountdownDelay, ThreatsPerWave, *GetNameSafe(ThreatClass.Get()));
  return true;
}

bool AArenaGameMode::ReportArenaThreatKilled(AActor *ThreatActor,
                                             AController *KillerController,
                                             int32 RewardCurrency) {
  if (!HasAuthority() || !ThreatActor || !KillerController ||
      RewardCurrency <= 0) {
    return false;
  }

  AArenaPlayerState *KillerPlayerState =
      KillerController->GetPlayerState<AArenaPlayerState>();
  if (!KillerPlayerState) {
    UE_LOG(LogArenaGameMode, Warning,
           TEXT("Arena threat kill rejected. Threat=%s Killer=%s Reward=%d Reason=NoArenaPlayerState"),
           *GetNameSafe(ThreatActor), *GetNameSafe(KillerController),
           RewardCurrency);
    return false;
  }

  FArenaPlayerRunStats NewPlayerStats =
      KillerPlayerState->GetArenaRunStats();
  ++NewPlayerStats.Kills;
  KillerPlayerState->SetArenaRunStats(NewPlayerStats);
  KillerPlayerState->AddArenaEarnedCurrency(RewardCurrency);

  int32 AliveThreatsAfterKill = -1;
  bool bWaveCompletedByKill = false;

  if (AArenaGameState *ArenaGameState = GetArenaGameState()) {
    FArenaRunState NewRunState = ArenaGameState->GetArenaRunState();
    if (NewRunState.Phase == EArenaPhase::Combat &&
        NewRunState.WaveState.AliveThreats > 0) {
      NewRunState.WaveState.AliveThreats =
          FMath::Max(0, NewRunState.WaveState.AliveThreats - 1);
      AliveThreatsAfterKill = NewRunState.WaveState.AliveThreats;

      if (NewRunState.WaveState.AliveThreats == 0) {
        NewRunState.Phase = EArenaPhase::Intermission;
        NewRunState.Result = EArenaRunResult::None;
        NewRunState.WaveState.bWaveCompleted = true;
        NewRunState.WaveState.bWaveFailed = false;
        bWaveCompletedByKill = true;
      }

      ApplyPhaseFlags(NewRunState);
      PublishArenaRunState(NewRunState);
      if (bWaveCompletedByKill) {
        ScheduleArenaIntermissionAdvance(NewRunState);
      }
    }
  }

  const FArenaPlayerRunStats FinalPlayerStats =
      KillerPlayerState->GetArenaRunStats();
  UE_LOG(LogArenaGameMode, Display,
         TEXT("Arena threat killed. Threat=%s Killer=%s Reward=%d Kills=%d Spendable=%d Earned=%d AliveThreats=%d WaveCompleted=%s"),
         *GetNameSafe(ThreatActor), *GetNameSafe(KillerController),
         RewardCurrency, FinalPlayerStats.Kills,
         FinalPlayerStats.SpendableCurrency, FinalPlayerStats.EarnedCurrency,
         AliveThreatsAfterKill,
         bWaveCompletedByKill ? TEXT("true") : TEXT("false"));
  return true;
}

bool AArenaGameMode::ReportArenaPlayerDied(AController *PlayerController,
                                           AActor *PlayerActor) {
  if (!HasAuthority() || !PlayerController) {
    return false;
  }

  AArenaPlayerState *ArenaPlayerState =
      PlayerController->GetPlayerState<AArenaPlayerState>();
  if (!ArenaPlayerState) {
    UE_LOG(LogArenaGameMode, Warning,
           TEXT("Arena player death rejected. Controller=%s Actor=%s Reason=NoArenaPlayerState"),
           *GetNameSafe(PlayerController), *GetNameSafe(PlayerActor));
    return false;
  }

  if (!ArenaPlayerState->IsArenaAlive()) {
    return true;
  }

  FArenaPlayerRunStats NewPlayerStats =
      ArenaPlayerState->GetArenaRunStats();
  ++NewPlayerStats.Deaths;
  ArenaPlayerState->SetArenaRunStats(NewPlayerStats);
  ArenaPlayerState->SetArenaAlive(false);

  const AArenaGameState *ArenaGameState = GetArenaGameState();
  const FArenaRunState CurrentRunState =
      ArenaGameState ? ArenaGameState->GetArenaRunState() : FArenaRunState();

  ClearPlayerArenaRuntimeRewards(ArenaPlayerState);

  bool bReturnedToLobby = false;
  bool bRespawnedAtCheckpoint = false;
  bool bWaitingForWaveRevive = false;
  if (CurrentRunState.bRunActive) {
    const bool bCanReviveAfterWave =
        CurrentRunState.Phase == EArenaPhase::Combat &&
        CurrentRunState.bWaveActive && HasAliveArenaPlayers();
    if (bCanReviveAfterWave) {
      DisableArenaPlayerUntilWaveComplete(PlayerController, PlayerActor);
      BeginSpectatingAliveTeammate(PlayerController);
      bWaitingForWaveRevive = true;
    } else {
      bReturnedToLobby = ReturnArenaRunToLobby(TEXT("PlayerDied"));
      FTransform LobbySpawnTransform;
      bRespawnedAtCheckpoint =
          bReturnedToLobby &&
          ResolveTaggedPlayerSpawnTransform(PlayerController, LobbyPlayerSpawnTag,
                                            LobbySpawnTransform);
      if (!bRespawnedAtCheckpoint) {
        bRespawnedAtCheckpoint =
            RespawnArenaPlayerAtCheckpoint(PlayerController, PlayerActor);
      }
    }
  } else {
    bRespawnedAtCheckpoint =
        RespawnArenaPlayerAtCheckpoint(PlayerController, PlayerActor);
  }

  if (bRespawnedAtCheckpoint) {
    ArenaPlayerState->SetArenaAlive(true);
  }

  UE_LOG(LogArenaGameMode, Display,
         TEXT("Arena player died. Player=%s Actor=%s RunMode=%s Deaths=%d Spendable=%d Earned=%d Committed=%d ReturnedToLobby=%s RespawnedAtCheckpoint=%s WaitingForWaveRevive=%s"),
         *ArenaPlayerState->GetPlayerName(), *GetNameSafe(PlayerActor),
         ProjectArenaDebug::LexToString(CurrentRunState.RunMode),
         ArenaPlayerState->GetDeaths(),
         ArenaPlayerState->GetSpendableCurrency(),
         ArenaPlayerState->GetEarnedCurrency(),
         ArenaPlayerState->GetCommittedCurrency(),
         bReturnedToLobby ? TEXT("true") : TEXT("false"),
         bRespawnedAtCheckpoint ? TEXT("true") : TEXT("false"),
         bWaitingForWaveRevive ? TEXT("true") : TEXT("false"));
  return true;
}

bool AArenaGameMode::SetArenaPhaseForDebug(EArenaPhase NewPhase) {
  AArenaGameState *ArenaGameState = GetArenaGameState();
  if (!ArenaGameState) {
    return false;
  }

  FArenaRunState NewRunState = ArenaGameState->GetArenaRunState();
  if (NewRunState.WaveState.TotalWaves <= 0) {
    NewRunState = BuildInitialRunState();
  }

  NewRunState.Phase = NewPhase;
  if (NewPhase != EArenaPhase::Result) {
    NewRunState.Result = EArenaRunResult::None;
  }
  ApplyPhaseFlags(NewRunState);
  return PublishArenaRunState(NewRunState);
}

bool AArenaGameMode::AdvanceArenaPhaseForDebug() {
  const AArenaGameState *ArenaGameState = GetArenaGameState();
  if (!ArenaGameState) {
    return false;
  }

  const FArenaRunState CurrentState = ArenaGameState->GetArenaRunState();
  switch (CurrentState.Phase) {
  case EArenaPhase::None:
  case EArenaPhase::Result:
    return InitializeArenaRun();
  case EArenaPhase::Lobby:
    return SetArenaPhaseForDebug(EArenaPhase::Countdown);
  case EArenaPhase::Countdown: {
    const int32 NextWave =
        FMath::Clamp(CurrentState.WaveState.CurrentWave + 1, 1,
                     FMath::Max(1, CurrentState.WaveState.TotalWaves));
    return StartCombatWaveForDebug(NextWave, 0);
  }
  case EArenaPhase::Combat:
    return CompleteWaveForDebug(true);
  case EArenaPhase::Intermission:
    if (CurrentState.WaveState.CurrentWave >= CurrentState.WaveState.TotalWaves) {
      return FinishArenaRunForDebug(EArenaRunResult::Completed);
    }
    return SetArenaPhaseForDebug(EArenaPhase::Countdown);
  default:
    break;
  }

  return false;
}

bool AArenaGameMode::StartCombatWaveForDebug(int32 WaveNumber,
                                             int32 SpawnedThreats) {
  ClearArenaRuntimeTimers();
  return StartArenaCombatWave(WaveNumber, SpawnedThreats, SpawnedThreats > 0,
                              TEXT("StartCombatWaveForDebug"));
}

bool AArenaGameMode::CompleteWaveForDebug(bool bSucceeded) {
  AArenaGameState *ArenaGameState = GetArenaGameState();
  if (!ArenaGameState) {
    return false;
  }

  FArenaRunState NewRunState = ArenaGameState->GetArenaRunState();
  if (NewRunState.WaveState.TotalWaves <= 0) {
    NewRunState = BuildInitialRunState();
  }

  NewRunState.WaveState.AliveThreats = 0;
  NewRunState.WaveState.bWaveCompleted = bSucceeded;
  NewRunState.WaveState.bWaveFailed = !bSucceeded;

  if (bSucceeded) {
    NewRunState.Phase = EArenaPhase::Intermission;
    NewRunState.Result = EArenaRunResult::None;
  } else {
    NewRunState.Phase = EArenaPhase::Result;
    NewRunState.Result = EArenaRunResult::Failed;
  }

  ApplyPhaseFlags(NewRunState);
  if (!PublishArenaRunState(NewRunState)) {
    return false;
  }

  if (bSucceeded) {
    ScheduleArenaIntermissionAdvance(NewRunState);
  }
  return true;
}

bool AArenaGameMode::FinishArenaRunForDebug(EArenaRunResult Result) {
  ClearArenaRuntimeTimers();

  AArenaGameState *ArenaGameState = GetArenaGameState();
  if (!ArenaGameState) {
    return false;
  }

  FArenaRunState NewRunState = ArenaGameState->GetArenaRunState();
  if (NewRunState.WaveState.TotalWaves <= 0) {
    NewRunState = BuildInitialRunState();
  }

  NewRunState.Phase = EArenaPhase::Result;
  NewRunState.Result = Result == EArenaRunResult::None ? EArenaRunResult::Aborted
                                                       : Result;
  NewRunState.WaveState.AliveThreats = 0;
  ApplyPhaseFlags(NewRunState);
  return PublishArenaRunState(NewRunState);
}

void AArenaGameMode::StartDebugAutoPhaseFlow() {
  UWorld *World = GetWorld();
  if (!World) {
    return;
  }

  DebugPhaseFlowStep = 0;
  World->GetTimerManager().ClearTimer(DebugPhaseFlowTimerHandle);
  World->GetTimerManager().SetTimer(
      DebugPhaseFlowTimerHandle, this, &AArenaGameMode::HandleDebugPhaseFlowStep,
      FMath::Max(0.1f, DebugPhaseStepInterval), true,
      FMath::Max(0.0f, DebugPhaseInitialDelay));

  if (bEnableDebugLogging) {
    UE_LOG(LogArenaGameMode, Display,
           TEXT("Debug auto phase flow scheduled. InitialDelay=%.2f StepInterval=%.2f NetMode=%s"),
           DebugPhaseInitialDelay, DebugPhaseStepInterval,
           ProjectArenaDebug::LexToString(World->GetNetMode()));
  }
}

void AArenaGameMode::StopDebugAutoPhaseFlow() {
  if (UWorld *World = GetWorld()) {
    World->GetTimerManager().ClearTimer(DebugPhaseFlowTimerHandle);
  }
}

void AArenaGameMode::StartNextArenaWaveFromCountdown() {
  const AArenaGameState *ArenaGameState = GetArenaGameState();
  if (!ArenaGameState) {
    return;
  }

  const FArenaRunState CurrentRunState = ArenaGameState->GetArenaRunState();
  if (CurrentRunState.Phase != EArenaPhase::Countdown) {
    UE_LOG(LogArenaGameMode, Display,
           TEXT("StartNextArenaWaveFromCountdown skipped. Phase=%s"),
           ProjectArenaDebug::LexToString(CurrentRunState.Phase));
    return;
  }

  const int32 TotalWaves =
      FMath::Max(1, CurrentRunState.WaveState.TotalWaves);
  const int32 NextWave =
      FMath::Clamp(CurrentRunState.WaveState.CurrentWave + 1, 1, TotalWaves);
  StartArenaCombatWave(NextWave, ThreatsPerWave, true,
                       TEXT("StartNextArenaWaveFromCountdown"));
}

void AArenaGameMode::StartNextArenaWaveFromIntermission() {
  const AArenaGameState *ArenaGameState = GetArenaGameState();
  if (!ArenaGameState) {
    return;
  }

  const FArenaRunState CurrentRunState = ArenaGameState->GetArenaRunState();
  if (CurrentRunState.Phase != EArenaPhase::Intermission) {
    UE_LOG(LogArenaGameMode, Display,
           TEXT("StartNextArenaWaveFromIntermission skipped. Phase=%s"),
           ProjectArenaDebug::LexToString(CurrentRunState.Phase));
    return;
  }

  const int32 TotalWaves =
      FMath::Max(1, CurrentRunState.WaveState.TotalWaves);
  if (CurrentRunState.WaveState.CurrentWave >= TotalWaves) {
    UE_LOG(LogArenaGameMode, Display,
           TEXT("Arena run completed after final wave. Wave=%d/%d ReturningToLobby=true"),
           CurrentRunState.WaveState.CurrentWave, TotalWaves);
    ReturnArenaRunToLobby(TEXT("RunCompleted"));
    return;
  }

  FArenaRunState CountdownRunState = CurrentRunState;
  CountdownRunState.Phase = EArenaPhase::Countdown;
  CountdownRunState.Result = EArenaRunResult::None;
  CountdownRunState.WaveState.AliveThreats = 0;
  CountdownRunState.WaveState.SpawnedThreats = 0;
  CountdownRunState.WaveState.bWaveCompleted = false;
  CountdownRunState.WaveState.bWaveFailed = false;
  ApplyPhaseFlags(CountdownRunState);
  if (!PublishArenaRunState(CountdownRunState)) {
    return;
  }

  UWorld *World = GetWorld();
  if (!World) {
    return;
  }

  World->GetTimerManager().ClearTimer(ArenaCountdownTimerHandle);
  const float CountdownDelay = FMath::Max(0.0f, ArenaCountdownDuration);
  if (CountdownDelay <= KINDA_SMALL_NUMBER) {
    StartNextArenaWaveFromCountdown();
  } else {
    World->GetTimerManager().SetTimer(
        ArenaCountdownTimerHandle, this,
        &AArenaGameMode::StartNextArenaWaveFromCountdown, CountdownDelay, false);
  }

  UE_LOG(LogArenaGameMode, Display,
         TEXT("Arena next wave countdown started. NextWave=%d/%d Delay=%.2f"),
         FMath::Clamp(CurrentRunState.WaveState.CurrentWave + 1, 1, TotalWaves),
         TotalWaves, CountdownDelay);
}

void AArenaGameMode::ScheduleArenaIntermissionAdvance(
    const FArenaRunState &RunState) {
  if (RunState.Phase != EArenaPhase::Intermission) {
    return;
  }

  const int32 RevivedPlayers = ReviveDeadArenaPlayersAtCheckpoint();

  UWorld *World = GetWorld();
  if (!World) {
    return;
  }

  World->GetTimerManager().ClearTimer(ArenaIntermissionTimerHandle);
  const float IntermissionDelay = FMath::Max(0.0f, ArenaIntermissionDuration);
  if (IntermissionDelay <= KINDA_SMALL_NUMBER) {
    StartNextArenaWaveFromIntermission();
  } else {
    World->GetTimerManager().SetTimer(
        ArenaIntermissionTimerHandle, this,
        &AArenaGameMode::StartNextArenaWaveFromIntermission, IntermissionDelay,
        false);
  }

  UE_LOG(LogArenaGameMode, Display,
         TEXT("Arena intermission advance scheduled. Wave=%d/%d Delay=%.2f RevivedPlayers=%d"),
         RunState.WaveState.CurrentWave, RunState.WaveState.TotalWaves,
         IntermissionDelay, RevivedPlayers);
}

bool AArenaGameMode::StartArenaCombatWave(int32 WaveNumber,
                                          int32 RequestedThreats,
                                          bool bSpawnThreats,
                                          const TCHAR *Context) {
  AArenaGameState *ArenaGameState = GetArenaGameState();
  if (!ArenaGameState) {
    return false;
  }

  FArenaRunState NewRunState = ArenaGameState->GetArenaRunState();
  if (NewRunState.WaveState.TotalWaves <= 0) {
    NewRunState = BuildInitialRunState();
  }

  const int32 RequestedThreatCount = FMath::Max(0, RequestedThreats);
  const int32 SpawnedThreatCount =
      bSpawnThreats ? SpawnArenaThreatsForWave(RequestedThreatCount)
                    : RequestedThreatCount;
  const int32 TotalWaves = FMath::Max(1, NewRunState.WaveState.TotalWaves);
  NewRunState.Phase = SpawnedThreatCount > 0 ? EArenaPhase::Combat
                                             : EArenaPhase::Intermission;
  NewRunState.Result = EArenaRunResult::None;
  NewRunState.WaveState.CurrentWave = FMath::Clamp(WaveNumber, 1, TotalWaves);
  NewRunState.WaveState.TotalWaves = TotalWaves;
  NewRunState.WaveState.SpawnedThreats = SpawnedThreatCount;
  NewRunState.WaveState.AliveThreats = SpawnedThreatCount;
  NewRunState.WaveState.bWaveCompleted = SpawnedThreatCount <= 0;
  NewRunState.WaveState.bWaveFailed = false;
  ApplyPhaseFlags(NewRunState);

  UE_LOG(LogArenaGameMode, Display,
         TEXT("Arena combat wave started. Context=%s Wave=%d/%d RequestedThreats=%d SpawnedThreats=%d Phase=%s"),
         Context ? Context : TEXT("Unknown"),
         NewRunState.WaveState.CurrentWave, TotalWaves, RequestedThreatCount,
         SpawnedThreatCount, ProjectArenaDebug::LexToString(NewRunState.Phase));
  return PublishArenaRunState(NewRunState);
}

int32 AArenaGameMode::SpawnArenaThreatsForWave(int32 RequestedThreats) {
  UWorld *World = GetWorld();
  if (!World || RequestedThreats <= 0) {
    return 0;
  }

  if (!ThreatClass) {
    UE_LOG(LogArenaGameMode, Warning,
           TEXT("Arena threat spawn skipped. Reason=NoThreatClass RequestedThreats=%d"),
           RequestedThreats);
    return 0;
  }

  int32 SpawnedThreats = 0;
  for (int32 SpawnIndex = 0; SpawnIndex < RequestedThreats; ++SpawnIndex) {
    FTransform SpawnTransform;
    if (!ResolveThreatSpawnTransform(SpawnIndex, SpawnTransform)) {
      UE_LOG(LogArenaGameMode, Warning,
             TEXT("Arena threat spawn skipped. Reason=NoThreatSpawnPoint Tag=%s RequestedThreats=%d"),
             *ThreatSpawnPointTag.ToString(), RequestedThreats);
      break;
    }

    FActorSpawnParameters SpawnParams;
    SpawnParams.Owner = this;
    SpawnParams.SpawnCollisionHandlingOverride =
        ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

    AActor *SpawnedThreat =
        World->SpawnActor<AActor>(ThreatClass, SpawnTransform, SpawnParams);
    if (!SpawnedThreat) {
      UE_LOG(LogArenaGameMode, Warning,
             TEXT("Arena threat spawn failed. Class=%s SpawnIndex=%d Location=%s"),
             *GetNameSafe(ThreatClass.Get()), SpawnIndex,
             *SpawnTransform.GetLocation().ToCompactString());
      continue;
    }

    if (!SpawnedThreat->GetIsReplicated()) {
      SpawnedThreat->SetReplicates(true);
    }
    SpawnedThreat->SetReplicateMovement(true);
    EnsureThreatAIComponent(SpawnedThreat);
    SpawnedThreat->ForceNetUpdate();

    ++SpawnedThreats;
    UE_LOG(LogArenaGameMode, Display,
           TEXT("Arena threat spawned. Threat=%s Class=%s SpawnIndex=%d Location=%s"),
           *GetNameSafe(SpawnedThreat), *GetNameSafe(ThreatClass.Get()),
           SpawnIndex, *SpawnTransform.GetLocation().ToCompactString());
  }

  return SpawnedThreats;
}

void AArenaGameMode::EnsureThreatAIComponent(AActor *ThreatActor) const {
  if (!bAttachDefaultThreatAIOnSpawn || !ThreatActor ||
      !DefaultThreatAIComponentClass ||
      ThreatActor->FindComponentByClass<UThreatAIComponent>()) {
    return;
  }

  UThreatAIComponent *ThreatAIComponent =
      NewObject<UThreatAIComponent>(ThreatActor, DefaultThreatAIComponentClass,
                                    TEXT("ThreatAIComponent"));
  if (!ThreatAIComponent) {
    UE_LOG(LogArenaGameMode, Warning,
           TEXT("Arena threat AI attach failed. Threat=%s Class=%s"),
           *GetNameSafe(ThreatActor),
           *GetNameSafe(DefaultThreatAIComponentClass.Get()));
    return;
  }

  ThreatActor->AddInstanceComponent(ThreatAIComponent);
  ThreatAIComponent->RegisterComponent();

  UE_LOG(LogArenaGameMode, Display,
         TEXT("Arena threat AI attached. Threat=%s ComponentClass=%s"),
         *GetNameSafe(ThreatActor),
         *GetNameSafe(DefaultThreatAIComponentClass.Get()));
}

bool AArenaGameMode::ResolveThreatSpawnTransform(
    int32 SpawnIndex, FTransform &OutTransform) const {
  if (ThreatSpawnPointTag.IsNone()) {
    return false;
  }

  UWorld *World = GetWorld();
  if (!World) {
    return false;
  }

  TArray<AActor *> SpawnActors;
  UGameplayStatics::GetAllActorsWithTag(World, ThreatSpawnPointTag,
                                        SpawnActors);
  SpawnActors.RemoveAll([](const AActor *Actor) { return !IsValid(Actor); });
  if (SpawnActors.IsEmpty()) {
    return false;
  }

  SpawnActors.Sort([](const AActor &Left, const AActor &Right) {
    return Left.GetName() < Right.GetName();
  });

  const int32 ResolvedSpawnIndex =
      FMath::Max(0, SpawnIndex) % SpawnActors.Num();
  OutTransform = SpawnActors[ResolvedSpawnIndex]->GetActorTransform();
  return true;
}

bool AArenaGameMode::HasAliveArenaPlayers() const {
  const AGameStateBase *BaseGameState = GameState;
  if (!BaseGameState) {
    return false;
  }

  for (const APlayerState *PlayerState : BaseGameState->PlayerArray) {
    const AArenaPlayerState *ArenaPlayerState =
        Cast<AArenaPlayerState>(PlayerState);
    if (ArenaPlayerState && ArenaPlayerState->IsArenaAlive()) {
      return true;
    }
  }

  return false;
}

void AArenaGameMode::DisableArenaPlayerUntilWaveComplete(
    AController *PlayerController, AActor *PlayerActor) const {
  if (!PlayerActor) {
    return;
  }

  if (ACharacter *Character = Cast<ACharacter>(PlayerActor)) {
    if (UCharacterMovementComponent *MovementComponent =
            Character->GetCharacterMovement()) {
      MovementComponent->StopMovementImmediately();
      MovementComponent->DisableMovement();
    }
  }

  PlayerActor->SetCanBeDamaged(false);
  PlayerActor->SetActorEnableCollision(false);
  PlayerActor->SetActorHiddenInGame(true);
  PlayerActor->ForceNetUpdate();

  if (APlayerController *PlayerControllerObject =
          Cast<APlayerController>(PlayerController)) {
    PlayerControllerObject->SetIgnoreMoveInput(true);
    PlayerControllerObject->SetIgnoreLookInput(true);
  }

  UE_LOG(LogArenaGameMode, Display,
         TEXT("Arena player disabled until wave complete. Player=%s Actor=%s"),
         *GetNameSafe(PlayerController), *GetNameSafe(PlayerActor));
}

bool AArenaGameMode::BeginSpectatingAliveTeammate(
    AController *PlayerController) const {
  APlayerController *DeadPlayerController =
      Cast<APlayerController>(PlayerController);
  const APlayerState *DeadPlayerState =
      PlayerController ? PlayerController->PlayerState : nullptr;
  const AGameStateBase *BaseGameState = GameState;
  if (!DeadPlayerController || !DeadPlayerState || !BaseGameState) {
    return false;
  }

  for (APlayerState *PlayerState : BaseGameState->PlayerArray) {
    const AArenaPlayerState *ArenaPlayerState =
        Cast<AArenaPlayerState>(PlayerState);
    if (!ArenaPlayerState || PlayerState == DeadPlayerState ||
        !ArenaPlayerState->IsArenaAlive()) {
      continue;
    }

    AController *TeammateController =
        Cast<AController>(ArenaPlayerState->GetOwner());
    APawn *TeammatePawn = TeammateController ? TeammateController->GetPawn()
                                             : nullptr;
    if (!TeammatePawn || TeammatePawn->IsHidden()) {
      continue;
    }

    FViewTargetTransitionParams TransitionParams;
    TransitionParams.BlendTime = 0.25f;
    DeadPlayerController->SetViewTarget(TeammatePawn, TransitionParams);
    DeadPlayerController->ClientSetViewTarget(TeammatePawn, TransitionParams);

    UE_LOG(LogArenaGameMode, Display,
           TEXT("Arena player spectating teammate. Player=%s Teammate=%s TeammatePawn=%s"),
           *GetNameSafe(DeadPlayerController),
           *ArenaPlayerState->GetPlayerName(), *GetNameSafe(TeammatePawn));
    return true;
  }

  UE_LOG(LogArenaGameMode, Display,
         TEXT("Arena spectator target not found. Player=%s"),
         *GetNameSafe(PlayerController));
  return false;
}

int32 AArenaGameMode::ReviveDeadArenaPlayersAtCheckpoint() {
  const AGameStateBase *BaseGameState = GameState;
  if (!BaseGameState) {
    return 0;
  }

  int32 RevivedPlayers = 0;
  for (APlayerState *PlayerState : BaseGameState->PlayerArray) {
    AArenaPlayerState *ArenaPlayerState = Cast<AArenaPlayerState>(PlayerState);
    if (!ArenaPlayerState || ArenaPlayerState->IsArenaAlive()) {
      continue;
    }

    AController *Controller =
        Cast<AController>(ArenaPlayerState->GetOwner());
    APawn *Pawn = Controller ? Controller->GetPawn() : nullptr;
    if (!Controller || !Pawn) {
      UE_LOG(LogArenaGameMode, Warning,
             TEXT("Arena wave revive skipped. Player=%s Reason=NoControllerOrPawn"),
             *ArenaPlayerState->GetPlayerName());
      continue;
    }

    if (!RespawnArenaPlayerAtCheckpoint(Controller, Pawn)) {
      UE_LOG(LogArenaGameMode, Warning,
             TEXT("Arena wave revive skipped. Player=%s Reason=RespawnFailed"),
             *ArenaPlayerState->GetPlayerName());
      continue;
    }

    ArenaPlayerState->SetArenaAlive(true);
    ++RevivedPlayers;
    UE_LOG(LogArenaGameMode, Display,
           TEXT("Arena player revived after completed wave. Player=%s Pawn=%s"),
           *ArenaPlayerState->GetPlayerName(), *GetNameSafe(Pawn));
  }

  return RevivedPlayers;
}

void AArenaGameMode::ClearArenaRuntimeTimers() {
  if (UWorld *World = GetWorld()) {
    World->GetTimerManager().ClearTimer(ArenaCountdownTimerHandle);
    World->GetTimerManager().ClearTimer(ArenaIntermissionTimerHandle);
  }
}

AArenaGameState *AArenaGameMode::GetArenaGameState() const {
  return GetGameState<AArenaGameState>();
}

bool AArenaGameMode::PublishArenaRunState(
    const FArenaRunState &NewRunState) const {
  if (AArenaGameState *ArenaGameState = GetArenaGameState()) {
    ArenaGameState->SetArenaRunState(NewRunState);
    LogArenaRunState(TEXT("PublishArenaRunState"), NewRunState);
    return true;
  }

  return false;
}

void AArenaGameMode::ApplyPhaseFlags(FArenaRunState &RunState) const {
  RunState.bWaveActive = RunState.Phase == EArenaPhase::Combat;
  RunState.bRunActive = RunState.Phase == EArenaPhase::Countdown ||
                        RunState.Phase == EArenaPhase::Combat ||
                        RunState.Phase == EArenaPhase::Intermission;
  RunState.bRunCompleted = RunState.Phase == EArenaPhase::Result &&
                           RunState.Result == EArenaRunResult::Completed;
  RunState.bRunFailed = RunState.Phase == EArenaPhase::Result &&
                        RunState.Result == EArenaRunResult::Failed;
}

void AArenaGameMode::ResetPlayerRunStats() const {
  const AGameStateBase *BaseGameState = GameState;
  if (!BaseGameState) {
    return;
  }

  for (APlayerState *PlayerState : BaseGameState->PlayerArray) {
    if (AArenaPlayerState *ArenaPlayerState = Cast<AArenaPlayerState>(PlayerState)) {
      ArenaPlayerState->ResetArenaRunStats();
    }
  }
}

void AArenaGameMode::ResetPlayerReadyStates() const {
  const AGameStateBase *BaseGameState = GameState;
  if (!BaseGameState) {
    return;
  }

  for (APlayerState *PlayerState : BaseGameState->PlayerArray) {
    if (AArenaPlayerState *ArenaPlayerState = Cast<AArenaPlayerState>(PlayerState)) {
      ArenaPlayerState->SetArenaReady(false);
    }
  }
}

void AArenaGameMode::ResetPlayerAliveStates() const {
  const AGameStateBase *BaseGameState = GameState;
  if (!BaseGameState) {
    return;
  }

  for (APlayerState *PlayerState : BaseGameState->PlayerArray) {
    if (AArenaPlayerState *ArenaPlayerState =
            Cast<AArenaPlayerState>(PlayerState)) {
      ArenaPlayerState->SetArenaAlive(true);
    }
  }
}

void AArenaGameMode::ClearPlayerArenaRuntimeRewards(
    AArenaPlayerState *PlayerState) const {
  if (!PlayerState) {
    return;
  }

  FArenaPlayerRunStats NewStats = PlayerState->GetArenaRunStats();
  NewStats.SpendableCurrency = 0;
  NewStats.EarnedCurrency = 0;
  NewStats.CommittedCurrency = 0;
  PlayerState->SetArenaRunStats(NewStats);
}

bool AArenaGameMode::ReturnArenaRunToLobby(const TCHAR *Reason) {
  ClearArenaRuntimeTimers();

  AArenaGameState *ArenaGameState = GetArenaGameState();
  if (!ArenaGameState) {
    return false;
  }

  FArenaRunState NewRunState = BuildInitialRunState();
  ApplyPhaseFlags(NewRunState);
  ResetPlayerReadyStates();
  ResetPlayerAliveStates();

  if (bEnableDebugLogging) {
    UE_LOG(LogArenaGameMode, Display,
           TEXT("Arena run returned to lobby. Reason=%s NetMode=%s"),
           Reason ? Reason : TEXT("Unknown"),
           ProjectArenaDebug::LexToString(GetNetMode()));
  }

  TeleportAllArenaPlayersToTaggedSpawns(LobbyPlayerSpawnTag,
                                        Reason ? Reason : TEXT("ReturnArenaRunToLobby"));
  return PublishArenaRunState(NewRunState);
}

void AArenaGameMode::TryStartArenaRunIfReady() {
  const AArenaGameState *ArenaGameState = GetArenaGameState();
  if (!ArenaGameState) {
    return;
  }

  const FArenaRunState CurrentRunState = ArenaGameState->GetArenaRunState();
  if (CurrentRunState.Phase != EArenaPhase::Lobby) {
    return;
  }

  if (!AreAllPlayersReady()) {
    return;
  }

  if (bEnableDebugLogging) {
    UE_LOG(LogArenaGameMode, Display,
           TEXT("All required players ready. Starting arena run. NetMode=%s"),
           ProjectArenaDebug::LexToString(GetNetMode()));
  }

  StartArenaRun();
}

void AArenaGameMode::RunDebugCurrencyTestIfNeeded(APlayerController *NewPlayer) {
  if (!bArenaRunInitialized || !bRunDebugCurrencyTestOnFirstLogin ||
      bDebugCurrencyTestHasRun || !HasAuthority() || !NewPlayer) {
    return;
  }

  AArenaPlayerState *ArenaPlayerState =
      NewPlayer->GetPlayerState<AArenaPlayerState>();
  if (!ArenaPlayerState) {
    return;
  }

  bDebugCurrencyTestHasRun = true;
  FArenaRuntimeCurrencyDebugTest::Run(*ArenaPlayerState);
}

void AArenaGameMode::SeedDebugPlayerStats(APlayerController *NewPlayer) const {
  const UWorld *World = GetWorld();
  const bool bForceDebugSeedInPIE =
      World && World->WorldType == EWorldType::PIE &&
      DebugSeedSpendableCurrency > 0;
  if (!bArenaRunInitialized || !NewPlayer ||
      (!bSeedDebugPlayerStatsOnLogin && !bForceDebugSeedInPIE)) {
    return;
  }

  AArenaPlayerState *ArenaPlayerState =
      NewPlayer->GetPlayerState<AArenaPlayerState>();
  if (!ArenaPlayerState) {
    return;
  }

  const AGameStateBase *BaseGameState = GameState;
  const int32 PlayerIndex = BaseGameState ? BaseGameState->PlayerArray.Num() : 1;

  FArenaPlayerRunStats DebugStats;
  DebugStats.Kills = FMath::Max(1, PlayerIndex);
  DebugStats.Deaths = 0;
  DebugStats.SpendableCurrency = FMath::Max(0, DebugSeedSpendableCurrency);
  DebugStats.EarnedCurrency = DebugStats.SpendableCurrency;
  DebugStats.CommittedCurrency = 0;
  DebugStats.ReachedWave = 1;
  ArenaPlayerState->SetArenaRunStats(DebugStats);
  ArenaPlayerState->ForceNetUpdate();

  UE_LOG(LogArenaGameMode, Display,
         TEXT("Seeded debug player stats. Player=%s NetMode=%s Spendable=%d ForcePIE=%s"),
         *ArenaPlayerState->GetPlayerName(),
         ProjectArenaDebug::LexToString(GetNetMode()),
         DebugStats.SpendableCurrency,
         bForceDebugSeedInPIE ? TEXT("true") : TEXT("false"));
}

void AArenaGameMode::SeedDebugPlayerStatsForExistingPlayers() {
  const AGameStateBase *BaseGameState = GameState;
  if (!BaseGameState) {
    return;
  }

  for (APlayerState *PlayerState : BaseGameState->PlayerArray) {
    APlayerController *PlayerController =
        PlayerState ? Cast<APlayerController>(PlayerState->GetOwner()) : nullptr;
    if (!PlayerController) {
      continue;
    }

    RunDebugCurrencyTestIfNeeded(PlayerController);
    SeedDebugPlayerStats(PlayerController);
  }
}

void AArenaGameMode::ApplyDebugPlayerSpawnOffset(AController *NewPlayer) const {
  if (!bOffsetDebugPlayerSpawnsInPIE || !NewPlayer || !NewPlayer->GetPawn()) {
    return;
  }

  const UWorld *World = GetWorld();
  if (!World || World->WorldType != EWorldType::PIE) {
    return;
  }

  const AGameStateBase *BaseGameState = GameState;
  const APlayerState *TargetPlayerState = NewPlayer->PlayerState;
  int32 PlayerIndex = INDEX_NONE;
  if (BaseGameState && TargetPlayerState) {
    PlayerIndex = BaseGameState->PlayerArray.IndexOfByKey(TargetPlayerState);
  }

  if (PlayerIndex <= 0) {
    return;
  }

  APawn *Pawn = NewPlayer->GetPawn();
  const FVector Offset =
      Pawn->GetActorRightVector() * (DebugPlayerSpawnSpacing * PlayerIndex);
  const FVector NewLocation = Pawn->GetActorLocation() + Offset;
  Pawn->SetActorLocation(NewLocation, false, nullptr, ETeleportType::TeleportPhysics);

  UE_LOG(LogArenaGameMode, Display,
         TEXT("Applied debug player spawn offset. Player=%s Pawn=%s Index=%d Offset=%s Location=%s"),
         *GetNameSafe(NewPlayer), *GetNameSafe(Pawn), PlayerIndex,
         *Offset.ToCompactString(), *NewLocation.ToCompactString());
}

void AArenaGameMode::RegisterPlayerRespawnCheckpoint(
    AController *PlayerController) {
  AArenaPlayerState *ArenaPlayerState =
      PlayerController ? PlayerController->GetPlayerState<AArenaPlayerState>()
                       : nullptr;
  APawn *Pawn = PlayerController ? PlayerController->GetPawn() : nullptr;
  if (!ArenaPlayerState || !Pawn) {
    return;
  }

  FTransform CheckpointTransform = Pawn->GetActorTransform();
  if (!ResolveTaggedPlayerSpawnTransform(PlayerController, ArenaPlayerSpawnTag,
                                         CheckpointTransform)) {
    ResolveTaggedPlayerSpawnTransform(PlayerController, LobbyPlayerSpawnTag,
                                      CheckpointTransform);
  }

  PlayerRespawnCheckpointTransforms.FindOrAdd(ArenaPlayerState) =
      CheckpointTransform;

  UE_LOG(LogArenaGameMode, Display,
         TEXT("Registered arena respawn checkpoint. Player=%s Pawn=%s Location=%s Rotation=%s"),
         *ArenaPlayerState->GetPlayerName(), *GetNameSafe(Pawn),
         *CheckpointTransform.GetLocation().ToCompactString(),
         *CheckpointTransform.GetRotation().Rotator().ToCompactString());
}

bool AArenaGameMode::RespawnArenaPlayerAtCheckpoint(
    AController *PlayerController, AActor *PlayerActor) const {
  const AArenaPlayerState *ArenaPlayerState =
      PlayerController ? PlayerController->GetPlayerState<AArenaPlayerState>()
                       : nullptr;
  if (!ArenaPlayerState || !PlayerActor) {
    return false;
  }

  const FTransform *CheckpointTransform =
      PlayerRespawnCheckpointTransforms.Find(ArenaPlayerState);
  if (!CheckpointTransform) {
    UE_LOG(LogArenaGameMode, Warning,
           TEXT("Arena checkpoint respawn skipped. Player=%s Actor=%s Reason=NoCheckpoint"),
           *ArenaPlayerState->GetPlayerName(), *GetNameSafe(PlayerActor));
    return false;
  }

  return TeleportPlayerActor(PlayerController, PlayerActor, *CheckpointTransform,
                             TEXT("RespawnCheckpoint"));
}

void AArenaGameMode::TeleportAllArenaPlayersToTaggedSpawns(
    FName SpawnTag, const TCHAR *Context) {
  const AGameStateBase *BaseGameState = GameState;
  if (!BaseGameState) {
    return;
  }

  for (APlayerState *PlayerState : BaseGameState->PlayerArray) {
    AController *Controller =
        PlayerState ? Cast<AController>(PlayerState->GetOwner()) : nullptr;
    if (!Controller) {
      continue;
    }

    TeleportArenaPlayerToTaggedSpawn(Controller, SpawnTag, Context);
  }
}

bool AArenaGameMode::TeleportArenaPlayerToTaggedSpawn(
    AController *PlayerController, FName SpawnTag, const TCHAR *Context) {
  if (!PlayerController || SpawnTag.IsNone()) {
    return false;
  }

  APawn *Pawn = PlayerController->GetPawn();
  if (!Pawn) {
    UE_LOG(LogArenaGameMode, Warning,
           TEXT("Arena tagged teleport skipped. Controller=%s Tag=%s Context=%s Reason=NoPawn"),
           *GetNameSafe(PlayerController), *SpawnTag.ToString(),
           Context ? Context : TEXT("Unknown"));
    return false;
  }

  FTransform DestinationTransform;
  if (!ResolveTaggedPlayerSpawnTransform(PlayerController, SpawnTag,
                                         DestinationTransform)) {
    UE_LOG(LogArenaGameMode, Warning,
           TEXT("Arena tagged teleport skipped. Controller=%s Pawn=%s Tag=%s Context=%s Reason=NoTaggedSpawn"),
           *GetNameSafe(PlayerController), *GetNameSafe(Pawn),
           *SpawnTag.ToString(), Context ? Context : TEXT("Unknown"));
    return false;
  }

  return TeleportPlayerActor(PlayerController, Pawn, DestinationTransform,
                             Context);
}

bool AArenaGameMode::ResolveTaggedPlayerSpawnTransform(
    AController *PlayerController, FName SpawnTag,
    FTransform &OutTransform) const {
  if (SpawnTag.IsNone()) {
    return false;
  }

  UWorld *World = GetWorld();
  if (!World) {
    return false;
  }

  TArray<AActor *> SpawnActors;
  UGameplayStatics::GetAllActorsWithTag(World, SpawnTag, SpawnActors);
  SpawnActors.RemoveAll([](const AActor *Actor) { return !IsValid(Actor); });
  if (SpawnActors.IsEmpty()) {
    return false;
  }

  SpawnActors.Sort([](const AActor &Left, const AActor &Right) {
    return Left.GetName() < Right.GetName();
  });

  const int32 PlayerIndex = GetArenaPlayerIndex(PlayerController);
  const int32 SpawnIndex =
      PlayerIndex >= 0 ? PlayerIndex % SpawnActors.Num() : 0;
  OutTransform = SpawnActors[SpawnIndex]->GetActorTransform();
  return true;
}

bool AArenaGameMode::TeleportPlayerActor(
    AController *PlayerController, AActor *PlayerActor,
    const FTransform &DestinationTransform, const TCHAR *Context) const {
  if (!PlayerActor) {
    return false;
  }

  if (ACharacter *Character = Cast<ACharacter>(PlayerActor)) {
    if (UCharacterMovementComponent *MovementComponent =
            Character->GetCharacterMovement()) {
      MovementComponent->StopMovementImmediately();
      MovementComponent->ClearAccumulatedForces();
      MovementComponent->SetMovementMode(MOVE_Walking);
    }
  }

  PlayerActor->SetActorHiddenInGame(false);
  PlayerActor->SetActorEnableCollision(true);
  PlayerActor->SetCanBeDamaged(true);

  const FVector RespawnLocation = DestinationTransform.GetLocation();
  const FRotator RespawnRotation = DestinationTransform.GetRotation().Rotator();
  const bool bTeleported =
      PlayerActor->TeleportTo(RespawnLocation, RespawnRotation, false, true);
  if (!bTeleported) {
    PlayerActor->SetActorTransform(DestinationTransform, false, nullptr,
                                   ETeleportType::TeleportPhysics);
  }

  if (APlayerController *PlayerControllerObject =
          Cast<APlayerController>(PlayerController)) {
    PlayerControllerObject->ResetIgnoreMoveInput();
    PlayerControllerObject->ResetIgnoreLookInput();
    PlayerControllerObject->SetControlRotation(RespawnRotation);
    if (APawn *Pawn = Cast<APawn>(PlayerActor)) {
      FViewTargetTransitionParams TransitionParams;
      TransitionParams.BlendTime = 0.15f;
      PlayerControllerObject->SetViewTarget(Pawn, TransitionParams);
      PlayerControllerObject->ClientSetViewTarget(Pawn, TransitionParams);
      PlayerControllerObject->ClientRestart(Pawn);
    }
  }

  PlayerActor->ForceNetUpdate();

  UE_LOG(LogArenaGameMode, Display,
         TEXT("Teleported arena player. Context=%s Player=%s Actor=%s Location=%s Rotation=%s Teleported=%s"),
         Context ? Context : TEXT("Unknown"), *GetNameSafe(PlayerController),
         *GetNameSafe(PlayerActor),
         *RespawnLocation.ToCompactString(), *RespawnRotation.ToCompactString(),
         bTeleported ? TEXT("true") : TEXT("false"));
  return true;
}

int32 AArenaGameMode::GetArenaPlayerIndex(AController *PlayerController) const {
  const AGameStateBase *BaseGameState = GameState;
  const APlayerState *TargetPlayerState =
      PlayerController ? PlayerController->PlayerState : nullptr;
  if (!BaseGameState || !TargetPlayerState) {
    return INDEX_NONE;
  }

  return BaseGameState->PlayerArray.IndexOfByKey(TargetPlayerState);
}

void AArenaGameMode::HandleDebugPhaseFlowStep() {
  bool bKeepRunning = true;

  switch (DebugPhaseFlowStep) {
  case 0:
    SetArenaPhaseForDebug(EArenaPhase::Countdown);
    break;
  case 1:
    StartCombatWaveForDebug(1, DebugWaveThreatCount);
    break;
  case 2:
    CompleteWaveForDebug(true);
    break;
  case 3:
    FinishArenaRunForDebug(EArenaRunResult::Completed);
    bKeepRunning = false;
    break;
  default:
    bKeepRunning = false;
    break;
  }

  ++DebugPhaseFlowStep;

  if (!bKeepRunning) {
    StopDebugAutoPhaseFlow();
  }
}

void AArenaGameMode::LogArenaRunState(const TCHAR *Context,
                                      const FArenaRunState &RunState) const {
  if (!bEnableDebugLogging) {
    return;
  }

  const UWorld *World = GetWorld();
  UE_LOG(LogArenaGameMode, Display,
         TEXT("%s NetMode=%s Phase=%s Mode=%s Result=%s Wave=%d/%d Alive=%d Spawned=%d RunActive=%s WaveActive=%s"),
         Context ? Context : TEXT("ArenaState"),
         ProjectArenaDebug::LexToString(World ? World->GetNetMode() : NM_MAX),
         ProjectArenaDebug::LexToString(RunState.Phase),
         ProjectArenaDebug::LexToString(RunState.RunMode),
         ProjectArenaDebug::LexToString(RunState.Result),
         RunState.WaveState.CurrentWave, RunState.WaveState.TotalWaves,
         RunState.WaveState.AliveThreats, RunState.WaveState.SpawnedThreats,
         RunState.bRunActive ? TEXT("true") : TEXT("false"),
         RunState.bWaveActive ? TEXT("true") : TEXT("false"));
}
