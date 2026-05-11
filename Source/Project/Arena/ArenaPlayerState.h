// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Arena/ArenaTypes.h"
#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"
#include "ArenaPlayerState.generated.h"

UCLASS(BlueprintType, Blueprintable)
class PROJECT_API AArenaPlayerState : public APlayerState {
  GENERATED_BODY()

public:
  AArenaPlayerState();

  virtual void GetLifetimeReplicatedProps(
      TArray<FLifetimeProperty> &OutLifetimeProps) const override;

  void SetArenaRunStats(const FArenaPlayerRunStats &NewRunStats);

  UFUNCTION(BlueprintCallable, Category = "Arena|Player")
  void ResetArenaRunStats();

  UFUNCTION(BlueprintCallable, Category = "Arena|Currency")
  int32 AddArenaEarnedCurrency(int32 Amount);

  UFUNCTION(BlueprintCallable, Category = "Arena|Currency")
  bool TrySpendArenaCurrency(int32 Amount);

  UFUNCTION(BlueprintCallable, Category = "Arena|Currency")
  int32 CommitArenaEarnedCurrency();

  UFUNCTION(BlueprintCallable, Category = "Arena|Currency")
  void SetArenaRuntimeCurrency(int32 NewSpendableCurrency,
                               int32 NewEarnedCurrency,
                               int32 NewCommittedCurrency);

  UFUNCTION(BlueprintCallable, Category = "Arena|Ready")
  void SetArenaReady(bool bNewReady);

  UFUNCTION(BlueprintPure, Category = "Arena|Ready")
  bool IsArenaReady() const { return bArenaReady; }

  UFUNCTION(BlueprintCallable, Category = "Arena|Player")
  void SetArenaAlive(bool bNewAlive);

  UFUNCTION(BlueprintPure, Category = "Arena|Player")
  bool IsArenaAlive() const { return bArenaAlive; }

  UFUNCTION(BlueprintPure, Category = "Arena|Player")
  FArenaPlayerRunStats GetArenaRunStats() const { return ArenaRunStats; }

  UFUNCTION(BlueprintPure, Category = "Arena|Player")
  int32 GetKills() const { return ArenaRunStats.Kills; }

  UFUNCTION(BlueprintPure, Category = "Arena|Player")
  int32 GetDeaths() const { return ArenaRunStats.Deaths; }

  UFUNCTION(BlueprintPure, Category = "Arena|Currency")
  int32 GetSpendableCurrency() const {
    return ArenaRunStats.SpendableCurrency;
  }

  UFUNCTION(BlueprintPure, Category = "Arena|Player")
  int32 GetEarnedCurrency() const { return ArenaRunStats.EarnedCurrency; }

  UFUNCTION(BlueprintPure, Category = "Arena|Player")
  int32 GetCommittedCurrency() const { return ArenaRunStats.CommittedCurrency; }

  UFUNCTION(BlueprintPure, Category = "Arena|Player")
  int32 GetReachedWave() const { return ArenaRunStats.ReachedWave; }

  UFUNCTION(BlueprintImplementableEvent, Category = "Arena|Player",
            meta = (DisplayName = "On Arena Run Stats Changed"))
  void BP_OnArenaRunStatsChanged(const FArenaPlayerRunStats &NewRunStats);

  UFUNCTION(BlueprintImplementableEvent, Category = "Arena|Ready",
            meta = (DisplayName = "On Arena Ready Changed"))
  void BP_OnArenaReadyChanged(bool bNewReady);

  UFUNCTION(BlueprintImplementableEvent, Category = "Arena|Player",
            meta = (DisplayName = "On Arena Alive Changed"))
  void BP_OnArenaAliveChanged(bool bNewAlive);

protected:
  UPROPERTY(ReplicatedUsing = OnRep_ArenaRunStats, BlueprintReadOnly,
            Category = "Arena|Player", meta = (AllowPrivateAccess = "true"))
  FArenaPlayerRunStats ArenaRunStats;

  UPROPERTY(ReplicatedUsing = OnRep_ArenaReady, BlueprintReadOnly,
            Category = "Arena|Ready", meta = (AllowPrivateAccess = "true"))
  bool bArenaReady = false;

  UPROPERTY(ReplicatedUsing = OnRep_ArenaAlive, BlueprintReadOnly,
            Category = "Arena|Player", meta = (AllowPrivateAccess = "true"))
  bool bArenaAlive = true;

private:
  UFUNCTION()
  void OnRep_ArenaRunStats();

  UFUNCTION()
  void OnRep_ArenaReady();

  UFUNCTION()
  void OnRep_ArenaAlive();
};
