#include "ProjectGameViewportClient.h"

#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "GenericPlatform/GenericPlatformMisc.h"
#include "HAL/PlatformTime.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Misc/CoreDelegates.h"
#include "Misc/MessageDialog.h"
#include "Project.h"
#include "Save/ProjectSaveSubsystem.h"
#include "UI/Menu/ProjectInGameMenuWidgetBase.h"
#include "UI/Menu/ProjectStartupMenuWidgetBase.h"

#define LOCTEXT_NAMESPACE "ProjectGameViewportClient"

void UProjectGameViewportClient::Tick(float DeltaTime) {
  Super::Tick(DeltaTime);

  EnsureOverlayWidgets();
  BindMapLifecycleDelegates();
  MaybeShowStartupMenu();
  SyncLoadingScreenWithSaveSubsystem();

  if (bPendingSaveSelectionRefresh) {
    const UProjectSaveSubsystem *SaveSubsystem =
        GetGameInstance()
            ? GetGameInstance()->GetSubsystem<UProjectSaveSubsystem>()
            : nullptr;
    if (!SaveSubsystem ||
        SaveSubsystem->GetOperationState() == EProjectSaveOperationState::Idle) {
      RefreshAvailableSaveSlots();
      bPendingSaveSelectionRefresh = false;
    }
  }

  TickLoadingScreen(DeltaTime);
  RefreshOverlayWidgets();
  UpdateOverlayInputState();
}

void UProjectGameViewportClient::BeginDestroy() {
  if (StartupOverlayWidget) {
    StartupOverlayWidget->OnStartGameRequested.RemoveAll(this);
    StartupOverlayWidget->OnContinueGameRequested.RemoveAll(this);
    StartupOverlayWidget->OnOpenSaveSelectionRequested.RemoveAll(this);
    StartupOverlayWidget->OnBackToStartupMenuRequested.RemoveAll(this);
    StartupOverlayWidget->OnLoadSaveSlotRequested.RemoveAll(this);
    StartupOverlayWidget->OnOverwriteSaveSlotRequested.RemoveAll(this);
    StartupOverlayWidget->OnDeleteSaveSlotRequested.RemoveAll(this);
    StartupOverlayWidget->OnExitGameRequested.RemoveAll(this);
    StartupOverlayWidget->RemoveFromParent();
    StartupOverlayWidget = nullptr;
  }

  if (InGameOverlayWidget) {
    InGameOverlayWidget->OnResumeRequested.RemoveAll(this);
    InGameOverlayWidget->OnSaveGameRequested.RemoveAll(this);
    InGameOverlayWidget->OnOpenSaveSelectionRequested.RemoveAll(this);
    InGameOverlayWidget->OnBackToInGameMenuRequested.RemoveAll(this);
    InGameOverlayWidget->OnLoadSaveSlotRequested.RemoveAll(this);
    InGameOverlayWidget->OnOverwriteSaveSlotRequested.RemoveAll(this);
    InGameOverlayWidget->OnDeleteSaveSlotRequested.RemoveAll(this);
    InGameOverlayWidget->OnReturnToMainMenuRequested.RemoveAll(this);
    InGameOverlayWidget->OnExitGameRequested.RemoveAll(this);
    InGameOverlayWidget->RemoveFromParent();
    InGameOverlayWidget = nullptr;
  }

  UnbindMapLifecycleDelegates();

  Super::BeginDestroy();
}

bool UProjectGameViewportClient::WindowCloseRequested() {
  const bool bAllowClose = Super::WindowCloseRequested();
  if (bAllowClose) {
    RequestProcessExit();
  }

  return bAllowClose;
}

void UProjectGameViewportClient::CloseRequested(FViewport *InViewport) {
  RequestProcessExit();
  Super::CloseRequested(InViewport);
}

void UProjectGameViewportClient::SetStartupMenuEnabled(bool bEnabled) {
  if (bStartupMenuEnabled == bEnabled) {
    return;
  }

  bStartupMenuEnabled = bEnabled;
  if (!bStartupMenuEnabled &&
      ActiveOverlayWidgetType == EProjectViewportOverlayWidgetType::Startup &&
      (OverlayState == EProjectViewportOverlayState::StartupMenu ||
       OverlayState == EProjectViewportOverlayState::SaveSelection)) {
    HideStartupMenu();
  }
}

void UProjectGameViewportClient::SetInGameMenuEnabled(bool bEnabled) {
  if (bInGameMenuEnabled == bEnabled) {
    return;
  }

  bInGameMenuEnabled = bEnabled;
  if (!bInGameMenuEnabled &&
      ActiveOverlayWidgetType == EProjectViewportOverlayWidgetType::InGame &&
      (OverlayState == EProjectViewportOverlayState::InGameMenu ||
       OverlayState == EProjectViewportOverlayState::SaveSelection)) {
    HideInGameMenu();
  }
}

bool UProjectGameViewportClient::HandleEscapeMenuAction() {
  EnsureOverlayWidgets();

  UE_LOG(
      LogProject, Log,
      TEXT("ProjectGameViewportClient: HandleEscapeMenuAction (ActiveWidgetType=%d, OverlayState=%d, InGameWidget=%s, InGameWidgetInViewport=%s)"),
      static_cast<int32>(ActiveOverlayWidgetType),
      static_cast<int32>(OverlayState),
      *GetNameSafe(InGameOverlayWidget),
      InGameOverlayWidget && InGameOverlayWidget->IsInViewport() ? TEXT("true")
                                                                 : TEXT("false"));

  if (ActiveOverlayWidgetType == EProjectViewportOverlayWidgetType::Startup) {
    if (OverlayState == EProjectViewportOverlayState::SaveSelection) {
      ShowStartupMenu();
      return true;
    }

    if (OverlayState == EProjectViewportOverlayState::StartupMenu) {
      if (ConfirmExitGameRequest()) {
        ExecuteExitGameRequest();
      }
      return true;
    }

    if (OverlayState == EProjectViewportOverlayState::LoadingScreen) {
      return true;
    }
  }

  if (ActiveOverlayWidgetType == EProjectViewportOverlayWidgetType::InGame) {
    if (OverlayState == EProjectViewportOverlayState::SaveSelection) {
      ShowInGameMenu();
      return true;
    }

    if (OverlayState == EProjectViewportOverlayState::InGameMenu) {
      HideInGameMenu();
      return true;
    }

    if (OverlayState == EProjectViewportOverlayState::LoadingScreen) {
      return true;
    }
  }

  if (OverlayState != EProjectViewportOverlayState::None) {
    return true;
  }

  if (!CanShowInGameMenu()) {
    return false;
  }

  ShowInGameMenu();
  return true;
}

void UProjectGameViewportClient::EnsureOverlayWidgets() {
  if (!GetGameInstance()) {
    return;
  }

  APlayerController *OwningPlayer = GetPrimaryPlayerController();
  const bool bStartupWidgetDetached =
      StartupOverlayWidget && !StartupOverlayWidget->IsInViewport();
  const bool bInGameWidgetDetached =
      InGameOverlayWidget && !InGameOverlayWidget->IsInViewport();

  if (StartupOverlayWidget &&
      ((OwningPlayer &&
        StartupOverlayWidget->GetOwningPlayer() != OwningPlayer) ||
       bStartupWidgetDetached)) {
    UE_LOG(
        LogProject, Log,
        TEXT("ProjectGameViewportClient: recreating startup overlay (Detached=%s, OwnerMismatch=%s, Widget=%s)"),
        bStartupWidgetDetached ? TEXT("true") : TEXT("false"),
        (OwningPlayer &&
         StartupOverlayWidget->GetOwningPlayer() != OwningPlayer)
            ? TEXT("true")
            : TEXT("false"),
        *GetNameSafe(StartupOverlayWidget));
    StartupOverlayWidget->OnStartGameRequested.RemoveAll(this);
    StartupOverlayWidget->OnContinueGameRequested.RemoveAll(this);
    StartupOverlayWidget->OnOpenSaveSelectionRequested.RemoveAll(this);
    StartupOverlayWidget->OnBackToStartupMenuRequested.RemoveAll(this);
    StartupOverlayWidget->OnLoadSaveSlotRequested.RemoveAll(this);
    StartupOverlayWidget->OnOverwriteSaveSlotRequested.RemoveAll(this);
    StartupOverlayWidget->OnDeleteSaveSlotRequested.RemoveAll(this);
    StartupOverlayWidget->OnExitGameRequested.RemoveAll(this);
    StartupOverlayWidget->RemoveFromParent();
    StartupOverlayWidget = nullptr;
  }

  if (!StartupOverlayWidget) {
    const TSubclassOf<UProjectStartupMenuWidgetBase> WidgetClass =
        ResolveStartupMenuWidgetClass();
    StartupOverlayWidget =
        OwningPlayer
            ? CreateWidget<UProjectStartupMenuWidgetBase>(OwningPlayer, WidgetClass)
            : CreateWidget<UProjectStartupMenuWidgetBase>(GetGameInstance(),
                                                          WidgetClass);
    if (StartupOverlayWidget) {
      StartupOverlayWidget->OnStartGameRequested.AddUObject(
          this, &UProjectGameViewportClient::HandleStartGameRequested);
      StartupOverlayWidget->OnContinueGameRequested.AddUObject(
          this, &UProjectGameViewportClient::HandleContinueGameRequested);
      StartupOverlayWidget->OnOpenSaveSelectionRequested.AddUObject(
          this,
          &UProjectGameViewportClient::HandleStartupOpenSaveSelectionRequested);
      StartupOverlayWidget->OnBackToStartupMenuRequested.AddUObject(
          this, &UProjectGameViewportClient::HandleBackToStartupMenuRequested);
      StartupOverlayWidget->OnLoadSaveSlotRequested.AddUObject(
          this,
          &UProjectGameViewportClient::HandleStartupLoadSelectedSaveRequested);
      StartupOverlayWidget->OnOverwriteSaveSlotRequested.AddUObject(
          this, &UProjectGameViewportClient::HandleOverwriteSaveSlotRequested);
      StartupOverlayWidget->OnDeleteSaveSlotRequested.AddUObject(
          this, &UProjectGameViewportClient::HandleDeleteSaveSlotRequested);
      StartupOverlayWidget->OnExitGameRequested.AddUObject(
          this, &UProjectGameViewportClient::HandleExitGameRequested);
      if (OwningPlayer) {
        if (!StartupOverlayWidget->AddToPlayerScreen(1000)) {
          StartupOverlayWidget->AddToViewport(1000);
        }
      } else {
        StartupOverlayWidget->AddToViewport(1000);
      }
    }
  }

  if (InGameOverlayWidget &&
      ((OwningPlayer &&
        InGameOverlayWidget->GetOwningPlayer() != OwningPlayer) ||
       bInGameWidgetDetached)) {
    UE_LOG(
        LogProject, Log,
        TEXT("ProjectGameViewportClient: recreating in-game overlay (Detached=%s, OwnerMismatch=%s, Widget=%s)"),
        bInGameWidgetDetached ? TEXT("true") : TEXT("false"),
        (OwningPlayer &&
         InGameOverlayWidget->GetOwningPlayer() != OwningPlayer)
            ? TEXT("true")
            : TEXT("false"),
        *GetNameSafe(InGameOverlayWidget));
    InGameOverlayWidget->OnResumeRequested.RemoveAll(this);
    InGameOverlayWidget->OnSaveGameRequested.RemoveAll(this);
    InGameOverlayWidget->OnOpenSaveSelectionRequested.RemoveAll(this);
    InGameOverlayWidget->OnBackToInGameMenuRequested.RemoveAll(this);
    InGameOverlayWidget->OnLoadSaveSlotRequested.RemoveAll(this);
    InGameOverlayWidget->OnOverwriteSaveSlotRequested.RemoveAll(this);
    InGameOverlayWidget->OnDeleteSaveSlotRequested.RemoveAll(this);
    InGameOverlayWidget->OnReturnToMainMenuRequested.RemoveAll(this);
    InGameOverlayWidget->OnExitGameRequested.RemoveAll(this);
    InGameOverlayWidget->RemoveFromParent();
    InGameOverlayWidget = nullptr;
  }

  if (!InGameOverlayWidget) {
    const TSubclassOf<UProjectInGameMenuWidgetBase> WidgetClass =
        ResolveInGameMenuWidgetClass();
    InGameOverlayWidget =
        OwningPlayer
            ? CreateWidget<UProjectInGameMenuWidgetBase>(OwningPlayer, WidgetClass)
            : CreateWidget<UProjectInGameMenuWidgetBase>(GetGameInstance(),
                                                         WidgetClass);
    if (InGameOverlayWidget) {
      InGameOverlayWidget->OnResumeRequested.AddUObject(
          this, &UProjectGameViewportClient::HandleResumeGameRequested);
      InGameOverlayWidget->OnSaveGameRequested.AddUObject(
          this, &UProjectGameViewportClient::HandleSaveGameRequested);
      InGameOverlayWidget->OnOpenSaveSelectionRequested.AddUObject(
          this,
          &UProjectGameViewportClient::HandleInGameOpenSaveSelectionRequested);
      InGameOverlayWidget->OnBackToInGameMenuRequested.AddUObject(
          this, &UProjectGameViewportClient::HandleBackToInGameMenuRequested);
      InGameOverlayWidget->OnLoadSaveSlotRequested.AddUObject(
          this, &UProjectGameViewportClient::HandleInGameLoadSelectedSaveRequested);
      InGameOverlayWidget->OnOverwriteSaveSlotRequested.AddUObject(
          this, &UProjectGameViewportClient::HandleOverwriteSaveSlotRequested);
      InGameOverlayWidget->OnDeleteSaveSlotRequested.AddUObject(
          this, &UProjectGameViewportClient::HandleDeleteSaveSlotRequested);
      InGameOverlayWidget->OnReturnToMainMenuRequested.AddUObject(
          this, &UProjectGameViewportClient::HandleReturnToMainMenuRequested);
      InGameOverlayWidget->OnExitGameRequested.AddUObject(
          this, &UProjectGameViewportClient::HandleExitGameRequested);
      if (OwningPlayer) {
        if (!InGameOverlayWidget->AddToPlayerScreen(1000)) {
          InGameOverlayWidget->AddToViewport(1000);
        }
      } else {
        InGameOverlayWidget->AddToViewport(1000);
      }
    }
  }

  RefreshOverlayWidgets();
}

void UProjectGameViewportClient::RefreshOverlayWidgets() {
  if (StartupOverlayWidget) {
    StartupOverlayWidget->SetCanContinueGame(CanContinueGame());
    StartupOverlayWidget->SetCanOpenSaveSelection(CanOpenSaveSelection());
    StartupOverlayWidget->SetAvailableSaveSlots(AvailableSaveSlots);
    StartupOverlayWidget->SetLoadingState(GetLoadingScreenTitle(),
                                          GetLoadingScreenStatus(),
                                          DisplayedLoadingProgress);
    StartupOverlayWidget->SetViewState(GetStartupWidgetViewState());
  }

  if (InGameOverlayWidget) {
    InGameOverlayWidget->SetCanSaveGame(CanSaveGame());
    InGameOverlayWidget->SetCanOpenSaveSelection(CanOpenSaveSelection());
    InGameOverlayWidget->SetAvailableSaveSlots(AvailableSaveSlots);
    InGameOverlayWidget->SetLoadingState(GetLoadingScreenTitle(),
                                         GetLoadingScreenStatus(),
                                         DisplayedLoadingProgress);
    InGameOverlayWidget->SetViewState(GetInGameWidgetViewState());
  }
}

void UProjectGameViewportClient::RefreshAvailableSaveSlots() {
  AvailableSaveSlots.Reset();

  if (const UProjectSaveSubsystem *SaveSubsystem =
          GetGameInstance()
              ? GetGameInstance()->GetSubsystem<UProjectSaveSubsystem>()
              : nullptr) {
    SaveSubsystem->GetAllSaveMetadata(AvailableSaveSlots);
  }
}

void UProjectGameViewportClient::BindMapLifecycleDelegates() {
  if (!PreLoadMapHandle.IsValid()) {
    PreLoadMapHandle = FCoreUObjectDelegates::PreLoadMap.AddUObject(
        this, &UProjectGameViewportClient::HandlePreLoadMap);
  }

  if (!PostLoadMapHandle.IsValid()) {
    PostLoadMapHandle = FCoreUObjectDelegates::PostLoadMapWithWorld.AddUObject(
        this, &UProjectGameViewportClient::HandlePostLoadMapWithWorld);
  }
}

void UProjectGameViewportClient::UnbindMapLifecycleDelegates() {
  if (PreLoadMapHandle.IsValid()) {
    FCoreUObjectDelegates::PreLoadMap.Remove(PreLoadMapHandle);
    PreLoadMapHandle.Reset();
  }

  if (PostLoadMapHandle.IsValid()) {
    FCoreUObjectDelegates::PostLoadMapWithWorld.Remove(PostLoadMapHandle);
    PostLoadMapHandle.Reset();
  }
}

void UProjectGameViewportClient::MaybeShowStartupMenu() {
  if (bStartupMenuShown || !bShouldShowStartupMenu || !bStartupMenuEnabled) {
    return;
  }

  UWorld *CurrentWorld = GetWorld();
  if (!CurrentWorld || !CurrentWorld->IsGameWorld()) {
    return;
  }

  if (const UProjectSaveSubsystem *SaveSubsystem =
          GetGameInstance()
              ? GetGameInstance()->GetSubsystem<UProjectSaveSubsystem>()
              : nullptr) {
    if (SaveSubsystem->HasPendingRestore()) {
      return;
    }
  }

  bStartupMenuShown = true;
  ShowStartupMenu();
}

void UProjectGameViewportClient::ShowStartupMenu() {
  if (!bStartupMenuEnabled) {
    return;
  }

  EnsureOverlayWidgets();
  RefreshAvailableSaveSlots();
  ActiveOverlayWidgetType = EProjectViewportOverlayWidgetType::Startup;
  OverlayState = EProjectViewportOverlayState::StartupMenu;
  LoadingScreenPurpose = EProjectLoadingScreenPurpose::None;
  LoadingTitle = FText::GetEmpty();
  LoadingStatus = FText::GetEmpty();
  DisplayedLoadingProgress = 0.0f;
  TargetLoadingProgress = 0.0f;
  LoadingHideAtSeconds = -1.0;

  if (UWorld *CurrentWorld = GetWorld();
      CurrentWorld && !UGameplayStatics::IsGamePaused(CurrentWorld)) {
    UGameplayStatics::SetGamePaused(CurrentWorld, true);
    bPausedForOverlayMenu = true;
  }

  RefreshOverlayWidgets();
  UpdateOverlayInputState();
}

void UProjectGameViewportClient::ShowStartupSaveSelectionMenu() {
  if (!bStartupMenuEnabled) {
    return;
  }

  EnsureOverlayWidgets();
  RefreshAvailableSaveSlots();
  ActiveOverlayWidgetType = EProjectViewportOverlayWidgetType::Startup;
  OverlayState = EProjectViewportOverlayState::SaveSelection;

  if (UWorld *CurrentWorld = GetWorld();
      CurrentWorld && !UGameplayStatics::IsGamePaused(CurrentWorld)) {
    UGameplayStatics::SetGamePaused(CurrentWorld, true);
    bPausedForOverlayMenu = true;
  }

  RefreshOverlayWidgets();
  UpdateOverlayInputState();
}

void UProjectGameViewportClient::HideStartupMenu() {
  if (ActiveOverlayWidgetType == EProjectViewportOverlayWidgetType::Startup &&
      (OverlayState == EProjectViewportOverlayState::StartupMenu ||
       OverlayState == EProjectViewportOverlayState::SaveSelection)) {
    ActiveOverlayWidgetType = EProjectViewportOverlayWidgetType::None;
    OverlayState = EProjectViewportOverlayState::None;
    LoadingScreenPurpose = EProjectLoadingScreenPurpose::None;
  }

  if (bPausedForOverlayMenu) {
    if (UWorld *CurrentWorld = GetWorld();
        CurrentWorld && UGameplayStatics::IsGamePaused(CurrentWorld)) {
      UGameplayStatics::SetGamePaused(CurrentWorld, false);
    }

    bPausedForOverlayMenu = false;
  }

  RefreshOverlayWidgets();
  UpdateOverlayInputState();
}

void UProjectGameViewportClient::ShowInGameMenu() {
  if (!bInGameMenuEnabled) {
    return;
  }

  EnsureOverlayWidgets();
  if (ActiveOverlayWidgetType != EProjectViewportOverlayWidgetType::InGame &&
      !CanShowInGameMenu()) {
    UE_LOG(LogProject, Warning,
           TEXT("ProjectGameViewportClient: ShowInGameMenu aborted by CanShowInGameMenu"));
    return;
  }

  RefreshAvailableSaveSlots();
  ActiveOverlayWidgetType = EProjectViewportOverlayWidgetType::InGame;
  OverlayState = EProjectViewportOverlayState::InGameMenu;
  LoadingScreenPurpose = EProjectLoadingScreenPurpose::None;

  if (UWorld *CurrentWorld = GetWorld();
      CurrentWorld && !UGameplayStatics::IsGamePaused(CurrentWorld)) {
    UGameplayStatics::SetGamePaused(CurrentWorld, true);
    bPausedForOverlayMenu = true;
  }

  RefreshOverlayWidgets();
  UpdateOverlayInputState();

  UE_LOG(
      LogProject, Log,
      TEXT("ProjectGameViewportClient: ShowInGameMenu completed (Widget=%s, InViewport=%s, Visibility=%d, OverlayState=%d)"),
      *GetNameSafe(InGameOverlayWidget),
      InGameOverlayWidget && InGameOverlayWidget->IsInViewport() ? TEXT("true")
                                                                 : TEXT("false"),
      InGameOverlayWidget ? static_cast<int32>(InGameOverlayWidget->GetVisibility())
                          : -1,
      static_cast<int32>(OverlayState));
}

void UProjectGameViewportClient::ShowInGameSaveSelectionMenu() {
  if (!bInGameMenuEnabled) {
    return;
  }

  EnsureOverlayWidgets();
  if (ActiveOverlayWidgetType != EProjectViewportOverlayWidgetType::InGame &&
      !CanShowInGameMenu()) {
    return;
  }

  RefreshAvailableSaveSlots();
  ActiveOverlayWidgetType = EProjectViewportOverlayWidgetType::InGame;
  OverlayState = EProjectViewportOverlayState::SaveSelection;

  if (UWorld *CurrentWorld = GetWorld();
      CurrentWorld && !UGameplayStatics::IsGamePaused(CurrentWorld)) {
    UGameplayStatics::SetGamePaused(CurrentWorld, true);
    bPausedForOverlayMenu = true;
  }

  RefreshOverlayWidgets();
  UpdateOverlayInputState();
}

void UProjectGameViewportClient::HideInGameMenu() {
  if (ActiveOverlayWidgetType == EProjectViewportOverlayWidgetType::InGame &&
      (OverlayState == EProjectViewportOverlayState::InGameMenu ||
       OverlayState == EProjectViewportOverlayState::SaveSelection)) {
    ActiveOverlayWidgetType = EProjectViewportOverlayWidgetType::None;
    OverlayState = EProjectViewportOverlayState::None;
    LoadingScreenPurpose = EProjectLoadingScreenPurpose::None;
  }

  if (bPausedForOverlayMenu) {
    if (UWorld *CurrentWorld = GetWorld();
        CurrentWorld && UGameplayStatics::IsGamePaused(CurrentWorld)) {
      UGameplayStatics::SetGamePaused(CurrentWorld, false);
    }

    bPausedForOverlayMenu = false;
  }

  RefreshOverlayWidgets();
  UpdateOverlayInputState();
}

void UProjectGameViewportClient::BeginLoadingScreen(
    EProjectViewportOverlayWidgetType WidgetType,
    EProjectLoadingScreenPurpose Purpose, const FText &InTitle,
    const FText &InStatus, float InitialProgress) {
  EnsureOverlayWidgets();
  ActiveOverlayWidgetType = WidgetType;
  OverlayState = EProjectViewportOverlayState::LoadingScreen;
  LoadingScreenPurpose = Purpose;
  LoadingTitle = InTitle;
  LoadingStatus = InStatus;
  DisplayedLoadingProgress = FMath::Clamp(InitialProgress, 0.0f, 1.0f);
  TargetLoadingProgress = DisplayedLoadingProgress;
  LoadingHideAtSeconds = -1.0;
  RefreshOverlayWidgets();
  UpdateOverlayInputState();
}

void UProjectGameViewportClient::CompleteLoadingScreen(
    float HoldDurationSeconds) {
  TargetLoadingProgress = 1.0f;
  LoadingHideAtSeconds =
      FPlatformTime::Seconds() + FMath::Max(0.0f, HoldDurationSeconds);
}

void UProjectGameViewportClient::HideLoadingScreen() {
  if (OverlayState != EProjectViewportOverlayState::LoadingScreen) {
    return;
  }

  ActiveOverlayWidgetType = EProjectViewportOverlayWidgetType::None;
  OverlayState = EProjectViewportOverlayState::None;
  LoadingScreenPurpose = EProjectLoadingScreenPurpose::None;
  LoadingTitle = FText::GetEmpty();
  LoadingStatus = FText::GetEmpty();
  DisplayedLoadingProgress = 0.0f;
  TargetLoadingProgress = 0.0f;
  LoadingHideAtSeconds = -1.0;
  RefreshOverlayWidgets();
  UpdateOverlayInputState();
}

void UProjectGameViewportClient::TickLoadingScreen(float DeltaTime) {
  if (OverlayState != EProjectViewportOverlayState::LoadingScreen) {
    return;
  }

  DisplayedLoadingProgress = FMath::FInterpTo(
      DisplayedLoadingProgress, TargetLoadingProgress, DeltaTime, 4.0f);
  if (FMath::IsNearlyEqual(DisplayedLoadingProgress, TargetLoadingProgress,
                           0.005f)) {
    DisplayedLoadingProgress = TargetLoadingProgress;
  }

  if (LoadingHideAtSeconds >= 0.0 &&
      FPlatformTime::Seconds() >= LoadingHideAtSeconds &&
      DisplayedLoadingProgress >= 0.995f) {
    HideLoadingScreen();
  }
}

void UProjectGameViewportClient::SyncLoadingScreenWithSaveSubsystem() {
  UProjectSaveSubsystem *SaveSubsystem =
      GetGameInstance() ? GetGameInstance()->GetSubsystem<UProjectSaveSubsystem>()
                        : nullptr;
  if (!SaveSubsystem) {
    return;
  }

  const EProjectSaveOperationState OperationState =
      SaveSubsystem->GetOperationState();
  const bool bIsLoadOperation =
      OperationState == EProjectSaveOperationState::LoadingSlot ||
      OperationState == EProjectSaveOperationState::OpeningLevel ||
      OperationState == EProjectSaveOperationState::Restoring;

  if (!bIsLoadOperation) {
    if (OverlayState == EProjectViewportOverlayState::LoadingScreen &&
        LoadingScreenPurpose == EProjectLoadingScreenPurpose::LoadingSave &&
        LoadingHideAtSeconds < 0.0) {
      CompleteLoadingScreen();
    }

    return;
  }

  if (OverlayState != EProjectViewportOverlayState::LoadingScreen ||
      LoadingScreenPurpose != EProjectLoadingScreenPurpose::LoadingSave) {
    BeginLoadingScreen(
        ResolveLoadingOverlayWidgetType(),
        EProjectLoadingScreenPurpose::LoadingSave,
        LOCTEXT("LoadingSaveTitle", "Загрузка сохранения"),
        LOCTEXT("LoadingSaveStatusInitial", "Чтение данных сохранения..."),
        0.08f);
  }

  TargetLoadingProgress =
      FMath::Max(TargetLoadingProgress,
                 FMath::Clamp(SaveSubsystem->GetOperationProgress(), 0.0f, 0.99f));

  switch (OperationState) {
  case EProjectSaveOperationState::LoadingSlot:
    LoadingStatus =
        LOCTEXT("LoadingSaveStatusSlot", "Чтение выбранного файла сохранения...");
    break;
  case EProjectSaveOperationState::OpeningLevel:
    LoadingStatus =
        LOCTEXT("LoadingSaveStatusLevel", "Открытие сохранённого уровня...");
    break;
  case EProjectSaveOperationState::Restoring:
    LoadingStatus =
        LOCTEXT("LoadingSaveStatusRestore", "Восстановление мира и игрока...");
    break;
  default:
    break;
  }
}

void UProjectGameViewportClient::UpdateOverlayInputState() {
  const bool bOverlayVisible = OverlayState != EProjectViewportOverlayState::None;

  APlayerController *PlayerController = GetPrimaryPlayerController();
  if (!bOverlayVisible) {
    if (!bOverlayInputStateApplied) {
      return;
    }

    if (APlayerController *AppliedController = OverlayInputPlayerController.Get()) {
      RestoreOverlayInputState(AppliedController);
    } else if (PlayerController) {
      RestoreOverlayInputState(PlayerController);
    }

    bOverlayInputStateApplied = false;
    AppliedOverlayInputState = EProjectViewportOverlayState::None;
    OverlayInputPlayerController.Reset();
    return;
  }

  if (!PlayerController) {
    return;
  }

  const bool bMenuVisible =
      OverlayState == EProjectViewportOverlayState::StartupMenu ||
      OverlayState == EProjectViewportOverlayState::InGameMenu ||
      OverlayState == EProjectViewportOverlayState::SaveSelection;
  const bool bNeedsReapply =
      !bOverlayInputStateApplied ||
      AppliedOverlayInputState != OverlayState ||
      OverlayInputPlayerController.Get() != PlayerController;

  if (!bNeedsReapply) {
    return;
  }

  if (APlayerController *PreviousController = OverlayInputPlayerController.Get();
      PreviousController && PreviousController != PlayerController) {
    RestoreOverlayInputState(PreviousController);
  }

  ApplyOverlayInputState(PlayerController, bMenuVisible, true);
  bOverlayInputStateApplied = true;
  AppliedOverlayInputState = OverlayState;
  OverlayInputPlayerController = PlayerController;
}

void UProjectGameViewportClient::ApplyOverlayInputState(
    APlayerController *PlayerController, bool bMenuVisible,
    bool bBlockGameplayInput) {
  if (!PlayerController) {
    return;
  }

  PlayerController->ResetIgnoreMoveInput();
  PlayerController->ResetIgnoreLookInput();

  if (bBlockGameplayInput) {
    PlayerController->SetIgnoreMoveInput(true);
    PlayerController->SetIgnoreLookInput(true);
  }

  PlayerController->bShowMouseCursor = bMenuVisible;

  if (bMenuVisible) {
    FInputModeGameAndUI InputMode;
    InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
    InputMode.SetHideCursorDuringCapture(false);
    if (UUserWidget *ActiveWidget = GetActiveOverlayWidget()) {
      InputMode.SetWidgetToFocus(ActiveWidget->TakeWidget());
      PlayerController->SetInputMode(InputMode);
      ActiveWidget->SetFocus();
    } else {
      PlayerController->SetInputMode(InputMode);
    }
  } else {
    FInputModeGameOnly InputMode;
    PlayerController->SetInputMode(InputMode);
  }
}

void UProjectGameViewportClient::RestoreOverlayInputState(
    APlayerController *PlayerController) {
  if (!PlayerController) {
    return;
  }

  PlayerController->ResetIgnoreMoveInput();
  PlayerController->ResetIgnoreLookInput();
  PlayerController->bShowMouseCursor = false;

  FInputModeGameOnly InputMode;
  PlayerController->SetInputMode(InputMode);
}

EProjectStartupMenuViewState
UProjectGameViewportClient::GetStartupWidgetViewState() const {
  if (ActiveOverlayWidgetType != EProjectViewportOverlayWidgetType::Startup) {
    return EProjectStartupMenuViewState::Hidden;
  }

  switch (OverlayState) {
  case EProjectViewportOverlayState::StartupMenu:
    return EProjectStartupMenuViewState::StartupMenu;
  case EProjectViewportOverlayState::SaveSelection:
    return EProjectStartupMenuViewState::SaveSelection;
  case EProjectViewportOverlayState::LoadingScreen:
    return EProjectStartupMenuViewState::LoadingScreen;
  default:
    return EProjectStartupMenuViewState::Hidden;
  }
}

EProjectInGameMenuViewState
UProjectGameViewportClient::GetInGameWidgetViewState() const {
  if (ActiveOverlayWidgetType != EProjectViewportOverlayWidgetType::InGame) {
    return EProjectInGameMenuViewState::Hidden;
  }

  switch (OverlayState) {
  case EProjectViewportOverlayState::InGameMenu:
    return EProjectInGameMenuViewState::InGameMenu;
  case EProjectViewportOverlayState::SaveSelection:
    return EProjectInGameMenuViewState::SaveSelection;
  case EProjectViewportOverlayState::LoadingScreen:
    return EProjectInGameMenuViewState::LoadingScreen;
  default:
    return EProjectInGameMenuViewState::Hidden;
  }
}

TSubclassOf<UProjectStartupMenuWidgetBase>
UProjectGameViewportClient::ResolveStartupMenuWidgetClass() const {
  if (UClass *WidgetClass = StartupMenuWidgetClass.LoadSynchronous()) {
    return WidgetClass;
  }

  return UProjectStartupMenuWidgetBase::StaticClass();
}

TSubclassOf<UProjectInGameMenuWidgetBase>
UProjectGameViewportClient::ResolveInGameMenuWidgetClass() const {
  if (UClass *WidgetClass = InGameMenuWidgetClass.LoadSynchronous()) {
    return WidgetClass;
  }

  return UProjectInGameMenuWidgetBase::StaticClass();
}

UUserWidget *UProjectGameViewportClient::GetActiveOverlayWidget() const {
  switch (ActiveOverlayWidgetType) {
  case EProjectViewportOverlayWidgetType::Startup:
    return StartupOverlayWidget;
  case EProjectViewportOverlayWidgetType::InGame:
    return InGameOverlayWidget;
  default:
    return nullptr;
  }
}

UProjectGameViewportClient::EProjectViewportOverlayWidgetType
UProjectGameViewportClient::ResolveLoadingOverlayWidgetType() const {
  if (ActiveOverlayWidgetType != EProjectViewportOverlayWidgetType::None) {
    return ActiveOverlayWidgetType;
  }

  return bShouldShowStartupMenu ? EProjectViewportOverlayWidgetType::Startup
                                : EProjectViewportOverlayWidgetType::InGame;
}

APlayerController *UProjectGameViewportClient::GetPrimaryPlayerController()
    const {
  return GetWorld() ? UGameplayStatics::GetPlayerController(GetWorld(), 0)
                    : nullptr;
}

FString UProjectGameViewportClient::GetStartupGameMapName() const {
  FString MapName = StartupGameMap.TrimStartAndEnd();
  if (MapName.IsEmpty() && GetWorld()) {
    MapName = UGameplayStatics::GetCurrentLevelName(GetWorld(), true);
  }

  return MapName;
}

bool UProjectGameViewportClient::CanSaveGame() const {
  const UProjectSaveSubsystem *SaveSubsystem =
      GetGameInstance() ? GetGameInstance()->GetSubsystem<UProjectSaveSubsystem>()
                        : nullptr;
  UWorld *CurrentWorld = GetWorld();
  const bool bStartupOverlayActive =
      ActiveOverlayWidgetType == EProjectViewportOverlayWidgetType::Startup &&
      OverlayState != EProjectViewportOverlayState::None;
  return SaveSubsystem && SaveSubsystem->CanStartOperation() && CurrentWorld &&
         CurrentWorld->IsGameWorld() && !bStartupOverlayActive;
}

bool UProjectGameViewportClient::CanContinueGame() const {
  const UProjectSaveSubsystem *SaveSubsystem =
      GetGameInstance() ? GetGameInstance()->GetSubsystem<UProjectSaveSubsystem>()
                        : nullptr;
  if (!SaveSubsystem) {
    return false;
  }

  FProjectSaveSlotMetadata Metadata;
  return SaveSubsystem->GetLatestSaveMetadata(Metadata);
}

bool UProjectGameViewportClient::CanOpenSaveSelection() const {
  return CanContinueGame();
}

bool UProjectGameViewportClient::CanShowInGameMenu() const {
  UWorld *CurrentWorld = GetWorld();
  const bool bStartupOverlayActive =
      ActiveOverlayWidgetType == EProjectViewportOverlayWidgetType::Startup &&
      OverlayState != EProjectViewportOverlayState::None;
  if (!bInGameMenuEnabled || !CurrentWorld || !CurrentWorld->IsGameWorld() ||
      bStartupOverlayActive) {
    return false;
  }

  const UProjectSaveSubsystem *SaveSubsystem =
      GetGameInstance() ? GetGameInstance()->GetSubsystem<UProjectSaveSubsystem>()
                        : nullptr;
  if (!SaveSubsystem) {
    return true;
  }

  if (SaveSubsystem->HasPendingRestore()) {
    return false;
  }

  const EProjectSaveOperationState OperationState =
      SaveSubsystem->GetOperationState();
  return OperationState != EProjectSaveOperationState::LoadingSlot &&
         OperationState != EProjectSaveOperationState::OpeningLevel &&
         OperationState != EProjectSaveOperationState::Restoring;
}

FText UProjectGameViewportClient::GetLoadingScreenTitle() const {
  return LoadingTitle.IsEmpty() ? LOCTEXT("DefaultLoadingTitle", "Загрузка")
                                : LoadingTitle;
}

FText UProjectGameViewportClient::GetLoadingScreenStatus() const {
  return LoadingStatus.IsEmpty()
             ? LOCTEXT("DefaultLoadingStatus",
                       "Подождите, идёт подготовка...")
             : LoadingStatus;
}

bool UProjectGameViewportClient::ShouldPromptToExitGame() const {
  return ActiveOverlayWidgetType == EProjectViewportOverlayWidgetType::Startup &&
         OverlayState == EProjectViewportOverlayState::StartupMenu;
}

bool UProjectGameViewportClient::ConfirmExitGameRequest() const {
  const EAppReturnType::Type Result = FMessageDialog::Open(
      EAppMsgType::YesNo, EAppReturnType::No,
      LOCTEXT("ConfirmExitGameMessage", "Выйти из игры?"),
      LOCTEXT("ConfirmExitGameTitle", "Подтверждение выхода"));
  return Result == EAppReturnType::Yes;
}

void UProjectGameViewportClient::ExecuteExitGameRequest() {
  if (APlayerController *PlayerController = GetPrimaryPlayerController()) {
    UKismetSystemLibrary::QuitGame(this, PlayerController,
                                   EQuitPreference::Quit, false);
    return;
  }

  RequestProcessExit();
}

void UProjectGameViewportClient::HandleStartGameRequested() {
  bShouldShowStartupMenu = false;
  HideStartupMenu();

  const FString MapName = GetStartupGameMapName();
  if (MapName.IsEmpty()) {
    ShowStartupMenu();
    return;
  }

  BeginLoadingScreen(EProjectViewportOverlayWidgetType::Startup,
                     EProjectLoadingScreenPurpose::StartingNewGame,
                     LOCTEXT("StartLoadingTitle", "Запуск игры"),
                     LOCTEXT("StartLoadingStatus", "Подготовка уровня..."),
                     0.08f);
  TargetLoadingProgress = 0.15f;

  UGameplayStatics::OpenLevel(this, FName(*MapName));
}

void UProjectGameViewportClient::HandleContinueGameRequested() {
  UProjectSaveSubsystem *SaveSubsystem =
      GetGameInstance() ? GetGameInstance()->GetSubsystem<UProjectSaveSubsystem>()
                        : nullptr;
  if (!SaveSubsystem) {
    return;
  }

  bShouldShowStartupMenu = false;
  HideStartupMenu();
  BeginLoadingScreen(EProjectViewportOverlayWidgetType::Startup,
                     EProjectLoadingScreenPurpose::LoadingSave,
                     LOCTEXT("ContinueLoadingTitle", "Продолжение игры"),
                     LOCTEXT("ContinueLoadingStatus",
                             "Поиск последнего сохранения..."),
                     0.08f);

  if (!SaveSubsystem->ContinueFromLatestSave()) {
    HideLoadingScreen();
    ShowStartupMenu();
  }
}

void UProjectGameViewportClient::HandleStartupOpenSaveSelectionRequested() {
  ShowStartupSaveSelectionMenu();
}

void UProjectGameViewportClient::HandleBackToStartupMenuRequested() {
  ShowStartupMenu();
}

void UProjectGameViewportClient::HandleStartupLoadSelectedSaveRequested(
    const FString &SlotName) {
  UProjectSaveSubsystem *SaveSubsystem =
      GetGameInstance() ? GetGameInstance()->GetSubsystem<UProjectSaveSubsystem>()
                        : nullptr;
  if (!SaveSubsystem) {
    return;
  }

  bShouldShowStartupMenu = false;
  HideStartupMenu();
  BeginLoadingScreen(
      EProjectViewportOverlayWidgetType::Startup,
      EProjectLoadingScreenPurpose::LoadingSave,
      LOCTEXT("ManualLoadingTitle", "Загрузка сохранения"),
      LOCTEXT("ManualLoadingStatus",
              "Подготовка выбранного сохранения..."),
      0.08f);

  if (!SaveSubsystem->LoadSaveSlot(SlotName)) {
    HideLoadingScreen();
    ShowStartupMenu();
  }
}

void UProjectGameViewportClient::HandleResumeGameRequested() {
  HideInGameMenu();
}

void UProjectGameViewportClient::HandleSaveGameRequested() {
  UProjectSaveSubsystem *SaveSubsystem =
      GetGameInstance() ? GetGameInstance()->GetSubsystem<UProjectSaveSubsystem>()
                        : nullptr;
  if (!SaveSubsystem) {
    return;
  }

  if (SaveSubsystem->ManualSave()) {
    bPendingSaveSelectionRefresh = true;
  }
}

void UProjectGameViewportClient::HandleInGameOpenSaveSelectionRequested() {
  ShowInGameSaveSelectionMenu();
}

void UProjectGameViewportClient::HandleBackToInGameMenuRequested() {
  ShowInGameMenu();
}

void UProjectGameViewportClient::HandleInGameLoadSelectedSaveRequested(
    const FString &SlotName) {
  UProjectSaveSubsystem *SaveSubsystem =
      GetGameInstance() ? GetGameInstance()->GetSubsystem<UProjectSaveSubsystem>()
                        : nullptr;
  if (!SaveSubsystem) {
    return;
  }

  HideInGameMenu();
  BeginLoadingScreen(
      EProjectViewportOverlayWidgetType::InGame,
      EProjectLoadingScreenPurpose::LoadingSave,
      LOCTEXT("InGameManualLoadingTitle", "Загрузка сохранения"),
      LOCTEXT("InGameManualLoadingStatus",
              "Подготовка выбранного сохранения..."),
      0.08f);

  if (!SaveSubsystem->LoadSaveSlot(SlotName)) {
    HideLoadingScreen();
    ShowInGameMenu();
  }
}

void UProjectGameViewportClient::HandleOverwriteSaveSlotRequested(
    const FString &SlotName) {
  UProjectSaveSubsystem *SaveSubsystem =
      GetGameInstance() ? GetGameInstance()->GetSubsystem<UProjectSaveSubsystem>()
                        : nullptr;
  if (!SaveSubsystem) {
    return;
  }

  if (SaveSubsystem->OverwriteSaveSlot(SlotName)) {
    bPendingSaveSelectionRefresh = true;
  }
}

void UProjectGameViewportClient::HandleDeleteSaveSlotRequested(
    const FString &SlotName) {
  UProjectSaveSubsystem *SaveSubsystem =
      GetGameInstance() ? GetGameInstance()->GetSubsystem<UProjectSaveSubsystem>()
                        : nullptr;
  if (!SaveSubsystem) {
    return;
  }

  SaveSubsystem->DeleteSaveSlot(SlotName);
  RefreshAvailableSaveSlots();
  RefreshOverlayWidgets();
}

void UProjectGameViewportClient::HandleReturnToMainMenuRequested() {
  const FString MapName = GetStartupGameMapName();

  HideInGameMenu();
  bShouldShowStartupMenu = true;
  bStartupMenuShown = false;

  if (MapName.IsEmpty()) {
    ShowStartupMenu();
    return;
  }

  BeginLoadingScreen(EProjectViewportOverlayWidgetType::InGame,
                     EProjectLoadingScreenPurpose::ReturningToMainMenu,
                     LOCTEXT("ReturnToMainMenuTitle", "Главное меню"),
                     LOCTEXT("ReturnToMainMenuStatus",
                             "Возврат в главное меню..."),
                     0.08f);
  TargetLoadingProgress = 0.15f;

  UGameplayStatics::OpenLevel(this, FName(*MapName));
}

void UProjectGameViewportClient::HandleExitGameRequested() {
  if (ShouldPromptToExitGame() && !ConfirmExitGameRequest()) {
    return;
  }

  ExecuteExitGameRequest();
}

void UProjectGameViewportClient::HandlePreLoadMap(const FString &MapName) {
  if (OverlayState != EProjectViewportOverlayState::LoadingScreen) {
    return;
  }

  switch (LoadingScreenPurpose) {
  case EProjectLoadingScreenPurpose::StartingNewGame:
    LoadingStatus =
        LOCTEXT("StartLoadingOpenLevel", "Открытие стартового уровня...");
    TargetLoadingProgress = FMath::Max(TargetLoadingProgress, 0.42f);
    break;
  case EProjectLoadingScreenPurpose::LoadingSave:
    LoadingStatus =
        LOCTEXT("SaveLoadingOpenLevel", "Переход на сохранённый уровень...");
    TargetLoadingProgress = FMath::Max(TargetLoadingProgress, 0.62f);
    break;
  case EProjectLoadingScreenPurpose::ReturningToMainMenu:
    LoadingStatus =
        LOCTEXT("ReturnToMenuOpenLevel", "Открытие стартовой карты...");
    TargetLoadingProgress = FMath::Max(TargetLoadingProgress, 0.55f);
    break;
  default:
    break;
  }
}

void UProjectGameViewportClient::HandlePostLoadMapWithWorld(
    UWorld *LoadedWorld) {
  if (!LoadedWorld || !LoadedWorld->IsGameWorld()) {
    return;
  }

  switch (LoadingScreenPurpose) {
  case EProjectLoadingScreenPurpose::StartingNewGame:
    LoadingStatus =
        LOCTEXT("StartLoadingComplete", "Инициализация новой игры...");
    TargetLoadingProgress = FMath::Max(TargetLoadingProgress, 0.92f);
    CompleteLoadingScreen();
    break;
  case EProjectLoadingScreenPurpose::LoadingSave:
    LoadingStatus =
        LOCTEXT("SaveLoadingRestore", "Подготовка сохранённого мира...");
    TargetLoadingProgress = FMath::Max(TargetLoadingProgress, 0.78f);
    break;
  case EProjectLoadingScreenPurpose::ReturningToMainMenu:
    LoadingStatus =
        LOCTEXT("ReturnToMenuComplete", "Подготовка главного меню...");
    TargetLoadingProgress = FMath::Max(TargetLoadingProgress, 0.92f);
    CompleteLoadingScreen(0.0f);
    break;
  default:
    break;
  }
}

bool UProjectGameViewportClient::ShouldRequestProcessExit() const {
#if WITH_EDITOR
  if (GIsEditor) {
    return false;
  }
#endif

  return true;
}

void UProjectGameViewportClient::RequestProcessExit() {
  if (bHasRequestedProcessExit || !ShouldRequestProcessExit()) {
    return;
  }

  bHasRequestedProcessExit = true;

  UE_LOG(
      LogProject, Log,
      TEXT("ProjectGameViewportClient: requesting process exit after window close"));
  FPlatformMisc::RequestExit(
      false, TEXT("UProjectGameViewportClient::RequestProcessExit"));
}

#undef LOCTEXT_NAMESPACE
