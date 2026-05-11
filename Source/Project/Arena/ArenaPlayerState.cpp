// Copyright Epic Games, Inc. All Rights Reserved.

#include "Arena/ArenaPlayerState.h"
#include "Net/UnrealNetwork.h"

DEFINE_LOG_CATEGORY_STATIC(LogArenaPlayerState, Log, All);

AArenaPlayerState::AArenaPlayerState() { bReplicates = true; }

void AArenaPlayerState::GetLifetimeReplicatedProps(
    TArray<FLifetimeProperty> &OutLifetimeProps) const {
  Super::GetLifetimeReplicatedProps(OutLifetimeProps);

  DOREPLIFETIME(AArenaPlayerState, ArenaRunStats);
  DOREPLIFETIME(AArenaPlayerState, bArenaReady);
  DOREPLIFETIME(AArenaPlayerState, bArenaAlive);
}

void AArenaPlayerState::SetArenaRunStats(
    const FArenaPlayerRunStats &NewRunStats) {
  ArenaRunStats = NewRunStats;
  UE_LOG(LogArenaPlayerState, Display,
         TEXT("SetArenaRunStats NetMode=%s Player=%s Kills=%d Deaths=%d Spendable=%d Earned=%d Committed=%d ReachedWave=%d"),
         ProjectArenaDebug::LexToString(GetNetMode()), *GetPlayerName(),
         ArenaRunStats.Kills, ArenaRunStats.Deaths,
         ArenaRunStats.SpendableCurrency, ArenaRunStats.EarnedCurrency,
         ArenaRunStats.CommittedCurrency, ArenaRunStats.ReachedWave);
  BP_OnArenaRunStatsChanged(ArenaRunStats);
}

int32 AArenaPlayerState::AddArenaEarnedCurrency(int32 Amount) {
  if (!HasAuthority()) {
    UE_LOG(LogArenaPlayerState, Warning,
           TEXT("AddArenaEarnedCurrency ignored on non-authority NetMode=%s Player=%s Amount=%d"),
           ProjectArenaDebug::LexToString(GetNetMode()), *GetPlayerName(),
           Amount);
    return ArenaRunStats.SpendableCurrency;
  }

  const int32 SanitizedAmount = FMath::Max(0, Amount);
  if (SanitizedAmount == 0) {
    return ArenaRunStats.SpendableCurrency;
  }

  FArenaPlayerRunStats NewStats = ArenaRunStats;
  NewStats.SpendableCurrency += SanitizedAmount;
  NewStats.EarnedCurrency += SanitizedAmount;
  SetArenaRunStats(NewStats);
  return ArenaRunStats.SpendableCurrency;
}

bool AArenaPlayerState::TrySpendArenaCurrency(int32 Amount) {
  if (!HasAuthority()) {
    UE_LOG(LogArenaPlayerState, Warning,
           TEXT("TrySpendArenaCurrency ignored on non-authority NetMode=%s Player=%s Amount=%d"),
           ProjectArenaDebug::LexToString(GetNetMode()), *GetPlayerName(),
           Amount);
    return false;
  }

  if (Amount <= 0 || ArenaRunStats.SpendableCurrency < Amount) {
    return false;
  }

  FArenaPlayerRunStats NewStats = ArenaRunStats;
  NewStats.SpendableCurrency -= Amount;
  SetArenaRunStats(NewStats);
  return true;
}

int32 AArenaPlayerState::CommitArenaEarnedCurrency() {
  if (!HasAuthority()) {
    UE_LOG(LogArenaPlayerState, Warning,
           TEXT("CommitArenaEarnedCurrency ignored on non-authority NetMode=%s Player=%s"),
           ProjectArenaDebug::LexToString(GetNetMode()), *GetPlayerName());
    return ArenaRunStats.CommittedCurrency;
  }

  FArenaPlayerRunStats NewStats = ArenaRunStats;
  NewStats.CommittedCurrency =
      FMath::Max(NewStats.CommittedCurrency, NewStats.EarnedCurrency);
  SetArenaRunStats(NewStats);
  return ArenaRunStats.CommittedCurrency;
}

void AArenaPlayerState::SetArenaRuntimeCurrency(int32 NewSpendableCurrency,
                                                int32 NewEarnedCurrency,
                                                int32 NewCommittedCurrency) {
  if (!HasAuthority()) {
    UE_LOG(LogArenaPlayerState, Warning,
           TEXT("SetArenaRuntimeCurrency ignored on non-authority NetMode=%s Player=%s"),
           ProjectArenaDebug::LexToString(GetNetMode()), *GetPlayerName());
    return;
  }

  FArenaPlayerRunStats NewStats = ArenaRunStats;
  NewStats.SpendableCurrency = FMath::Max(0, NewSpendableCurrency);
  NewStats.EarnedCurrency = FMath::Max(0, NewEarnedCurrency);
  NewStats.CommittedCurrency = FMath::Max(0, NewCommittedCurrency);
  SetArenaRunStats(NewStats);
}

void AArenaPlayerState::SetArenaReady(bool bNewReady) {
  if (bArenaReady == bNewReady) {
    return;
  }

  bArenaReady = bNewReady;
  UE_LOG(LogArenaPlayerState, Display,
         TEXT("SetArenaReady NetMode=%s Player=%s Ready=%s"),
         ProjectArenaDebug::LexToString(GetNetMode()), *GetPlayerName(),
         bArenaReady ? TEXT("true") : TEXT("false"));
  BP_OnArenaReadyChanged(bArenaReady);
}

void AArenaPlayerState::SetArenaAlive(bool bNewAlive) {
  if (bArenaAlive == bNewAlive) {
    return;
  }

  bArenaAlive = bNewAlive;
  UE_LOG(LogArenaPlayerState, Display,
         TEXT("SetArenaAlive NetMode=%s Player=%s Alive=%s"),
         ProjectArenaDebug::LexToString(GetNetMode()), *GetPlayerName(),
         bArenaAlive ? TEXT("true") : TEXT("false"));
  BP_OnArenaAliveChanged(bArenaAlive);
}

void AArenaPlayerState::ResetArenaRunStats() {
  ArenaRunStats = FArenaPlayerRunStats();
  UE_LOG(LogArenaPlayerState, Display,
         TEXT("ResetArenaRunStats NetMode=%s Player=%s"),
         ProjectArenaDebug::LexToString(GetNetMode()), *GetPlayerName());
  BP_OnArenaRunStatsChanged(ArenaRunStats);
}

void AArenaPlayerState::OnRep_ArenaRunStats() {
  UE_LOG(LogArenaPlayerState, Display,
         TEXT("OnRep_ArenaRunStats NetMode=%s Player=%s Kills=%d Deaths=%d Spendable=%d Earned=%d Committed=%d ReachedWave=%d"),
         ProjectArenaDebug::LexToString(GetNetMode()), *GetPlayerName(),
         ArenaRunStats.Kills, ArenaRunStats.Deaths,
         ArenaRunStats.SpendableCurrency, ArenaRunStats.EarnedCurrency,
         ArenaRunStats.CommittedCurrency, ArenaRunStats.ReachedWave);
  BP_OnArenaRunStatsChanged(ArenaRunStats);
}

void AArenaPlayerState::OnRep_ArenaReady() {
  UE_LOG(LogArenaPlayerState, Display,
         TEXT("OnRep_ArenaReady NetMode=%s Player=%s Ready=%s"),
         ProjectArenaDebug::LexToString(GetNetMode()), *GetPlayerName(),
         bArenaReady ? TEXT("true") : TEXT("false"));
  BP_OnArenaReadyChanged(bArenaReady);
}

void AArenaPlayerState::OnRep_ArenaAlive() {
  UE_LOG(LogArenaPlayerState, Display,
         TEXT("OnRep_ArenaAlive NetMode=%s Player=%s Alive=%s"),
         ProjectArenaDebug::LexToString(GetNetMode()), *GetPlayerName(),
         bArenaAlive ? TEXT("true") : TEXT("false"));
  BP_OnArenaAliveChanged(bArenaAlive);
}
