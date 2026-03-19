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
#include "Misc/CoreDelegates.h"
#include "Project.h"
#include "Save/ProjectSaveIndex.h"
#include "Save/ProjectSaveGame.h"
#include "Save/SaveableActorComponent.h"

namespace {
constexpr uint64 SaveStatusMessageKey = 0x50524F4A53415645ULL;

bool IsDuplicateSaveId(TMap<FGuid, USaveableActorComponent *> &ComponentById,
                       USaveableActorComponent *SaveableComponent) {
  if (!SaveableComponent) {
    return false;
  }

  const FGuid SaveId = SaveableComponent->GetSaveId();
  if (!SaveId.IsValid()) {
    return true;
  }

  if (USaveableActorComponent **ExistingComponent = ComponentById.Find(SaveId)) {
    UE_LOG(LogProject, Error,
           TEXT("ProjectSaveSubsystem: duplicate SaveId '%s' on '%s' and '%s'"),
           *SaveId.ToString(), *GetNameSafe((*ExistingComponent)->GetOwner()),
           *GetNameSafe(SaveableComponent->GetOwner()));
    return true;
  }

  ComponentById.Add(SaveId, SaveableComponent);
  return false;
}
} // namespace

void UProjectSaveSubsystem::Initialize(FSubsystemCollectionBase &Collection) {
  Super::Initialize(Collection);
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

  DestroyedActorRecordsByMap.Reset();

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

bool UProjectSaveSubsystem::QuickSave() {
  return SaveToSlot(EProjectSaveSlotKind::Quick);
}

bool UProjectSaveSubsystem::QuickLoad() {
  return LoadLatestSave();
}

bool UProjectSaveSubsystem::ManualSave() {
  return SaveToSlot(EProjectSaveSlotKind::Manual);
}

bool UProjectSaveSubsystem::ManualLoad() {
  return LoadLatestSave();
}

void UProjectSaveSubsystem::RegisterDestroyedActorRecord(
    const FString &MapName, const FWorldActorSaveData &SaveRecord) {
  if (MapName.IsEmpty() || !SaveRecord.SaveId.IsValid()) {
    return;
  }

  DestroyedActorRecordsByMap.FindOrAdd(MapName).Add(SaveRecord.SaveId, SaveRecord);
}

void UProjectSaveSubsystem::ClearDestroyedActorRecord(const FString &MapName,
                                                      const FGuid &SaveId) {
  if (MapName.IsEmpty() || !SaveId.IsValid()) {
    return;
  }

  if (TMap<FGuid, FWorldActorSaveData> *Records = DestroyedActorRecordsByMap.Find(MapName)) {
    Records->Remove(SaveId);
  }
}

bool UProjectSaveSubsystem::SaveToSlot(EProjectSaveSlotKind SlotKind) {
  if (!CanStartOperation()) {
    UE_LOG(LogProject, Warning,
           TEXT("ProjectSaveSubsystem: rejecting save request because operation state is %d"),
           static_cast<int32>(OperationState));
    ShowStatusMessage(TEXT("Cannot save right now"), FColor::Yellow);
    return false;
  }

  ActiveSaveObject = BuildSaveGameObject(SlotKind);
  if (!ActiveSaveObject) {
    ShowStatusMessage(TEXT("Save failed"), FColor::Red);
    return false;
  }

  OperationState = EProjectSaveOperationState::Saving;
  PendingSaveSlotKind = SlotKind;
  PendingSaveSlotName =
      BuildUniqueSaveSlotName(SlotKind, ActiveSaveObject->SavedAtUtc);
  ShowStatusMessage(TEXT("Saving..."), FColor::Yellow);

  FAsyncSaveGameToSlotDelegate SaveDelegate;
  SaveDelegate.BindUObject(this, &UProjectSaveSubsystem::HandleAsyncSaveComplete);
  UGameplayStatics::AsyncSaveGameToSlot(ActiveSaveObject, PendingSaveSlotName,
                                        SaveUserIndex, SaveDelegate);
  return true;
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

  OperationState = EProjectSaveOperationState::LoadingSlot;
  ShowStatusMessage(TEXT("Loading latest save..."), FColor::Yellow);

  FAsyncLoadGameFromSlotDelegate LoadDelegate;
  LoadDelegate.BindUObject(this, &UProjectSaveSubsystem::HandleAsyncLoadComplete);
  UGameplayStatics::AsyncLoadGameFromSlot(LatestSaveMetadata.SlotName, SaveUserIndex,
                                          LoadDelegate);
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
                                               const FDateTime &SavedAtUtc) {
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

FString UProjectSaveSubsystem::GetCurrentMapName(UWorld *World) const {
  return World ? UGameplayStatics::GetCurrentLevelName(World, true) : FString();
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

  if (!GatherPlayerSaveData(World, SaveGameObject->PlayerData)) {
    UE_LOG(LogProject, Error,
           TEXT("ProjectSaveSubsystem: failed to gather player save data"));
    return nullptr;
  }

  GatherWorldActorSaveData(World, SaveGameObject->WorldActorRecords);
  return SaveGameObject;
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

void UProjectSaveSubsystem::GatherWorldActorSaveData(
    UWorld *World, TArray<FWorldActorSaveData> &OutWorldActorRecords) {
  OutWorldActorRecords.Reset();

  TMap<FGuid, USaveableActorComponent *> ComponentById;
  for (TActorIterator<AActor> ActorIt(World); ActorIt; ++ActorIt) {
    AActor *Actor = *ActorIt;
    if (!IsValid(Actor)) {
      continue;
    }

    USaveableActorComponent *SaveableComponent =
        Actor->FindComponentByClass<USaveableActorComponent>();
    if (!SaveableComponent || IsDuplicateSaveId(ComponentById, SaveableComponent)) {
      continue;
    }

    FWorldActorSaveData SaveRecord;
    if (SaveableComponent->BuildSaveRecord(SaveRecord)) {
      OutWorldActorRecords.Add(SaveRecord);
    }
  }

  const FString MapName = GetCurrentMapName(World);
  if (TMap<FGuid, FWorldActorSaveData> *DestroyedRecords =
          DestroyedActorRecordsByMap.Find(MapName)) {
    for (const TPair<FGuid, FWorldActorSaveData> &Pair : *DestroyedRecords) {
      if (!ComponentById.Contains(Pair.Key)) {
        OutWorldActorRecords.Add(Pair.Value);
      }
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

  TMap<FGuid, USaveableActorComponent *> ComponentById;
  for (TActorIterator<AActor> ActorIt(World); ActorIt; ++ActorIt) {
    AActor *Actor = *ActorIt;
    if (!IsValid(Actor)) {
      continue;
    }

    USaveableActorComponent *SaveableComponent =
        Actor->FindComponentByClass<USaveableActorComponent>();
    if (!SaveableComponent || IsDuplicateSaveId(ComponentById, SaveableComponent)) {
      continue;
    }
  }

  for (const FWorldActorSaveData &SaveRecord : SaveGameObject->WorldActorRecords) {
    if (USaveableActorComponent **FoundComponent = ComponentById.Find(SaveRecord.SaveId)) {
      (*FoundComponent)->ApplySaveRecord(SaveRecord);
    } else {
      UE_LOG(LogProject, Verbose,
             TEXT("ProjectSaveSubsystem: no saveable actor found for SaveId '%s'"),
             *SaveRecord.SaveId.ToString());
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
  PendingAssetLoadHandle.Reset();
  PendingLoadedSaveGame = nullptr;
  ActiveSaveObject = nullptr;
  PendingSaveSlotName.Empty();
  OperationState = EProjectSaveOperationState::Idle;
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
                       ActiveSaveObject->SavedAtUtc);
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

  if (PendingLoadedSaveGame->SaveSchemaVersion >
      UProjectSaveGame::CurrentSchemaVersion) {
    UE_LOG(LogProject, Error,
           TEXT("ProjectSaveSubsystem: save schema %d is newer than supported schema %d"),
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

void UProjectSaveSubsystem::HandlePostLoadMapWithWorld(UWorld *LoadedWorld) {
  if (bIsShuttingDown || !LoadedWorld || !LoadedWorld->IsGameWorld()) {
    return;
  }

  DestroyedActorRecordsByMap.FindOrAdd(GetCurrentMapName(LoadedWorld)).Reset();

  if (!HasPendingRestore()) {
    return;
  }

  StartPendingRestore(LoadedWorld, 0);
}
