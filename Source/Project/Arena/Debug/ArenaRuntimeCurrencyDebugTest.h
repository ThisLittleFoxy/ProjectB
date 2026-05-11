// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

class AArenaPlayerState;
struct FArenaPlayerRunStats;

class PROJECT_API FArenaRuntimeCurrencyDebugTest {
public:
  static bool Run(AArenaPlayerState &PlayerState);

private:
  static bool ExpectStats(const TCHAR *Step,
                          const FArenaPlayerRunStats &ActualStats,
                          int32 ExpectedSpendable, int32 ExpectedEarned,
                          int32 ExpectedCommitted);
};
