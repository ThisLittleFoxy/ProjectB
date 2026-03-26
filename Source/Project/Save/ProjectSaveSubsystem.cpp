#include "Save/ProjectSaveSubsystem.h"

#include "Character/HealthComponent.h"
#include "Controllers/MainPlayerController.h"
#include "Engine/AssetManager.h"
#include "Engine/Engine.h"
#include "Engine/StreamableManager.h"
#include "EngineUtils.h"
#include "GameFramework/Pawn.h"
#include "Inventory/PlayerArmoryComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/App.h"
#include "Misc/CoreDelegates.h"
#include "Misc/EngineVersion.h"
#include "Project.h"
#include "Save/ProjectSaveGame.h"
#include "Save/ProjectWorldSaveProfile.h"
#include "Save/SaveableActorComponent.h"
#include "Save/SaveableActorInterface.h"

namespace {
constexpr uint64 SaveStatusMessageKey = 0x50524F4A53415645ULL;

bool RegisterUniquePersistentId(TMap<FGuid, AActor *> &ActorByPersistentId,
                                const FGuid &PersistentId, AActor *Actor) {
  if (!PersistentId.IsValid() || !Actor) {
    return false;
  }

  if (AActor **ExistingActor = ActorByPersistentId.Find(PersistentId)) {
    UE_LOG(LogProject, Error,
           TEXT("ProjectSaveSubsystem: duplicate PersistentId '%s' on '%s' and '%s'"),
           *PersistentId.ToString(), *GetNameSafe(*ExistingActor),
           *GetNameSafe(Actor));
    return false;
  }

  ActorByPersistentId.Add(PersistentId, Actor);
  return true;
}

struct FRestorableWorldActorTarget {
  TWeakObjectPtr<AActor> Actor;
  UProjectSaveSubsystem::FResolvedWorldSaveTarget ResolvedTarget;
};
} // namespace

void UProjectSaveSubsystem::Initialize(FSubsystemCollectionBase &Collection) {
  Super::Initialize(Collection);
  ResetRunPlayTimeTracking();
  PostLoadMapHandle =
      FCoreUObjectDelegates::PostLoadMapWithWorld.AddUObject(
          this, &UProjectSaveSubsystem::HandlePostLoadMapWithWorld);
  EnginePreExitHandle =
      FCoreDelegates::OnEnginePreExit.AddUObject(
          this, &UProjectSaveSubsystem::HandleShutdownRequested);
}

void UProjectSaveSubsystem::Deinitialize() {
  HandleShutdownRequested();

  if (PostLoadMapHandle.IsValid()) {
    FCoreUObjectDelegates::PostLoadMapWithWorld.Remove(PostLoadMapHandle);
    PostLoadMapHandle.Reset();
  }

  if (EnginePreExitHandle.IsValid()) {
    FCoreDelegates::OnEnginePreExit.Remove(EnginePreExitHandle);
    EnginePreExitHandle.Reset();
  }

  DefaultWorldActorRecordsByMap.Reset();
  DestroyedActorRecordsByMap.Reset();
  TrackedWorldActorsByObjectKey.Reset();

  Super::Deinitialize();
}

bool UProjectSaveSubsystem::CanStartOperation() const {
  return !bIsShuttingDown && OperationState == EProjectSaveOperationState::Idle;
}

bool UProjectSaveSubsystem::HasQuickSave() const {
  FProjectSaveSlotMetadata Metadata;
  return FindMostRecentSaveMetadata(Metadata, true, EProjectSaveSlotKind::Quick);
}

bool UProjectSaveSubsystem::HasManualSave() const {
  FProjectSaveSlotMetadata Metadata;
  return FindMostRecentSaveMetadata(Metadata, true, EProjectSaveSlotKind::Manual);
}

bool UProjectSaveSubsystem::HasPendingRestore() const {
  return PendingLoadedSaveGame != nullptr &&
         (OperationState == EProjectSaveOperationState::OpeningLevel ||
          OperationState == EProjectSaveOperationState::Restoring);
}

float UProjectSaveSubsystem::GetOperationProgress() const {
  switch (OperationState) {
  case EProjectSaveOperationState::Idle:
    return 1.0f;
  case EProjectSaveOperationState::Saving:
    return 0.15f;
  case EProjectSaveOperationState::LoadingSlot:
    return 0.25f;
  case EProjectSaveOperationState::OpeningLevel:
    return 0.6f;
  case EProjectSaveOperationState::Restoring:
    if (PendingAssetLoadHandle.IsValid()) {
      return 0.75f + (PendingAssetLoadHandle->GetProgress() * 0.2f);
    }

    return 0.95f;
  default:
    return 0.0f;
  }
}

bool UProjectSaveSubsystem::QuickSave() {
  return SaveToSlot(EProjectSaveSlotKind::Quick);
}

bool UProjectSaveSubsystem::QuickLoad() {
  return LoadLatestSave(EProjectSaveSlotKind::Quick);
}

bool UProjectSaveSubsystem::ManualSave() {
  return SaveToSlot(EProjectSaveSlotKind::Manual);
}

bool UProjectSaveSubsystem::ManualLoad() {
  return LoadLatestSave(EProjectSaveSlotKind::Manual);
}

bool UProjectSaveSubsystem::OverwriteSaveSlot(const FString &SlotName) {
  if (SlotName.IsEmpty() || SlotName == GetIndexSlotName()) {
    UE_LOG(LogProject, Warning,
           TEXT("ProjectSaveSubsystem: refusing to overwrite invalid slot '%s'"),
           *SlotName);
    ShowStatusMessage(TEXT("Invalid save slot"), FColor::Yellow);
    return false;
  }

  if (!UGameplayStatics::DoesSaveGameExist(SlotName, SaveUserIndex)) {
    UE_LOG(LogProject, Warning,
           TEXT("ProjectSaveSubsystem: requested overwrite slot '%s' does not exist"),
           *SlotName);
    ShowStatusMessage(TEXT("Selected save was not found"), FColor::Yellow);
    return false;
  }

  EProjectSaveSlotKind SlotKind = EProjectSaveSlotKind::Manual;
  if (!TryResolveExistingSlotKind(SlotName, SlotKind)) {
    UE_LOG(LogProject, Warning,
           TEXT("ProjectSaveSubsystem: could not resolve slot kind for '%s'"),
           *SlotName);
    ShowStatusMessage(TEXT("Save overwrite failed"), FColor::Red);
    return false;
  }

  return BeginSaveSlot(SlotKind, SlotName);
}

bool UProjectSaveSubsystem::DeleteSaveSlot(const FString &SlotName) {
  if (!CanStartOperation()) {
    UE_LOG(LogProject, Warning,
           TEXT("ProjectSaveSubsystem: rejecting delete request because operation state is %d"),
           static_cast<int32>(OperationState));
    ShowStatusMessage(TEXT("Cannot delete right now"), FColor::Yellow);
    return false;
  }

  if (SlotName.IsEmpty() || SlotName == GetIndexSlotName()) {
    UE_LOG(LogProject, Warning,
           TEXT("ProjectSaveSubsystem: refusing to delete invalid slot '%s'"),
           *SlotName);
    ShowStatusMessage(TEXT("Invalid save slot"), FColor::Yellow);
    return false;
  }

  if (!UGameplayStatics::DoesSaveGameExist(SlotName, SaveUserIndex)) {
    UE_LOG(LogProject, Warning,
           TEXT("ProjectSaveSubsystem: requested delete slot '%s' does not exist"),
           *SlotName);
    ShowStatusMessage(TEXT("Selected save was not found"), FColor::Yellow);
    RemoveSaveSlotFromIndex(SlotName);
    return false;
  }

  if (!UGameplayStatics::DeleteGameInSlot(SlotName, SaveUserIndex)) {
    UE_LOG(LogProject, Error,
           TEXT("ProjectSaveSubsystem: failed to delete slot '%s'"),
           *SlotName);
    ShowStatusMessage(TEXT("Delete failed"), FColor::Red);
    return false;
  }

  if (!RemoveSaveSlotFromIndex(SlotName)) {
    UE_LOG(LogProject, Warning,
           TEXT("ProjectSaveSubsystem: slot '%s' was deleted, but index cleanup failed"),
           *SlotName);
  }

  ShowStatusMessage(TEXT("Save deleted"), FColor::Green);
  return true;
}

bool UProjectSaveSubsystem::ContinueFromLatestSave() {
  return LoadLatestSave();
}

bool UProjectSaveSubsystem::LoadSaveSlot(const FString &SlotName) {
  return BeginLoadSlot(SlotName);
}

bool UProjectSaveSubsystem::GetLatestSaveMetadata(
    FProjectSaveSlotMetadata &OutMetadata) const {
  return FindMostRecentSaveMetadata(OutMetadata);
}

void UProjectSaveSubsystem::GetAllSaveMetadata(
    TArray<FProjectSaveSlotMetadata> &OutMetadata) const {
  OutMetadata.Reset();

  UProjectSaveIndex *SaveIndex = LoadOrCreateSaveIndex();
  if (!SaveIndex) {
    return;
  }

  for (const FProjectSaveSlotMetadata &SaveSlot : SaveIndex->SaveSlots) {
    if (UGameplayStatics::DoesSaveGameExist(SaveSlot.SlotName, SaveUserIndex)) {
      OutMetadata.Add(SaveSlot);
    }
  }

  OutMetadata.Sort([](const FProjectSaveSlotMetadata &Left,
                      const FProjectSaveSlotMetadata &Right) {
    return Left.SavedAtUtc > Right.SavedAtUtc;
  });
}

void UProjectSaveSubsystem::RegisterDestroyedActorRecord(
    const FString &MapName, const FWorldActorSaveData &SaveRecord) {
  if (MapName.IsEmpty() || !SaveRecord.PersistentId.IsValid()) {
    return;
  }

  DestroyedActorRecordsByMap.FindOrAdd(MapName).Add(SaveRecord.PersistentId,
                                                    SaveRecord);
}

void UProjectSaveSubsystem::ClearDestroyedActorRecord(const FString &MapName,
                                                      const FGuid &PersistentId) {
  if (MapName.IsEmpty() || !PersistentId.IsValid()) {
    return;
  }

  if (TMap<FGuid, FWorldActorSaveData> *Records =
          DestroyedActorRecordsByMap.Find(MapName)) {
    Records->Remove(PersistentId);
  }
}

bool UProjectSaveSubsystem::BeginSaveSlot(EProjectSaveSlotKind SlotKind,
                                          const FString &SlotName) {
  if (!CanStartOperation()) {
    UE_LOG(LogProject, Warning,
           TEXT("ProjectSaveSubsystem: rejecting save request because operation state is %d"),
           static_cast<int32>(OperationState));
    ShowStatusMessage(TEXT("Cannot save right now"), FColor::Yellow);
    return false;
  }

  if (SlotName.IsEmpty() || SlotName == GetIndexSlotName()) {
    UE_LOG(LogProject, Warning,
           TEXT("ProjectSaveSubsystem: refusing to save into invalid slot '%s'"),
           *SlotName);
    ShowStatusMessage(TEXT("Save failed"), FColor::Red);
    return false;
  }

  ActiveSaveObject = BuildSaveGameObject(SlotKind);
  if (!ActiveSaveObject) {
    ShowStatusMessage(TEXT("Save failed"), FColor::Red);
    return false;
  }

  OperationState = EProjectSaveOperationState::Saving;
  PendingSaveSlotKind = SlotKind;
  PendingSaveSlotName = SlotName;
  ShowStatusMessage(TEXT("Saving..."), FColor::Yellow);

  FAsyncSaveGameToSlotDelegate SaveDelegate;
  SaveDelegate.BindUObject(this, &UProjectSaveSubsystem::HandleAsyncSaveComplete);
  UGameplayStatics::AsyncSaveGameToSlot(ActiveSaveObject, PendingSaveSlotName,
                                        SaveUserIndex, SaveDelegate);
  return true;
}

bool UProjectSaveSubsystem::SaveToSlot(EProjectSaveSlotKind SlotKind) {
  const FDateTime SaveTimestamp = FDateTime::UtcNow();
  return BeginSaveSlot(SlotKind,
                       BuildUniqueSaveSlotName(SlotKind, SaveTimestamp));
}

bool UProjectSaveSubsystem::LoadLatestSave() {
  if (!CanStartOperation()) {
    UE_LOG(LogProject, Warning,
           TEXT("ProjectSaveSubsystem: rejecting load request because operation state is %d"),
           static_cast<int32>(OperationState));
    ShowStatusMessage(TEXT("Cannot load right now"), FColor::Yellow);
    return false;
  }

  FProjectSaveSlotMetadata LatestSaveMetadata;
  if (!FindMostRecentSaveMetadata(LatestSaveMetadata)) {
    UE_LOG(LogProject, Warning,
           TEXT("ProjectSaveSubsystem: no save entries were found in index"));
    ShowStatusMessage(TEXT("No save found"), FColor::Yellow);
    return false;
  }

  return BeginLoadSlot(LatestSaveMetadata.SlotName);
}

bool UProjectSaveSubsystem::LoadLatestSave(EProjectSaveSlotKind SlotKind) {
  if (!CanStartOperation()) {
    UE_LOG(LogProject, Warning,
           TEXT("ProjectSaveSubsystem: rejecting load request because operation state is %d"),
           static_cast<int32>(OperationState));
    ShowStatusMessage(TEXT("Cannot load right now"), FColor::Yellow);
    return false;
  }

  FProjectSaveSlotMetadata LatestSaveMetadata;
  if (!FindMostRecentSaveMetadata(LatestSaveMetadata, true, SlotKind)) {
    const TCHAR *SlotLabel =
        SlotKind == EProjectSaveSlotKind::Manual ? TEXT("manual") : TEXT("quick");
    UE_LOG(LogProject, Warning,
           TEXT("ProjectSaveSubsystem: no %s save entries were found in index"),
           SlotLabel);
    ShowStatusMessage(TEXT("No save found"), FColor::Yellow);
    return false;
  }

  return BeginLoadSlot(LatestSaveMetadata.SlotName);
}

bool UProjectSaveSubsystem::BeginLoadSlot(const FString &SlotName) {
  if (!CanStartOperation()) {
    UE_LOG(LogProject, Warning,
           TEXT("ProjectSaveSubsystem: rejecting load request because operation state is %d"),
           static_cast<int32>(OperationState));
    ShowStatusMessage(TEXT("Cannot load right now"), FColor::Yellow);
    return false;
  }

  if (SlotName.IsEmpty() ||
      !UGameplayStatics::DoesSaveGameExist(SlotName, SaveUserIndex)) {
    UE_LOG(LogProject, Warning,
           TEXT("ProjectSaveSubsystem: requested slot '%s' does not exist"),
           *SlotName);
    ShowStatusMessage(TEXT("Selected save was not found"), FColor::Yellow);
    return false;
  }

  OperationState = EProjectSaveOperationState::LoadingSlot;
  ShowStatusMessage(TEXT("Loading latest save..."), FColor::Yellow);

  FAsyncLoadGameFromSlotDelegate LoadDelegate;
  LoadDelegate.BindUObject(this, &UProjectSaveSubsystem::HandleAsyncLoadComplete);
  UGameplayStatics::AsyncLoadGameFromSlot(SlotName, SaveUserIndex, LoadDelegate);
  return true;
}

FString UProjectSaveSubsystem::BuildUniqueSaveSlotName(
    EProjectSaveSlotKind SlotKind, const FDateTime &Timestamp) const {
  const FString Prefix =
      SlotKind == EProjectSaveSlotKind::Manual ? TEXT("Manual") : TEXT("Quick");
  return FString::Printf(TEXT("%s_%lld"), *Prefix, Timestamp.GetTicks());
}

FString UProjectSaveSubsystem::GetIndexSlotName() const {
  return TEXT("ProjectSaveIndex");
}

FString UProjectSaveSubsystem::GetCurrentMapName(UWorld *World) const {
  return World ? UGameplayStatics::GetCurrentLevelName(World, true) : FString();
}

int64 UProjectSaveSubsystem::GetCurrentRunPlayTimeSeconds() const {
  const double SessionElapsedSeconds =
      FMath::Max(0.0, FPlatformTime::Seconds() - RunPlayTimeSessionStartSeconds);
  return LoadedRunPlayTimeSeconds + static_cast<int64>(SessionElapsedSeconds);
}

void UProjectSaveSubsystem::ResetRunPlayTimeTracking(int64 LoadedPlayTimeSeconds) {
  LoadedRunPlayTimeSeconds = FMath::Max<int64>(0, LoadedPlayTimeSeconds);
  RunPlayTimeSessionStartSeconds = FPlatformTime::Seconds();
}

void UProjectSaveSubsystem::BuildRunMetaSaveData(
    FRunMetaSaveData &OutRunMeta) const {
  OutRunMeta = FRunMetaSaveData();
  OutRunMeta.TotalPlayTimeSeconds = GetCurrentRunPlayTimeSeconds();
  OutRunMeta.BuildVersion = FApp::GetBuildVersion();
  if (OutRunMeta.BuildVersion.IsEmpty()) {
    OutRunMeta.BuildVersion = FEngineVersion::Current().ToString();
  }
}

UProjectSaveGame *UProjectSaveSubsystem::BuildSaveGameObject(
    EProjectSaveSlotKind SlotKind) {
  UWorld *World = GetWorld();
  if (!World) {
    UE_LOG(LogProject, Error, TEXT("ProjectSaveSubsystem: world is null during save"));
    return nullptr;
  }

  UProjectSaveGame *SaveGameObject =
      Cast<UProjectSaveGame>(UGameplayStatics::CreateSaveGameObject(
          UProjectSaveGame::StaticClass()));
  if (!SaveGameObject) {
    return nullptr;
  }

  SaveGameObject->SaveSchemaVersion = UProjectSaveGame::CurrentSchemaVersion;
  SaveGameObject->SlotKind = SlotKind;
  SaveGameObject->SavedAtUtc = FDateTime::UtcNow();
  BuildRunMetaSaveData(SaveGameObject->RunMeta);

  if (!GatherPlayerSaveData(World, SaveGameObject->PlayerData)) {
    UE_LOG(LogProject, Error,
           TEXT("ProjectSaveSubsystem: failed to gather player save data"));
    return nullptr;
  }

  GatherWorldActorSaveData(World, SaveGameObject->WorldActorRecords);
  return SaveGameObject;
}

UProjectSaveIndex *UProjectSaveSubsystem::LoadOrCreateSaveIndex() const {
  if (UGameplayStatics::DoesSaveGameExist(GetIndexSlotName(), SaveUserIndex)) {
    if (UProjectSaveIndex *ExistingIndex =
            Cast<UProjectSaveIndex>(UGameplayStatics::LoadGameFromSlot(
                GetIndexSlotName(), SaveUserIndex))) {
      return ExistingIndex;
    }
  }

  return Cast<UProjectSaveIndex>(UGameplayStatics::CreateSaveGameObject(
      UProjectSaveIndex::StaticClass()));
}

bool UProjectSaveSubsystem::SaveSaveIndex(UProjectSaveIndex *SaveIndex) const {
  return SaveIndex &&
         UGameplayStatics::SaveGameToSlot(SaveIndex, GetIndexSlotName(), SaveUserIndex);
}

bool UProjectSaveSubsystem::RemoveSaveSlotFromIndex(const FString &SlotName) const {
  if (SlotName.IsEmpty() || SlotName == GetIndexSlotName()) {
    return false;
  }

  UProjectSaveIndex *SaveIndex = LoadOrCreateSaveIndex();
  if (!SaveIndex) {
    return false;
  }

  const int32 RemovedCount = SaveIndex->SaveSlots.RemoveAll(
      [&SlotName](const FProjectSaveSlotMetadata &ExistingMetadata) {
        return ExistingMetadata.SlotName == SlotName;
      });

  if (RemovedCount == 0) {
    return true;
  }

  return SaveSaveIndex(SaveIndex);
}

bool UProjectSaveSubsystem::TryResolveExistingSlotKind(
    const FString &SlotName, EProjectSaveSlotKind &OutSlotKind) const {
  OutSlotKind = EProjectSaveSlotKind::Manual;

  if (SlotName.IsEmpty() || SlotName == GetIndexSlotName()) {
    return false;
  }

  if (UProjectSaveIndex *SaveIndex = LoadOrCreateSaveIndex()) {
    for (const FProjectSaveSlotMetadata &ExistingMetadata : SaveIndex->SaveSlots) {
      if (ExistingMetadata.SlotName == SlotName) {
        OutSlotKind = ExistingMetadata.SlotKind;
        return true;
      }
    }
  }

  if (UProjectSaveGame *ExistingSave =
          Cast<UProjectSaveGame>(
              UGameplayStatics::LoadGameFromSlot(SlotName, SaveUserIndex))) {
    OutSlotKind = ExistingSave->SlotKind;
    return true;
  }

  if (SlotName.StartsWith(TEXT("Quick"))) {
    OutSlotKind = EProjectSaveSlotKind::Quick;
    return true;
  }

  if (SlotName.StartsWith(TEXT("Manual"))) {
    OutSlotKind = EProjectSaveSlotKind::Manual;
    return true;
  }

  return false;
}

bool UProjectSaveSubsystem::FindMostRecentSaveMetadata(
    FProjectSaveSlotMetadata &OutMetadata, bool bFilterByKind,
    EProjectSaveSlotKind SlotKind) const {
  UProjectSaveIndex *SaveIndex = LoadOrCreateSaveIndex();
  if (!SaveIndex) {
    return false;
  }

  bool bFound = false;
  FDateTime MostRecentTimestamp = FDateTime::MinValue();
  for (const FProjectSaveSlotMetadata &SaveSlot : SaveIndex->SaveSlots) {
    if (bFilterByKind && SaveSlot.SlotKind != SlotKind) {
      continue;
    }

    if (!UGameplayStatics::DoesSaveGameExist(SaveSlot.SlotName, SaveUserIndex)) {
      continue;
    }

    if (!bFound || SaveSlot.SavedAtUtc > MostRecentTimestamp) {
      bFound = true;
      MostRecentTimestamp = SaveSlot.SavedAtUtc;
      OutMetadata = SaveSlot;
    }
  }

  return bFound;
}

void UProjectSaveSubsystem::AddSaveSlotToIndex(const FString &SlotName,
                                               EProjectSaveSlotKind SlotKind,
                                               const FString &MapName,
                                               const FDateTime &SavedAtUtc,
                                               const FRunMetaSaveData &RunMeta) {
  UProjectSaveIndex *SaveIndex = LoadOrCreateSaveIndex();
  if (!SaveIndex) {
    return;
  }

  SaveIndex->SaveSlots.RemoveAll(
      [&SlotName](const FProjectSaveSlotMetadata &ExistingMetadata) {
        return ExistingMetadata.SlotName == SlotName;
      });

  FProjectSaveSlotMetadata NewMetadata;
  NewMetadata.SlotName = SlotName;
  NewMetadata.SlotKind = SlotKind;
  NewMetadata.MapName = MapName;
  NewMetadata.SavedAtUtc = SavedAtUtc;
  NewMetadata.RunMeta = RunMeta;
  SaveIndex->SaveSlots.Add(MoveTemp(NewMetadata));

  SaveSaveIndex(SaveIndex);
}

void UProjectSaveSubsystem::ShowStatusMessage(const FString &Message,
                                              FColor Color) const {
  if (bIsShuttingDown) {
    return;
  }

  const FString UppercaseMessage = Message.ToUpper();

  if (GEngine) {
    GEngine->bEnableOnScreenDebugMessages = true;
    GEngine->bEnableOnScreenDebugMessagesDisplay = true;
    GEngine->AddOnScreenDebugMessage(SaveStatusMessageKey, 3.5f, Color,
                                     UppercaseMessage, true,
                                     FVector2D(3.0f, 3.0f));
  }

  if (APlayerController *PlayerController =
          GetWorld() ? UGameplayStatics::GetPlayerController(GetWorld(), 0) : nullptr) {
    PlayerController->ClientMessage(UppercaseMessage, NAME_None, 3.5f);
  }
}

bool UProjectSaveSubsystem::GatherPlayerSaveData(
    UWorld *World, FPlayerSaveData &OutPlayerSaveData) const {
  APlayerController *PlayerController = UGameplayStatics::GetPlayerController(World, 0);
  APawn *Pawn = PlayerController ? PlayerController->GetPawn() : nullptr;
  AMainPlayerController *MainPlayerController =
      Cast<AMainPlayerController>(PlayerController);
  if (!PlayerController || !Pawn || !MainPlayerController) {
    return false;
  }

  OutPlayerSaveData = FPlayerSaveData();
  OutPlayerSaveData.SavedMapName = GetCurrentMapName(World);
  OutPlayerSaveData.PawnTransform = Pawn->GetActorTransform();
  OutPlayerSaveData.ControlRotation = PlayerController->GetControlRotation();

  if (const UHealthComponent *HealthComponent =
          Pawn->FindComponentByClass<UHealthComponent>()) {
    OutPlayerSaveData.bHasHealthState = true;
    OutPlayerSaveData.CurrentHealth = HealthComponent->GetCurrentHealth();
  }

  if (UPlayerArmoryComponent *ArmoryComponent =
          MainPlayerController->GetPlayerArmoryComponent()) {
    ArmoryComponent->CaptureSaveData(OutPlayerSaveData);
  }

  return true;
}

void UProjectSaveSubsystem::PrimeWorldStateTracking(UWorld *World) {
  if (!World) {
    return;
  }

  const FString MapName = GetCurrentMapName(World);
  DefaultWorldActorRecordsByMap.FindOrAdd(MapName).Reset();
  DestroyedActorRecordsByMap.FindOrAdd(MapName).Reset();
  TrackedWorldActorsByObjectKey.Reset();

  for (TActorIterator<AActor> ActorIt(World); ActorIt; ++ActorIt) {
    AActor *Actor = *ActorIt;
    if (!IsValid(Actor)) {
      continue;
    }

    FGuid PersistentId;
    FResolvedWorldSaveTarget ResolvedTarget;
    if (!ResolveWorldSaveTarget(Actor, PersistentId, ResolvedTarget)) {
      continue;
    }

    FTrackedWorldActorContext Context;
    Context.MapName = MapName;
    Context.PersistentId = PersistentId;
    Context.ResolvedTarget = ResolvedTarget;
    TrackedWorldActorsByObjectKey.Add(FObjectKey(Actor), Context);

    Actor->OnDestroyed.RemoveDynamic(this,
                                     &UProjectSaveSubsystem::HandleTrackedActorDestroyed);
    if (ResolvedTarget.UsesComponent()) {
      if (ResolvedTarget.SaveableComponent->ShouldTrackDestroyedState()) {
        Actor->OnDestroyed.AddUniqueDynamic(
            this, &UProjectSaveSubsystem::HandleTrackedActorDestroyed);
      }
      continue;
    }

    FWorldActorSaveData DefaultRecord;
    BuildRuleBasedSaveRecord(Actor, PersistentId, ResolvedTarget, DefaultRecord);
    DefaultWorldActorRecordsByMap.FindOrAdd(MapName).Add(PersistentId,
                                                         DefaultRecord);

    if (ResolvedTarget.bSaveDestroyedState) {
      Actor->OnDestroyed.AddUniqueDynamic(
          this, &UProjectSaveSubsystem::HandleTrackedActorDestroyed);
    }
  }
}

bool UProjectSaveSubsystem::ResolveRestoreContext(
    UWorld *World, AMainPlayerController *&OutController, APawn *&OutPawn) const {
  OutController = nullptr;
  OutPawn = nullptr;

  if (!World) {
    return false;
  }

  OutController =
      Cast<AMainPlayerController>(UGameplayStatics::GetPlayerController(World, 0));
  OutPawn = OutController ? OutController->GetPawn() : nullptr;
  return OutController != nullptr && OutPawn != nullptr;
}

bool UProjectSaveSubsystem::ResolveWorldSaveTarget(
    AActor *Actor, FGuid &OutPersistentId,
    FResolvedWorldSaveTarget &OutResolvedTarget) const {
  OutPersistentId.Invalidate();
  OutResolvedTarget = FResolvedWorldSaveTarget();

  if (!Actor || !IsValid(Actor) || Actor->IsTemplate()) {
    return false;
  }

  if (USaveableActorComponent *SaveableComponent =
          Actor->FindComponentByClass<USaveableActorComponent>()) {
    if (SaveableComponent->HasPersistentIdOverride()) {
      OutPersistentId = SaveableComponent->GetPersistentIdOverride();
    } else {
      if (!TryBuildPlacedActorPersistentId(Actor, OutPersistentId)) {
        return false;
      }
    }

    if (!OutPersistentId.IsValid()) {
      return false;
    }

    OutResolvedTarget.SaveableComponent = SaveableComponent;
    OutResolvedTarget.Policy = EProjectWorldSavePolicy::CustomComponentOnly;
    return true;
  }

  const UProjectWorldSaveProfile *WorldSaveProfile = GetDefault<UProjectWorldSaveProfile>();
  if (!WorldSaveProfile) {
    return false;
  }

  for (const FProjectWorldSaveRule &Rule : WorldSaveProfile->WorldRules) {
    UClass *RuleClass = Rule.ActorClass.Get();
    if (!RuleClass && !Rule.ActorClass.IsNull()) {
      RuleClass = Rule.ActorClass.LoadSynchronous();
    }

    if (!RuleClass) {
      continue;
    }

    const bool bMatches =
        Rule.bIncludeDerivedClasses ? Actor->IsA(RuleClass)
                                    : Actor->GetClass() == RuleClass;
    if (!bMatches) {
      continue;
    }

    if (Rule.SavePolicy == EProjectWorldSavePolicy::None ||
        Rule.SavePolicy == EProjectWorldSavePolicy::CustomComponentOnly) {
      return false;
    }

    if (!TryBuildPlacedActorPersistentId(Actor, OutPersistentId)) {
      return false;
    }

    OutResolvedTarget.Policy = Rule.SavePolicy;
    if (Rule.SavePolicy == EProjectWorldSavePolicy::GameplayCritical) {
      OutResolvedTarget.bSaveDestroyedState = true;
      OutResolvedTarget.bSaveCustomData = true;
    } else if (Rule.SavePolicy == EProjectWorldSavePolicy::PersistentEnemy) {
      OutResolvedTarget.bSaveHealthState = true;
      OutResolvedTarget.bSaveDestroyedState = true;
    }

    OutResolvedTarget.bSaveHealthState |= Rule.bSaveHealthState;
    OutResolvedTarget.bSaveDestroyedState |= Rule.bSaveDestroyedState;
    OutResolvedTarget.bSaveTransformState |= Rule.bSaveTransform;
    OutResolvedTarget.bSaveCustomData |= Rule.bSaveCustomData;
    return true;
  }

  return false;
}

bool UProjectSaveSubsystem::TryBuildPlacedActorPersistentId(
    const AActor *Actor, FGuid &OutPersistentId) const {
  if (!Actor) {
    return false;
  }

  const FString SanitizedPath = UWorld::RemovePIEPrefix(Actor->GetPathName());
  if (SanitizedPath.IsEmpty()) {
    return false;
  }

  OutPersistentId = FGuid::NewDeterministicGuid(SanitizedPath);
  return OutPersistentId.IsValid();
}

bool UProjectSaveSubsystem::IsRuleBasedActorTracked(
    const FString &MapName, const FGuid &PersistentId) const {
  if (const TMap<FGuid, FWorldActorSaveData> *DefaultRecords =
          DefaultWorldActorRecordsByMap.Find(MapName)) {
    return DefaultRecords->Contains(PersistentId);
  }

  return false;
}

void UProjectSaveSubsystem::BuildRuleBasedSaveRecord(
    AActor *Actor, const FGuid &PersistentId,
    const FResolvedWorldSaveTarget &ResolvedTarget,
    FWorldActorSaveData &OutSaveRecord) const {
  OutSaveRecord = FWorldActorSaveData();
  OutSaveRecord.PersistentId = PersistentId;

  if (!Actor) {
    return;
  }

  if (ResolvedTarget.bSaveHealthState) {
    if (const UHealthComponent *HealthComponent =
            Actor->FindComponentByClass<UHealthComponent>()) {
      OutSaveRecord.bHasHealthState = true;
      OutSaveRecord.CurrentHealth = HealthComponent->GetCurrentHealth();
      if (ResolvedTarget.bSaveDestroyedState &&
          OutSaveRecord.CurrentHealth <= 0.0f) {
        OutSaveRecord.bDestroyedOrDead = true;
      }
    }
  }

  if (ResolvedTarget.bSaveTransformState) {
    OutSaveRecord.bHasTransformState = true;
    OutSaveRecord.ActorTransform = Actor->GetActorTransform();
  }

  if (ResolvedTarget.bSaveCustomData) {
    CaptureCustomDataForActor(Actor, OutSaveRecord.CustomData);
  }
}

void UProjectSaveSubsystem::ApplyRuleBasedSaveRecord(
    AActor *Actor, const FWorldActorSaveData &SaveRecord,
    const FResolvedWorldSaveTarget &ResolvedTarget) const {
  if (!Actor) {
    return;
  }

  if (SaveRecord.bHasTransformState) {
    Actor->SetActorTransform(SaveRecord.ActorTransform, false, nullptr,
                             ETeleportType::TeleportPhysics);
  }

  bool bHandledDestroyedStateViaHealth = false;
  if (SaveRecord.bHasHealthState) {
    if (UHealthComponent *HealthComponent =
            Actor->FindComponentByClass<UHealthComponent>()) {
      HealthComponent->RestoreHealthFromSave(SaveRecord.CurrentHealth);
      bHandledDestroyedStateViaHealth =
          SaveRecord.bDestroyedOrDead && SaveRecord.CurrentHealth <= 0.0f;
    }
  }

  if (!IsValid(Actor)) {
    return;
  }

  if (ResolvedTarget.bSaveCustomData &&
      Actor->GetClass()->ImplementsInterface(
          USaveableActorInterface::StaticClass())) {
    ISaveableActorInterface::Execute_ApplySaveCustomData(Actor,
                                                         SaveRecord.CustomData);
  }

  if (SaveRecord.bDestroyedOrDead && ResolvedTarget.bSaveDestroyedState &&
      !bHandledDestroyedStateViaHealth) {
    Actor->SetActorEnableCollision(false);
    Actor->SetActorHiddenInGame(true);
    Actor->Destroy();
  }
}

bool UProjectSaveSubsystem::CaptureCustomDataForActor(
    const AActor *Actor, FSaveableActorCustomData &OutCustomData) const {
  OutCustomData = FSaveableActorCustomData();

  if (!Actor || !Actor->GetClass()->ImplementsInterface(
                    USaveableActorInterface::StaticClass())) {
    return false;
  }

  ISaveableActorInterface::Execute_GatherSaveCustomData(
      const_cast<AActor *>(Actor), OutCustomData);
  return !OutCustomData.IsEmpty();
}

bool UProjectSaveSubsystem::IsWorldRecordAtDefaultState(
    const FString &MapName, const FWorldActorSaveData &SaveRecord) const {
  if (const TMap<FGuid, FWorldActorSaveData> *DefaultRecords =
          DefaultWorldActorRecordsByMap.Find(MapName)) {
    if (const FWorldActorSaveData *DefaultRecord =
            DefaultRecords->Find(SaveRecord.PersistentId)) {
      return AreWorldRecordsEquivalent(*DefaultRecord, SaveRecord);
    }
  }

  return false;
}

bool UProjectSaveSubsystem::AreWorldRecordsEquivalent(
    const FWorldActorSaveData &Left, const FWorldActorSaveData &Right) const {
  if (Left.bDestroyedOrDead != Right.bDestroyedOrDead) {
    return false;
  }

  if (Left.bHasHealthState != Right.bHasHealthState) {
    return false;
  }

  if (Left.bHasHealthState &&
      !FMath::IsNearlyEqual(Left.CurrentHealth, Right.CurrentHealth,
                            KINDA_SMALL_NUMBER)) {
    return false;
  }

  if (Left.bHasTransformState != Right.bHasTransformState) {
    return false;
  }

  if (Left.bHasTransformState &&
      !Left.ActorTransform.Equals(Right.ActorTransform, KINDA_SMALL_NUMBER)) {
    return false;
  }

  return Left.CustomData.IsEquivalentTo(Right.CustomData);
}

void UProjectSaveSubsystem::GatherWorldActorSaveData(
    UWorld *World, TArray<FWorldActorSaveData> &OutWorldActorRecords) {
  OutWorldActorRecords.Reset();

  const FString MapName = GetCurrentMapName(World);
  TMap<FGuid, AActor *> ActorByPersistentId;
  TSet<FGuid> PresentPersistentIds;

  for (TActorIterator<AActor> ActorIt(World); ActorIt; ++ActorIt) {
    AActor *Actor = *ActorIt;
    if (!IsValid(Actor)) {
      continue;
    }

    FGuid PersistentId;
    FResolvedWorldSaveTarget ResolvedTarget;
    if (!ResolveWorldSaveTarget(Actor, PersistentId, ResolvedTarget)) {
      continue;
    }

    if (!RegisterUniquePersistentId(ActorByPersistentId, PersistentId, Actor)) {
      continue;
    }

    PresentPersistentIds.Add(PersistentId);

    FWorldActorSaveData SaveRecord;
    bool bShouldSaveRecord = false;
    if (ResolvedTarget.UsesComponent()) {
      if (!ResolvedTarget.SaveableComponent->HasPersistentIdOverride() &&
          !TrackedWorldActorsByObjectKey.Contains(FObjectKey(Actor))) {
        continue;
      }

      bShouldSaveRecord =
          ResolvedTarget.SaveableComponent->BuildSaveRecord(PersistentId, SaveRecord);
    } else {
      if (!IsRuleBasedActorTracked(MapName, PersistentId)) {
        continue;
      }

      BuildRuleBasedSaveRecord(Actor, PersistentId, ResolvedTarget, SaveRecord);
      bShouldSaveRecord = !IsWorldRecordAtDefaultState(MapName, SaveRecord);
    }

    if (bShouldSaveRecord) {
      OutWorldActorRecords.Add(SaveRecord);
    }
  }

  if (TMap<FGuid, FWorldActorSaveData> *DestroyedRecords =
          DestroyedActorRecordsByMap.Find(MapName)) {
    for (const FGuid &PresentPersistentId : PresentPersistentIds) {
      DestroyedRecords->Remove(PresentPersistentId);
    }

    for (const TPair<FGuid, FWorldActorSaveData> &Pair : *DestroyedRecords) {
      if (!PresentPersistentIds.Contains(Pair.Key)) {
        OutWorldActorRecords.Add(Pair.Value);
      }
    }
  }
}

void UProjectSaveSubsystem::StartPendingRestore(UWorld *World, int32 AttemptIndex) {
  if (!PendingLoadedSaveGame || !World) {
    ResetOperationState();
    return;
  }

  AMainPlayerController *MainPlayerController = nullptr;
  APawn *Pawn = nullptr;
  if (!ResolveRestoreContext(World, MainPlayerController, Pawn)) {
    if (AttemptIndex >= RestoreRetryLimit) {
      UE_LOG(LogProject, Error,
             TEXT("ProjectSaveSubsystem: failed to resolve player context after map load"));
      ResetOperationState();
      return;
    }

    FTimerDelegate RetryDelegate;
    RetryDelegate.BindUObject(this, &UProjectSaveSubsystem::StartPendingRestore,
                              World, AttemptIndex + 1);
    World->GetTimerManager().SetTimerForNextTick(RetryDelegate);
    return;
  }

  OperationState = EProjectSaveOperationState::Restoring;
  ShowStatusMessage(TEXT("Restoring save..."), FColor::Yellow);
  QueueWeaponClassPreload(World);
}

void UProjectSaveSubsystem::ContinuePendingRestore(UWorld *World) {
  if (!PendingLoadedSaveGame || !World) {
    ResetOperationState();
    return;
  }

  AMainPlayerController *MainPlayerController = nullptr;
  APawn *Pawn = nullptr;
  if (!ResolveRestoreContext(World, MainPlayerController, Pawn)) {
    ResetOperationState();
    return;
  }

  RestoreWorldActorState(World, PendingLoadedSaveGame);
  RestorePlayerState(World, PendingLoadedSaveGame, MainPlayerController, Pawn);
  FinishPendingRestore(World);
}

void UProjectSaveSubsystem::RestoreWorldActorState(
    UWorld *World, const UProjectSaveGame *SaveGameObject) {
  if (!World || !SaveGameObject) {
    return;
  }

  TMap<FGuid, FRestorableWorldActorTarget> TargetsByPersistentId;
  TMap<FGuid, AActor *> ActorByPersistentId;
  for (TActorIterator<AActor> ActorIt(World); ActorIt; ++ActorIt) {
    AActor *Actor = *ActorIt;
    if (!IsValid(Actor)) {
      continue;
    }

    FGuid PersistentId;
    FResolvedWorldSaveTarget ResolvedTarget;
    if (!ResolveWorldSaveTarget(Actor, PersistentId, ResolvedTarget)) {
      continue;
    }

    if (!RegisterUniquePersistentId(ActorByPersistentId, PersistentId, Actor)) {
      continue;
    }

    FRestorableWorldActorTarget Target;
    Target.Actor = Actor;
    Target.ResolvedTarget = ResolvedTarget;
    TargetsByPersistentId.Add(PersistentId, Target);
  }

  for (const FWorldActorSaveData &SaveRecord : SaveGameObject->WorldActorRecords) {
    if (const FRestorableWorldActorTarget *FoundTarget =
            TargetsByPersistentId.Find(SaveRecord.PersistentId)) {
      if (FoundTarget->ResolvedTarget.UsesComponent()) {
        FoundTarget->ResolvedTarget.SaveableComponent->ApplySaveRecord(SaveRecord);
      } else {
        ApplyRuleBasedSaveRecord(FoundTarget->Actor.Get(), SaveRecord,
                                 FoundTarget->ResolvedTarget);
      }
    } else {
      UE_LOG(LogProject, Verbose,
             TEXT("ProjectSaveSubsystem: no persistent actor found for id '%s'"),
             *SaveRecord.PersistentId.ToString());
    }
  }
}

void UProjectSaveSubsystem::RestorePlayerState(
    UWorld *World, const UProjectSaveGame *SaveGameObject,
    AMainPlayerController *Controller, APawn *Pawn) {
  if (!SaveGameObject || !Controller || !Pawn) {
    return;
  }

  const FPlayerSaveData &PlayerData = SaveGameObject->PlayerData;

  Pawn->SetActorTransform(PlayerData.PawnTransform, false, nullptr,
                          ETeleportType::TeleportPhysics);
  Controller->SetControlRotation(PlayerData.ControlRotation);
  Controller->MarkStartupStateRestoredFromSave();

  if (UPlayerArmoryComponent *ArmoryComponent =
          Controller->GetPlayerArmoryComponent()) {
    ArmoryComponent->BindToPawn(Pawn);
    ArmoryComponent->RestoreFromSaveData(PlayerData);
  }

  if (PlayerData.bHasHealthState) {
    if (UHealthComponent *HealthComponent =
            Pawn->FindComponentByClass<UHealthComponent>()) {
      HealthComponent->RestoreHealthFromSave(PlayerData.CurrentHealth);
    }
  }
}

void UProjectSaveSubsystem::QueueWeaponClassPreload(UWorld *World) {
  if (!PendingLoadedSaveGame) {
    ResetOperationState();
    return;
  }

  TArray<FSoftObjectPath> AssetPaths;
  for (const FArmoryItemSaveData &ItemData :
       PendingLoadedSaveGame->PlayerData.ArmoryItems) {
    if (ItemData.WeaponClass.IsNull() || ItemData.WeaponClass.IsValid()) {
      continue;
    }

    AssetPaths.AddUnique(ItemData.WeaponClass.ToSoftObjectPath());
  }

  if (AssetPaths.IsEmpty()) {
    ContinuePendingRestore(World);
    return;
  }

  PendingAssetLoadHandle =
      UAssetManager::GetStreamableManager().RequestAsyncLoad(
          AssetPaths, FStreamableDelegate::CreateUObject(
                          this, &UProjectSaveSubsystem::ContinuePendingRestore, World));
}

void UProjectSaveSubsystem::FinishPendingRestore(UWorld *World) {
  const int64 LoadedPlayTimeSeconds =
      PendingLoadedSaveGame ? PendingLoadedSaveGame->RunMeta.TotalPlayTimeSeconds : 0;

  PendingAssetLoadHandle.Reset();
  PendingLoadedSaveGame = nullptr;
  ActiveSaveObject = nullptr;
  PendingSaveSlotName.Empty();
  OperationState = EProjectSaveOperationState::Idle;
  ResetRunPlayTimeTracking(LoadedPlayTimeSeconds);
  ShowStatusMessage(TEXT("Save loaded"), FColor::Green);
}

void UProjectSaveSubsystem::ResetOperationState() {
  if (PendingAssetLoadHandle.IsValid()) {
    PendingAssetLoadHandle->CancelHandle();
    PendingAssetLoadHandle.Reset();
  }

  if (UWorld *World = GetWorld()) {
    World->GetTimerManager().ClearAllTimersForObject(this);
  }

  ActiveSaveObject = nullptr;
  PendingLoadedSaveGame = nullptr;
  PendingSaveSlotName.Empty();
  PendingSaveSlotKind = EProjectSaveSlotKind::Quick;
  OperationState = EProjectSaveOperationState::Idle;
}

void UProjectSaveSubsystem::HandleShutdownRequested() {
  if (bIsShuttingDown) {
    return;
  }

  bIsShuttingDown = true;
  ResetOperationState();
}

void UProjectSaveSubsystem::HandleAsyncSaveComplete(const FString &SlotName,
                                                    const int32 UserIndex,
                                                    bool bSuccess) {
  if (bIsShuttingDown) {
    ResetOperationState();
    return;
  }

  if (!bSuccess) {
    UE_LOG(LogProject, Error,
           TEXT("ProjectSaveSubsystem: async save failed for slot '%s'"),
           *SlotName);
    ShowStatusMessage(TEXT("Save failed"), FColor::Red);
    ActiveSaveObject = nullptr;
    PendingSaveSlotName.Empty();
    OperationState = EProjectSaveOperationState::Idle;
    return;
  }

  if (ActiveSaveObject) {
    AddSaveSlotToIndex(SlotName, PendingSaveSlotKind,
                       ActiveSaveObject->PlayerData.SavedMapName,
                       ActiveSaveObject->SavedAtUtc, ActiveSaveObject->RunMeta);
  }

  ShowStatusMessage(TEXT("Game saved"), FColor::Green);
  ActiveSaveObject = nullptr;
  PendingSaveSlotName.Empty();
  OperationState = EProjectSaveOperationState::Idle;
}

void UProjectSaveSubsystem::HandleAsyncLoadComplete(const FString &SlotName,
                                                    const int32 UserIndex,
                                                    USaveGame *LoadedGameData) {
  if (bIsShuttingDown) {
    ResetOperationState();
    return;
  }

  PendingLoadedSaveGame = Cast<UProjectSaveGame>(LoadedGameData);
  if (!PendingLoadedSaveGame) {
    UE_LOG(LogProject, Error,
           TEXT("ProjectSaveSubsystem: async load returned invalid save object for slot '%s'"),
           *SlotName);
    ShowStatusMessage(TEXT("Load failed"), FColor::Red);
    ResetOperationState();
    return;
  }

  if (PendingLoadedSaveGame->SaveSchemaVersion !=
      UProjectSaveGame::CurrentSchemaVersion) {
    UE_LOG(LogProject, Error,
           TEXT("ProjectSaveSubsystem: unsupported save schema %d, expected %d"),
           PendingLoadedSaveGame->SaveSchemaVersion,
           UProjectSaveGame::CurrentSchemaVersion);
    ShowStatusMessage(TEXT("Save version is not supported"), FColor::Red);
    ResetOperationState();
    return;
  }

  const FString MapName = PendingLoadedSaveGame->PlayerData.SavedMapName;
  if (MapName.IsEmpty()) {
    UE_LOG(LogProject, Error,
           TEXT("ProjectSaveSubsystem: loaded save contains an empty map name"));
    ShowStatusMessage(TEXT("Load failed"), FColor::Red);
    ResetOperationState();
    return;
  }

  OperationState = EProjectSaveOperationState::OpeningLevel;
  ShowStatusMessage(TEXT("Opening saved level..."), FColor::Yellow);
  UGameplayStatics::OpenLevel(this, FName(*MapName));
}

void UProjectSaveSubsystem::HandleTrackedActorDestroyed(AActor *DestroyedActor) {
  if (bIsShuttingDown || OperationState == EProjectSaveOperationState::OpeningLevel ||
      !DestroyedActor) {
    return;
  }

  FTrackedWorldActorContext Context;
  if (!TrackedWorldActorsByObjectKey.RemoveAndCopyValue(FObjectKey(DestroyedActor),
                                                        Context)) {
    return;
  }

  FWorldActorSaveData SaveRecord;
  if (Context.ResolvedTarget.UsesComponent()) {
    if (!Context.ResolvedTarget.SaveableComponent.IsValid()) {
      return;
    }

    Context.ResolvedTarget.SaveableComponent->BuildSaveRecord(
        Context.PersistentId, SaveRecord);
  } else {
    BuildRuleBasedSaveRecord(DestroyedActor, Context.PersistentId,
                             Context.ResolvedTarget, SaveRecord);
  }

  SaveRecord.PersistentId = Context.PersistentId;
  SaveRecord.bDestroyedOrDead = true;
  RegisterDestroyedActorRecord(Context.MapName, SaveRecord);
}

void UProjectSaveSubsystem::HandlePostLoadMapWithWorld(UWorld *LoadedWorld) {
  if (bIsShuttingDown || !LoadedWorld || !LoadedWorld->IsGameWorld()) {
    return;
  }

  PrimeWorldStateTracking(LoadedWorld);

  if (!HasPendingRestore()) {
    return;
  }

  StartPendingRestore(LoadedWorld, 0);
}
