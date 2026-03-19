#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Save/ProjectSaveTypes.h"
#include "ProjectSaveSubsystem.generated.h"

class AMainPlayerController;
class UProjectSaveGame;
class UProjectSaveIndex;
class USaveGame;
class USaveableActorComponent;
struct FProjectSaveSlotMetadata;

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

  UFUNCTION(BlueprintCallable, Category = "Save")
  bool QuickSave();

  UFUNCTION(BlueprintCallable, Category = "Save")
  bool QuickLoad();

  UFUNCTION(BlueprintCallable, Category = "Save")
  bool ManualSave();

  UFUNCTION(BlueprintCallable, Category = "Save")
  bool ManualLoad();

  void RegisterDestroyedActorRecord(const FString &MapName,
                                    const FWorldActorSaveData &SaveRecord);
  void ClearDestroyedActorRecord(const FString &MapName, const FGuid &SaveId);

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
  TMap<FString, TMap<FGuid, FWorldActorSaveData>> DestroyedActorRecordsByMap;
  FString PendingSaveSlotName;
  EProjectSaveSlotKind PendingSaveSlotKind = EProjectSaveSlotKind::Quick;
  bool bIsShuttingDown = false;

  bool SaveToSlot(EProjectSaveSlotKind SlotKind);
  bool LoadLatestSave();
  FString BuildUniqueSaveSlotName(EProjectSaveSlotKind SlotKind,
                                  const FDateTime &Timestamp) const;
  FString GetIndexSlotName() const;
  FString GetCurrentMapName(UWorld *World) const;

  UProjectSaveGame *BuildSaveGameObject(EProjectSaveSlotKind SlotKind);
  UProjectSaveIndex *LoadOrCreateSaveIndex() const;
  bool SaveSaveIndex(UProjectSaveIndex *SaveIndex) const;
  bool FindMostRecentSaveMetadata(FProjectSaveSlotMetadata &OutMetadata,
                                  bool bFilterByKind = false,
                                  EProjectSaveSlotKind SlotKind = EProjectSaveSlotKind::Quick) const;
  void AddSaveSlotToIndex(const FString &SlotName, EProjectSaveSlotKind SlotKind,
                          const FString &MapName, const FDateTime &SavedAtUtc);
  bool GatherPlayerSaveData(UWorld *World, FPlayerSaveData &OutPlayerSaveData) const;
  void GatherWorldActorSaveData(UWorld *World,
                                TArray<FWorldActorSaveData> &OutWorldActorRecords);
  bool ResolveRestoreContext(UWorld *World, AMainPlayerController *&OutController,
                             APawn *&OutPawn) const;
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

  void HandlePostLoadMapWithWorld(UWorld *LoadedWorld);
};
