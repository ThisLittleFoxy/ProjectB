#pragma once

#include "CoreMinimal.h"
#include "Engine/GameViewportClient.h"
#include "Save/ProjectSaveIndex.h"
#include "UObject/SoftObjectPtr.h"
#include "ProjectGameViewportClient.generated.h"

class APlayerController;
class UProjectInGameMenuWidgetBase;
class UProjectStartupMenuWidgetBase;
class UUserWidget;
enum class EProjectInGameMenuViewState : uint8;
enum class EProjectStartupMenuViewState : uint8;

UCLASS(Config = Engine)
class PROJECT_API UProjectGameViewportClient : public UGameViewportClient {
  GENERATED_BODY()

public:
  virtual void Tick(float DeltaTime) override;
  virtual void BeginDestroy() override;
  virtual bool WindowCloseRequested() override;
  virtual void CloseRequested(FViewport *InViewport) override;
  bool HandleEscapeMenuAction();
  void SetStartupMenuEnabled(bool bEnabled);
  bool IsStartupMenuEnabled() const { return bStartupMenuEnabled; }
  void SetInGameMenuEnabled(bool bEnabled);
  bool IsInGameMenuEnabled() const { return bInGameMenuEnabled; }

private:
  enum class EProjectViewportOverlayWidgetType : uint8 {
    None,
    Startup,
    InGame
  };

  enum class EProjectViewportOverlayState : uint8 {
    None,
    StartupMenu,
    InGameMenu,
    SaveSelection,
    LoadingScreen
  };

  enum class EProjectLoadingScreenPurpose : uint8 {
    None,
    StartingNewGame,
    LoadingSave,
    ReturningToMainMenu
  };

  void EnsureOverlayWidgets();
  void RefreshOverlayWidgets();
  void RefreshAvailableSaveSlots();
  void BindMapLifecycleDelegates();
  void UnbindMapLifecycleDelegates();
  bool IsArenaRuntimeWorld() const;
  void SuppressOverlaysForArenaRuntime();
  void MaybeShowStartupMenu();
  void ShowStartupMenu();
  void ShowStartupSaveSelectionMenu();
  void HideStartupMenu();
  void ShowInGameMenu();
  void ShowInGameSaveSelectionMenu();
  void HideInGameMenu();
  void BeginLoadingScreen(EProjectViewportOverlayWidgetType WidgetType,
                          EProjectLoadingScreenPurpose Purpose,
                          const FText &InTitle, const FText &InStatus,
                          float InitialProgress = 0.0f);
  void CompleteLoadingScreen(float HoldDurationSeconds = 0.35f);
  void HideLoadingScreen();
  void TickLoadingScreen(float DeltaTime);
  void SyncLoadingScreenWithSaveSubsystem();
  void UpdateOverlayInputState();
  void ApplyOverlayInputState(APlayerController *PlayerController,
                              bool bMenuVisible, bool bBlockGameplayInput);
  void RestoreOverlayInputState(APlayerController *PlayerController);
  EProjectStartupMenuViewState GetStartupWidgetViewState() const;
  EProjectInGameMenuViewState GetInGameWidgetViewState() const;
  TSubclassOf<UProjectStartupMenuWidgetBase> ResolveStartupMenuWidgetClass() const;
  TSubclassOf<UProjectInGameMenuWidgetBase> ResolveInGameMenuWidgetClass() const;
  UUserWidget *GetActiveOverlayWidget() const;
  EProjectViewportOverlayWidgetType ResolveLoadingOverlayWidgetType() const;
  APlayerController *GetPrimaryPlayerController() const;
  FString GetStartupGameMapName() const;
  bool CanSaveGame() const;
  bool CanContinueGame() const;
  bool CanOpenSaveSelection() const;
  bool CanShowInGameMenu() const;
  FText GetLoadingScreenTitle() const;
  FText GetLoadingScreenStatus() const;
  bool ShouldPromptToExitGame() const;
  bool ConfirmExitGameRequest() const;
  void ExecuteExitGameRequest();
  void HandleStartGameRequested();
  void HandleContinueGameRequested();
  void HandleStartupOpenSaveSelectionRequested();
  void HandleBackToStartupMenuRequested();
  void HandleStartupLoadSelectedSaveRequested(const FString &SlotName);
  void HandleResumeGameRequested();
  void HandleSaveGameRequested();
  void HandleInGameOpenSaveSelectionRequested();
  void HandleBackToInGameMenuRequested();
  void HandleInGameLoadSelectedSaveRequested(const FString &SlotName);
  void HandleOverwriteSaveSlotRequested(const FString &SlotName);
  void HandleDeleteSaveSlotRequested(const FString &SlotName);
  void HandleReturnToMainMenuRequested();
  void HandleExitGameRequested();
  void HandlePreLoadMap(const FString &MapName);
  void HandlePostLoadMapWithWorld(UWorld *LoadedWorld);
  bool ShouldRequestProcessExit() const;
  void RequestProcessExit();

  UPROPERTY(EditAnywhere, Config, Category = "Startup")
  FString StartupGameMap = TEXT("/Game/Maps/TestLvl");

  UPROPERTY(EditAnywhere, Config, Category = "Startup")
  TSoftClassPtr<UProjectStartupMenuWidgetBase> StartupMenuWidgetClass;

  UPROPERTY(EditAnywhere, Config, Category = "In Game Menu")
  TSoftClassPtr<UProjectInGameMenuWidgetBase> InGameMenuWidgetClass;

  UPROPERTY(Transient)
  TObjectPtr<UProjectStartupMenuWidgetBase> StartupOverlayWidget;

  UPROPERTY(Transient)
  TObjectPtr<UProjectInGameMenuWidgetBase> InGameOverlayWidget;

  UPROPERTY(Transient)
  TWeakObjectPtr<APlayerController> OverlayInputPlayerController;

  TArray<FProjectSaveSlotMetadata> AvailableSaveSlots;
  FDelegateHandle PreLoadMapHandle;
  FDelegateHandle PostLoadMapHandle;
  EProjectViewportOverlayWidgetType ActiveOverlayWidgetType =
      EProjectViewportOverlayWidgetType::None;
  EProjectViewportOverlayState OverlayState = EProjectViewportOverlayState::None;
  EProjectLoadingScreenPurpose LoadingScreenPurpose =
      EProjectLoadingScreenPurpose::None;
  EProjectViewportOverlayState AppliedOverlayInputState =
      EProjectViewportOverlayState::None;
  FText LoadingTitle;
  FText LoadingStatus;
  float DisplayedLoadingProgress = 0.0f;
  float TargetLoadingProgress = 0.0f;
  double LoadingHideAtSeconds = -1.0;
  bool bStartupMenuShown = false;
  bool bShouldShowStartupMenu = true;
  bool bStartupMenuEnabled = true;
  bool bInGameMenuEnabled = true;
  bool bOverlayInputStateApplied = false;
  bool bPausedForOverlayMenu = false;
  bool bHasRequestedProcessExit = false;
  bool bPendingSaveSelectionRefresh = false;
};
