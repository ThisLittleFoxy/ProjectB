// Copyright Epic Games, Inc. All Rights Reserved.

#include "Arena/Debug/ArenaRuntimeCurrencyDebugTest.h"

#include "Arena/ArenaPlayerState.h"
#include "Arena/ArenaTypes.h"

DEFINE_LOG_CATEGORY_STATIC(LogArenaCurrencyDebug, Log, All);

bool FArenaRuntimeCurrencyDebugTest::Run(AArenaPlayerState &PlayerState) {
  if (!PlayerState.HasAuthority()) {
    UE_LOG(LogArenaCurrencyDebug, Warning,
           TEXT("Currency debug test skipped on non-authority NetMode=%s Player=%s"),
           ProjectArenaDebug::LexToString(PlayerState.GetNetMode()),
           *PlayerState.GetPlayerName());
    return false;
  }

  const FArenaPlayerRunStats OriginalStats = PlayerState.GetArenaRunStats();
  bool bPassed = true;

  UE_LOG(LogArenaCurrencyDebug, Display,
         TEXT("Currency debug test started NetMode=%s Player=%s"),
         ProjectArenaDebug::LexToString(PlayerState.GetNetMode()),
         *PlayerState.GetPlayerName());

  PlayerState.ResetArenaRunStats();
  bPassed &= ExpectStats(TEXT("Reset"), PlayerState.GetArenaRunStats(), 0, 0, 0);

  PlayerState.AddArenaEarnedCurrency(100);
  bPassed &= ExpectStats(TEXT("Add 100"), PlayerState.GetArenaRunStats(), 100,
                         100, 0);

  const bool bSpend35 = PlayerState.TrySpendArenaCurrency(35);
  if (!bSpend35) {
    UE_LOG(LogArenaCurrencyDebug, Error,
           TEXT("Spend 35 failed unexpectedly"));
    bPassed = false;
  }
  bPassed &= ExpectStats(TEXT("Spend 35"), PlayerState.GetArenaRunStats(), 65,
                         100, 0);

  const bool bSpend999 = PlayerState.TrySpendArenaCurrency(999);
  if (bSpend999) {
    UE_LOG(LogArenaCurrencyDebug, Error,
           TEXT("Spend 999 succeeded unexpectedly"));
    bPassed = false;
  } else {
    UE_LOG(LogArenaCurrencyDebug, Display,
           TEXT("Spend 999 rejected OK"));
  }
  bPassed &= ExpectStats(TEXT("Reject spend 999"), PlayerState.GetArenaRunStats(),
                         65, 100, 0);

  PlayerState.CommitArenaEarnedCurrency();
  bPassed &= ExpectStats(TEXT("Commit"), PlayerState.GetArenaRunStats(), 65,
                         100, 100);

  PlayerState.SetArenaRunStats(OriginalStats);

  UE_LOG(LogArenaCurrencyDebug, Display, TEXT("Currency debug test %s"),
         bPassed ? TEXT("PASSED") : TEXT("FAILED"));
  return bPassed;
}

bool FArenaRuntimeCurrencyDebugTest::ExpectStats(
    const TCHAR *Step, const FArenaPlayerRunStats &ActualStats,
    int32 ExpectedSpendable, int32 ExpectedEarned, int32 ExpectedCommitted) {
  const bool bMatches =
      ActualStats.SpendableCurrency == ExpectedSpendable &&
      ActualStats.EarnedCurrency == ExpectedEarned &&
      ActualStats.CommittedCurrency == ExpectedCommitted;

  if (bMatches) {
    UE_LOG(LogArenaCurrencyDebug, Display,
           TEXT("%s OK Spendable=%d/%d Earned=%d/%d Committed=%d/%d"), Step,
           ActualStats.SpendableCurrency, ExpectedSpendable,
           ActualStats.EarnedCurrency, ExpectedEarned,
           ActualStats.CommittedCurrency, ExpectedCommitted);
  } else {
    UE_LOG(LogArenaCurrencyDebug, Error,
           TEXT("%s FAILED Spendable=%d/%d Earned=%d/%d Committed=%d/%d"),
           Step, ActualStats.SpendableCurrency, ExpectedSpendable,
           ActualStats.EarnedCurrency, ExpectedEarned,
           ActualStats.CommittedCurrency, ExpectedCommitted);
  }

  return bMatches;
}
