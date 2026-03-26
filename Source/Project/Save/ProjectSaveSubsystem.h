#pragma once

#include "CoreMinimal.h"
#include "Save/ProjectSaveIndex.h"
#include "Save/ProjectSaveTypes.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "UObject/ObjectKey.h"
#include "ProjectSaveSubsystem.generated.h"

class AMainPlayerController;
class UProjectSaveGame;
class UProjectSaveIndex;
class USaveGame;
class USaveableActorComponent;

UENUM(BlueprintType)
enum class EProjectSaveOperationState : uint8 {
  Idle UMETA(DisplayName = "Idle"),
  Saving UMETA(DisplayName = "Saving"),
  LoadingSlot UMETA(DisplayName = "Loading Slot"),
  OpeningLevel UMETA(DisplayName = "Opening Level"),
  Restoring UMETA(DisplayName = "Restoring")
};

UCLASS()
class PROJECT_API UProjectSaveSubsystem : public UGameInstanceSubsystem {
  GENERATED_BODY()

public:
  virtual void Initialize(FSubsystemCollectionBase &Collection) override;
  virtual void Deinitialize() override;

  UFUNCTION(BlueprintPure, Category = "Save")
  bool CanStartOperation() const;

  UFUNCTION(BlueprintPure, Category = "Save")
  bool HasQuickSave() const;

  UFUNCTION(BlueprintPure, Category = "Save")
  bool HasManualSave() const;

  UFUNCTION(BlueprintPure, Category = "Save")
  bool HasPendingRestore() const;

  UFUNCTION(BlueprintPure, Category = "Save")
  EProjectSaveOperationState GetOperationState() const {
    return OperationState;
  }

  UFUNCTION(BlueprintPure, Category = "Save")
  float GetOperationProgress() const;

  UFUNCTION(BlueprintCallable, Category = "Save")
  bool QuickSave();

  UFUNCTION(BlueprintCallable, Category = "Save")
  bool QuickLoad();

  UFUNCTION(BlueprintCallable, Category = "Save")
  bool ManualSave();

  UFUNCTION(BlueprintCallable, Category = "Save")
  bool ManualLoad();

  UFUNCTION(BlueprintCallable, Category = "Save")
  bool OverwriteSaveSlot(const FString &SlotName);

  UFUNCTION(BlueprintCallable, Category = "Save")
  bool DeleteSaveSlot(const FString &SlotName);

  UFUNCTION(BlueprintCallable, Category = "Save")
  bool ContinueFromLatestSave();

  UFUNCTION(BlueprintCallable, Category = "Save")
  bool LoadSaveSlot(const FString &SlotName);

  UFUNCTION(BlueprintCallable, Category = "Save")
  bool GetLatestSaveMetadata(FProjectSaveSlotMetadata &OutMetadata) const;

  UFUNCTION(BlueprintCallable, Category = "Save")
  void GetAllSaveMetadata(TArray<FProjectSaveSlotMetadata> &OutMetadata) const;

  void RegisterDestroyedActorRecord(const FString &MapName,
                                    const FWorldActorSaveData &SaveRecord);
  void ClearDestroyedActorRecord(const FString &MapName,
                                 const FGuid &PersistentId);

public:
  struct FResolvedWorldSaveTarget {
    TWeakObjectPtr<USaveableActorComponent> SaveableComponent;
    EProjectWorldSavePolicy Policy = EProjectWorldSavePolicy::None;
    bool bSaveHealthState = false;
    bool bSaveDestroyedState = false;
    bool bSaveTransformState = false;
    bool bSaveCustomData = false;

    bool UsesComponent() const { return SaveableComponent.IsValid(); }
    bool IsPersistent() const {
      return UsesComponent() || Policy != EProjectWorldSavePolicy::None;
    }
  };

  struct FTrackedWorldActorContext {
    FString MapName;
    FGuid PersistentId;
    FResolvedWorldSaveTarget ResolvedTarget;
  };

private:
  static constexpr int32 SaveUserIndex = 0;
  static constexpr int32 RestoreRetryLimit = 50;

  UPROPERTY(Transient)
  TObjectPtr<UProjectSaveGame> ActiveSaveObject;

  UPROPERTY(Transient)
  TObjectPtr<UProjectSaveGame> PendingLoadedSaveGame;

  UPROPERTY(Transient)
  EProjectSaveOperationState OperationState = EProjectSaveOperationState::Idle;

  TSharedPtr<struct FStreamableHandle> PendingAssetLoadHandle;
  FDelegateHandle PostLoadMapHandle;
  FDelegateHandle EnginePreExitHandle;
  TMap<FString, TMap<FGuid, FWorldActorSaveData>> DefaultWorldActorRecordsByMap;
  TMap<FString, TMap<FGuid, FWorldActorSaveData>> DestroyedActorRecordsByMap;
  TMap<FObjectKey, FTrackedWorldActorContext> TrackedWorldActorsByObjectKey;
  FString PendingSaveSlotName;
  EProjectSaveSlotKind PendingSaveSlotKind = EProjectSaveSlotKind::Quick;
  bool bIsShuttingDown = false;
  double RunPlayTimeSessionStartSeconds = 0.0;
  int64 LoadedRunPlayTimeSeconds = 0;

  bool BeginSaveSlot(EProjectSaveSlotKind SlotKind, const FString &SlotName);
  bool SaveToSlot(EProjectSaveSlotKind SlotKind);
  bool LoadLatestSave();
  bool LoadLatestSave(EProjectSaveSlotKind SlotKind);
  bool BeginLoadSlot(const FString &SlotName);
  FString BuildUniqueSaveSlotName(EProjectSaveSlotKind SlotKind,
                                  const FDateTime &Timestamp) const;
  FString GetIndexSlotName() const;
  FString GetCurrentMapName(UWorld *World) const;
  int64 GetCurrentRunPlayTimeSeconds() const;
  void ResetRunPlayTimeTracking(int64 LoadedPlayTimeSeconds = 0);
  void BuildRunMetaSaveData(FRunMetaSaveData &OutRunMeta) const;

  UProjectSaveGame *BuildSaveGameObject(EProjectSaveSlotKind SlotKind);
  UProjectSaveIndex *LoadOrCreateSaveIndex() const;
  bool SaveSaveIndex(UProjectSaveIndex *SaveIndex) const;
  bool RemoveSaveSlotFromIndex(const FString &SlotName) const;
  bool TryResolveExistingSlotKind(const FString &SlotName,
                                  EProjectSaveSlotKind &OutSlotKind) const;
  bool FindMostRecentSaveMetadata(
      FProjectSaveSlotMetadata &OutMetadata, bool bFilterByKind = false,
      EProjectSaveSlotKind SlotKind = EProjectSaveSlotKind::Quick) const;
  void AddSaveSlotToIndex(const FString &SlotName, EProjectSaveSlotKind SlotKind,
                          const FString &MapName, const FDateTime &SavedAtUtc,
                          const FRunMetaSaveData &RunMeta);
  bool GatherPlayerSaveData(UWorld *World,
                            FPlayerSaveData &OutPlayerSaveData) const;
  void GatherWorldActorSaveData(UWorld *World,
                                TArray<FWorldActorSaveData> &OutWorldActorRecords);
  void PrimeWorldStateTracking(UWorld *World);
  bool ResolveRestoreContext(UWorld *World, AMainPlayerController *&OutController,
                             APawn *&OutPawn) const;
  bool ResolveWorldSaveTarget(AActor *Actor, FGuid &OutPersistentId,
                              FResolvedWorldSaveTarget &OutResolvedTarget) const;
  bool TryBuildPlacedActorPersistentId(const AActor *Actor,
                                       FGuid &OutPersistentId) const;
  bool IsRuleBasedActorTracked(const FString &MapName,
                               const FGuid &PersistentId) const;
  void BuildRuleBasedSaveRecord(
      AActor *Actor, const FGuid &PersistentId,
      const FResolvedWorldSaveTarget &ResolvedTarget,
      FWorldActorSaveData &OutSaveRecord) const;
  void ApplyRuleBasedSaveRecord(
      AActor *Actor, const FWorldActorSaveData &SaveRecord,
      const FResolvedWorldSaveTarget &ResolvedTarget) const;
  bool CaptureCustomDataForActor(
      const AActor *Actor,
      FSaveableActorCustomData &OutCustomData) const;
  bool IsWorldRecordAtDefaultState(const FString &MapName,
                                   const FWorldActorSaveData &SaveRecord) const;
  bool AreWorldRecordsEquivalent(const FWorldActorSaveData &Left,
                                 const FWorldActorSaveData &Right) const;
  void StartPendingRestore(UWorld *World, int32 AttemptIndex);
  void ContinuePendingRestore(UWorld *World);
  void RestoreWorldActorState(UWorld *World, const UProjectSaveGame *SaveGameObject);
  void RestorePlayerState(UWorld *World, const UProjectSaveGame *SaveGameObject,
                          AMainPlayerController *Controller, APawn *Pawn);
  void QueueWeaponClassPreload(UWorld *World);
  void FinishPendingRestore(UWorld *World);
  void ResetOperationState();
  void HandleShutdownRequested();
  void ShowStatusMessage(const FString &Message, FColor Color = FColor::White) const;

  UFUNCTION()
  void HandleAsyncSaveComplete(const FString &SlotName, const int32 UserIndex,
                               bool bSuccess);

  UFUNCTION()
  void HandleAsyncLoadComplete(const FString &SlotName, const int32 UserIndex,
                               USaveGame *LoadedGameData);

  UFUNCTION()
  void HandleTrackedActorDestroyed(AActor *DestroyedActor);

  void HandlePostLoadMapWithWorld(UWorld *LoadedWorld);
};
