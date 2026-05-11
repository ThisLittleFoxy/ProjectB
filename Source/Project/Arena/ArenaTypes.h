// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ArenaTypes.generated.h"

UENUM(BlueprintType)
enum class EArenaRunMode : uint8 {
  Solo UMETA(DisplayName = "Solo"),
  Duo UMETA(DisplayName = "Duo")
};

UENUM(BlueprintType)
enum class EArenaPhase : uint8 {
  None UMETA(DisplayName = "None"),
  Lobby UMETA(DisplayName = "Lobby"),
  Countdown UMETA(DisplayName = "Countdown"),
  Combat UMETA(DisplayName = "Combat"),
  Intermission UMETA(DisplayName = "Intermission"),
  Result UMETA(DisplayName = "Result")
};

UENUM(BlueprintType)
enum class EArenaRunResult : uint8 {
  None UMETA(DisplayName = "None"),
  Completed UMETA(DisplayName = "Completed"),
  Failed UMETA(DisplayName = "Failed"),
  Aborted UMETA(DisplayName = "Aborted")
};

namespace ProjectArenaDebug {
FORCEINLINE const TCHAR *LexToString(EArenaRunMode RunMode) {
  switch (RunMode) {
  case EArenaRunMode::Solo:
    return TEXT("Solo");
  case EArenaRunMode::Duo:
    return TEXT("Duo");
  default:
    return TEXT("Unknown");
  }
}

FORCEINLINE const TCHAR *LexToString(EArenaPhase Phase) {
  switch (Phase) {
  case EArenaPhase::None:
    return TEXT("None");
  case EArenaPhase::Lobby:
    return TEXT("Lobby");
  case EArenaPhase::Countdown:
    return TEXT("Countdown");
  case EArenaPhase::Combat:
    return TEXT("Combat");
  case EArenaPhase::Intermission:
    return TEXT("Intermission");
  case EArenaPhase::Result:
    return TEXT("Result");
  default:
    return TEXT("Unknown");
  }
}

FORCEINLINE const TCHAR *LexToString(EArenaRunResult Result) {
  switch (Result) {
  case EArenaRunResult::None:
    return TEXT("None");
  case EArenaRunResult::Completed:
    return TEXT("Completed");
  case EArenaRunResult::Failed:
    return TEXT("Failed");
  case EArenaRunResult::Aborted:
    return TEXT("Aborted");
  default:
    return TEXT("Unknown");
  }
}

FORCEINLINE const TCHAR *LexToString(ENetMode NetMode) {
  switch (NetMode) {
  case NM_Standalone:
    return TEXT("Standalone");
  case NM_DedicatedServer:
    return TEXT("DedicatedServer");
  case NM_ListenServer:
    return TEXT("ListenServer");
  case NM_Client:
    return TEXT("Client");
  default:
    return TEXT("UnknownNetMode");
  }
}
} // namespace ProjectArenaDebug

USTRUCT(BlueprintType)
struct PROJECT_API FArenaWaveState {
  GENERATED_BODY()

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Arena|Wave",
            meta = (ClampMin = "0"))
  int32 CurrentWave = 0;

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Arena|Wave",
            meta = (ClampMin = "0"))
  int32 TotalWaves = 0;

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Arena|Wave",
            meta = (ClampMin = "0"))
  int32 AliveThreats = 0;

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Arena|Wave",
            meta = (ClampMin = "0"))
  int32 SpawnedThreats = 0;

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Arena|Wave")
  bool bWaveCompleted = false;

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Arena|Wave")
  bool bWaveFailed = false;
};

USTRUCT(BlueprintType)
struct PROJECT_API FArenaRunState {
  GENERATED_BODY()

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Arena|Run")
  EArenaRunMode RunMode = EArenaRunMode::Solo;

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Arena|Run")
  EArenaPhase Phase = EArenaPhase::None;

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Arena|Run")
  EArenaRunResult Result = EArenaRunResult::None;

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Arena|Run",
            meta = (ClampMin = "0.0"))
  float ElapsedSeconds = 0.0f;

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Arena|Run")
  bool bRunActive = false;

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Arena|Run")
  bool bWaveActive = false;

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Arena|Run")
  bool bRunCompleted = false;

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Arena|Run")
  bool bRunFailed = false;

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Arena|Run")
  FArenaWaveState WaveState;
};

USTRUCT(BlueprintType)
struct PROJECT_API FArenaPlayerRunStats {
  GENERATED_BODY()

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Arena|Player",
            meta = (ClampMin = "0"))
  int32 Kills = 0;

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Arena|Player",
            meta = (ClampMin = "0"))
  int32 Deaths = 0;

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Arena|Player",
            meta = (ClampMin = "0"))
  int32 SpendableCurrency = 0;

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Arena|Player",
            meta = (ClampMin = "0"))
  int32 EarnedCurrency = 0;

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Arena|Player",
            meta = (ClampMin = "0"))
  int32 CommittedCurrency = 0;

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Arena|Player",
            meta = (ClampMin = "0"))
  int32 ReachedWave = 0;
};
