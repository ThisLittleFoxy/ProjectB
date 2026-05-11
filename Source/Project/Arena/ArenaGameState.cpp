// Copyright Epic Games, Inc. All Rights Reserved.

#include "Arena/ArenaGameState.h"
#include "Net/UnrealNetwork.h"

DEFINE_LOG_CATEGORY_STATIC(LogArenaGameState, Log, All);

AArenaGameState::AArenaGameState() { bReplicates = true; }

void AArenaGameState::GetLifetimeReplicatedProps(
    TArray<FLifetimeProperty> &OutLifetimeProps) const {
  Super::GetLifetimeReplicatedProps(OutLifetimeProps);

  DOREPLIFETIME(AArenaGameState, ArenaRunState);
}

void AArenaGameState::SetArenaRunState(const FArenaRunState &NewRunState) {
  ArenaRunState = NewRunState;
  UE_LOG(LogArenaGameState, Display,
         TEXT("SetArenaRunState NetMode=%s Phase=%s Mode=%s Result=%s Wave=%d/%d Alive=%d Spawned=%d"),
         ProjectArenaDebug::LexToString(GetNetMode()),
         ProjectArenaDebug::LexToString(ArenaRunState.Phase),
         ProjectArenaDebug::LexToString(ArenaRunState.RunMode),
         ProjectArenaDebug::LexToString(ArenaRunState.Result),
         ArenaRunState.WaveState.CurrentWave,
         ArenaRunState.WaveState.TotalWaves,
         ArenaRunState.WaveState.AliveThreats,
         ArenaRunState.WaveState.SpawnedThreats);
  BP_OnArenaRunStateChanged(ArenaRunState);
}

void AArenaGameState::OnRep_ArenaRunState() {
  UE_LOG(LogArenaGameState, Display,
         TEXT("OnRep_ArenaRunState NetMode=%s Phase=%s Mode=%s Result=%s Wave=%d/%d Alive=%d Spawned=%d"),
         ProjectArenaDebug::LexToString(GetNetMode()),
         ProjectArenaDebug::LexToString(ArenaRunState.Phase),
         ProjectArenaDebug::LexToString(ArenaRunState.RunMode),
         ProjectArenaDebug::LexToString(ArenaRunState.Result),
         ArenaRunState.WaveState.CurrentWave,
         ArenaRunState.WaveState.TotalWaves,
         ArenaRunState.WaveState.AliveThreats,
         ArenaRunState.WaveState.SpawnedThreats);
  BP_OnArenaRunStateChanged(ArenaRunState);
}
