// Copyright Epic Games, Inc. All Rights Reserved.

#include "Arena/ArenaGameMode.h"
#include "Arena/Debug/ArenaRuntimeCurrencyDebugTest.h"
#include "Arena/ArenaGameState.h"
#include "Arena/ArenaPlayerState.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "GameFramework/Controller.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerState.h"

DEFINE_LOG_CATEGORY_STATIC(LogArenaGameMode, Log, All);

AArenaGameMode::AArenaGameMode() {
  GameStateClass = AArenaGameState::StaticClass();
  PlayerStateClass = AArenaPlayerState::StaticClass();
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
  return PublishArenaRunState(NewRunState);
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
  if (CurrentRunState.bRunActive) {
    bReturnedToLobby =
        ReturnArenaRunToLobbyAfterPlayerDeath(TEXT("PlayerDied"));
  }

  UE_LOG(LogArenaGameMode, Display,
         TEXT("Arena player died. Player=%s Actor=%s RunMode=%s Deaths=%d Spendable=%d Earned=%d Committed=%d ReturnedToLobby=%s"),
         *ArenaPlayerState->GetPlayerName(), *GetNameSafe(PlayerActor),
         ProjectArenaDebug::LexToString(CurrentRunState.RunMode),
         ArenaPlayerState->GetDeaths(),
         ArenaPlayerState->GetSpendableCurrency(),
         ArenaPlayerState->GetEarnedCurrency(),
         ArenaPlayerState->GetCommittedCurrency(),
         bReturnedToLobby ? TEXT("true") : TEXT("false"));
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
  AArenaGameState *ArenaGameState = GetArenaGameState();
  if (!ArenaGameState) {
    return false;
  }

  FArenaRunState NewRunState = ArenaGameState->GetArenaRunState();
  if (NewRunState.WaveState.TotalWaves <= 0) {
    NewRunState = BuildInitialRunState();
  }

  const int32 TotalWaves = FMath::Max(1, NewRunState.WaveState.TotalWaves);
  NewRunState.Phase = EArenaPhase::Combat;
  NewRunState.Result = EArenaRunResult::None;
  NewRunState.WaveState.CurrentWave = FMath::Clamp(WaveNumber, 1, TotalWaves);
  NewRunState.WaveState.TotalWaves = TotalWaves;
  NewRunState.WaveState.SpawnedThreats = FMath::Max(0, SpawnedThreats);
  NewRunState.WaveState.AliveThreats = FMath::Max(0, SpawnedThreats);
  NewRunState.WaveState.bWaveCompleted = false;
  NewRunState.WaveState.bWaveFailed = false;
  ApplyPhaseFlags(NewRunState);
  return PublishArenaRunState(NewRunState);
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
  return PublishArenaRunState(NewRunState);
}

bool AArenaGameMode::FinishArenaRunForDebug(EArenaRunResult Result) {
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

bool AArenaGameMode::ReturnArenaRunToLobbyAfterPlayerDeath(const TCHAR *Reason) {
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
           TEXT("Arena run returned to lobby after player death. Reason=%s NetMode=%s"),
           Reason ? Reason : TEXT("Unknown"),
           ProjectArenaDebug::LexToString(GetNetMode()));
  }

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
