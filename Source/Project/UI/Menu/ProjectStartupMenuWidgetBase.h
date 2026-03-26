#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Save/ProjectSaveIndex.h"
#include "ProjectStartupMenuWidgetBase.generated.h"

class UButton;
class UPanelWidget;
class UProgressBar;
class UProjectSaveSlotEntryWidgetBase;
class UTextBlock;
class UWidget;

UENUM(BlueprintType)
enum class EProjectStartupMenuViewState : uint8 {
  Hidden UMETA(DisplayName = "Hidden"),
  StartupMenu UMETA(DisplayName = "Startup Menu"),
  SaveSelection UMETA(DisplayName = "Save Selection"),
  LoadingScreen UMETA(DisplayName = "Loading Screen")
};

DECLARE_MULTICAST_DELEGATE(FProjectStartupMenuActionDelegate);
DECLARE_MULTICAST_DELEGATE_OneParam(FProjectStartupMenuLoadSlotDelegate,
                                    const FString &);

UCLASS(BlueprintType, Blueprintable)
class PROJECT_API UProjectStartupMenuWidgetBase : public UUserWidget {
  GENERATED_BODY()

public:
  UProjectStartupMenuWidgetBase(const FObjectInitializer &ObjectInitializer);

  virtual TSharedRef<SWidget> RebuildWidget() override;
  virtual void NativeConstruct() override;
  virtual void NativeDestruct() override;

  UFUNCTION(BlueprintCallable, Category = "Startup Menu")
  void SetViewState(EProjectStartupMenuViewState NewState);

  UFUNCTION(BlueprintPure, Category = "Startup Menu")
  EProjectStartupMenuViewState GetViewState() const { return ViewState; }

  UFUNCTION(BlueprintCallable, Category = "Startup Menu")
  void SetCanContinueGame(bool bInCanContinueGame);

  UFUNCTION(BlueprintPure, Category = "Startup Menu")
  bool GetCanContinueGame() const { return bCanContinueGame; }

  UFUNCTION(BlueprintCallable, Category = "Startup Menu")
  void SetCanOpenSaveSelection(bool bInCanOpenSaveSelection);

  UFUNCTION(BlueprintPure, Category = "Startup Menu")
  bool GetCanOpenSaveSelection() const { return bCanOpenSaveSelection; }

  UFUNCTION(BlueprintCallable, Category = "Startup Menu")
  void SetAvailableSaveSlots(const TArray<FProjectSaveSlotMetadata> &NewSaveSlots);

  UFUNCTION(BlueprintPure, Category = "Startup Menu")
  TArray<FProjectSaveSlotMetadata> GetAvailableSaveSlots() const {
    return AvailableSaveSlots;
  }

  UFUNCTION(BlueprintCallable, Category = "Startup Menu")
  void SetLoadingState(const FText &InTitle, const FText &InStatus,
                       float InProgress);

  UFUNCTION(BlueprintPure, Category = "Startup Menu")
  FText GetLoadingTitle() const { return LoadingTitle; }

  UFUNCTION(BlueprintPure, Category = "Startup Menu")
  FText GetLoadingStatus() const { return LoadingStatus; }

  UFUNCTION(BlueprintPure, Category = "Startup Menu")
  float GetLoadingProgress() const { return LoadingProgress; }

  UFUNCTION(BlueprintPure, Category = "Startup Menu")
  FText GetLoadingPercentText() const;

  UFUNCTION(BlueprintCallable, Category = "Startup Menu")
  void RequestStartGame();

  UFUNCTION(BlueprintCallable, Category = "Startup Menu")
  void RequestContinueGame();

  UFUNCTION(BlueprintCallable, Category = "Startup Menu")
  void RequestOpenSaveSelection();

  UFUNCTION(BlueprintCallable, Category = "Startup Menu")
  void RequestBackToStartupMenu();

  UFUNCTION(BlueprintCallable, Category = "Startup Menu")
  void RequestLoadSaveSlot(const FString &SlotName);

  UFUNCTION(BlueprintCallable, Category = "Startup Menu")
  void RequestOverwriteSaveSlot(const FString &SlotName);

  UFUNCTION(BlueprintCallable, Category = "Startup Menu")
  void RequestDeleteSaveSlot(const FString &SlotName);

  UFUNCTION(BlueprintCallable, Category = "Startup Menu")
  void RequestExitGame();

  UFUNCTION(BlueprintImplementableEvent, Category = "Startup Menu")
  void BP_OnStartupMenuRefreshed();

  UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Startup Menu")
  TSubclassOf<UProjectSaveSlotEntryWidgetBase> SaveSlotEntryWidgetClass;

  FProjectStartupMenuActionDelegate OnStartGameRequested;
  FProjectStartupMenuActionDelegate OnContinueGameRequested;
  FProjectStartupMenuActionDelegate OnOpenSaveSelectionRequested;
  FProjectStartupMenuActionDelegate OnBackToStartupMenuRequested;
  FProjectStartupMenuActionDelegate OnExitGameRequested;
  FProjectStartupMenuLoadSlotDelegate OnLoadSaveSlotRequested;
  FProjectStartupMenuLoadSlotDelegate OnOverwriteSaveSlotRequested;
  FProjectStartupMenuLoadSlotDelegate OnDeleteSaveSlotRequested;

protected:
  UPROPERTY(BlueprintReadOnly, Category = "Startup Menu")
  EProjectStartupMenuViewState ViewState =
      EProjectStartupMenuViewState::Hidden;

  UPROPERTY(BlueprintReadOnly, Category = "Startup Menu")
  bool bCanContinueGame = false;

  UPROPERTY(BlueprintReadOnly, Category = "Startup Menu")
  bool bCanOpenSaveSelection = false;

  UPROPERTY(BlueprintReadOnly, Category = "Startup Menu")
  TArray<FProjectSaveSlotMetadata> AvailableSaveSlots;

  UPROPERTY(BlueprintReadOnly, Category = "Startup Menu")
  FText LoadingTitle;

  UPROPERTY(BlueprintReadOnly, Category = "Startup Menu")
  FText LoadingStatus;

  UPROPERTY(BlueprintReadOnly, Category = "Startup Menu")
  float LoadingProgress = 0.0f;

private:
  UFUNCTION()
  void HandleStartButtonClicked();

  UFUNCTION()
  void HandleContinueButtonClicked();

  UFUNCTION()
  void HandleLoadButtonClicked();

  UFUNCTION()
  void HandleBackButtonClicked();

  UFUNCTION()
  void HandleExitButtonClicked();

  void BuildDefaultLayout();
  void InitializeNamedWidgets();
  void RefreshVisualState();
  void RefreshSaveSlotList();
  void RefreshLoadingState();
  void CleanupButtonBindings();
  bool AreSaveSlotsEquivalent(
      const TArray<FProjectSaveSlotMetadata> &OtherSaveSlots) const;
  TSubclassOf<UProjectSaveSlotEntryWidgetBase> ResolveSaveSlotEntryWidgetClass() const;

  UPROPERTY(Transient)
  TObjectPtr<UWidget> CachedStartupMenuPanel;

  UPROPERTY(Transient)
  TObjectPtr<UWidget> CachedSaveSelectionPanel;

  UPROPERTY(Transient)
  TObjectPtr<UWidget> CachedLoadingPanel;

  UPROPERTY(Transient)
  TObjectPtr<UButton> CachedStartButton;

  UPROPERTY(Transient)
  TObjectPtr<UButton> CachedContinueButton;

  UPROPERTY(Transient)
  TObjectPtr<UButton> CachedLoadButton;

  UPROPERTY(Transient)
  TObjectPtr<UButton> CachedSettingsButton;

  UPROPERTY(Transient)
  TObjectPtr<UButton> CachedExitButton;

  UPROPERTY(Transient)
  TObjectPtr<UButton> CachedBackButton;

  UPROPERTY(Transient)
  TObjectPtr<UPanelWidget> CachedSaveSlotList;

  UPROPERTY(Transient)
  TObjectPtr<UTextBlock> CachedEmptySaveSlotsText;

  UPROPERTY(Transient)
  TObjectPtr<UTextBlock> CachedLoadingTitleText;

  UPROPERTY(Transient)
  TObjectPtr<UTextBlock> CachedLoadingStatusText;

  UPROPERTY(Transient)
  TObjectPtr<UTextBlock> CachedLoadingPercentText;

  UPROPERTY(Transient)
  TObjectPtr<UProgressBar> CachedLoadingProgressBar;
};
