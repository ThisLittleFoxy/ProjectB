#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Save/ProjectSaveIndex.h"
#include "ProjectInGameMenuWidgetBase.generated.h"

class UButton;
class UPanelWidget;
class UProgressBar;
class UProjectSaveSlotEntryWidgetBase;
class UTextBlock;
class UWidget;

UENUM(BlueprintType)
enum class EProjectInGameMenuViewState : uint8 {
  Hidden UMETA(DisplayName = "Hidden"),
  InGameMenu UMETA(DisplayName = "In Game Menu"),
  SaveSelection UMETA(DisplayName = "Save Selection"),
  LoadingScreen UMETA(DisplayName = "Loading Screen")
};

DECLARE_MULTICAST_DELEGATE(FProjectInGameMenuActionDelegate);
DECLARE_MULTICAST_DELEGATE_OneParam(FProjectInGameMenuLoadSlotDelegate,
                                    const FString &);

UCLASS(BlueprintType, Blueprintable)
class PROJECT_API UProjectInGameMenuWidgetBase : public UUserWidget {
  GENERATED_BODY()

public:
  UProjectInGameMenuWidgetBase(const FObjectInitializer &ObjectInitializer);

  virtual TSharedRef<SWidget> RebuildWidget() override;
  virtual void NativeConstruct() override;
  virtual void NativeDestruct() override;
  virtual FReply NativeOnKeyDown(const FGeometry &InGeometry,
                                 const FKeyEvent &InKeyEvent) override;

  UFUNCTION(BlueprintCallable, Category = "In Game Menu")
  void SetViewState(EProjectInGameMenuViewState NewState);

  UFUNCTION(BlueprintPure, Category = "In Game Menu")
  EProjectInGameMenuViewState GetViewState() const { return ViewState; }

  UFUNCTION(BlueprintCallable, Category = "In Game Menu")
  void SetCanSaveGame(bool bInCanSaveGame);

  UFUNCTION(BlueprintPure, Category = "In Game Menu")
  bool GetCanSaveGame() const { return bCanSaveGame; }

  UFUNCTION(BlueprintCallable, Category = "In Game Menu")
  void SetCanOpenSaveSelection(bool bInCanOpenSaveSelection);

  UFUNCTION(BlueprintPure, Category = "In Game Menu")
  bool GetCanOpenSaveSelection() const { return bCanOpenSaveSelection; }

  UFUNCTION(BlueprintCallable, Category = "In Game Menu")
  void SetAvailableSaveSlots(const TArray<FProjectSaveSlotMetadata> &NewSaveSlots);

  UFUNCTION(BlueprintPure, Category = "In Game Menu")
  TArray<FProjectSaveSlotMetadata> GetAvailableSaveSlots() const {
    return AvailableSaveSlots;
  }

  UFUNCTION(BlueprintCallable, Category = "In Game Menu")
  void SetLoadingState(const FText &InTitle, const FText &InStatus,
                       float InProgress);

  UFUNCTION(BlueprintPure, Category = "In Game Menu")
  FText GetLoadingTitle() const { return LoadingTitle; }

  UFUNCTION(BlueprintPure, Category = "In Game Menu")
  FText GetLoadingStatus() const { return LoadingStatus; }

  UFUNCTION(BlueprintPure, Category = "In Game Menu")
  float GetLoadingProgress() const { return LoadingProgress; }

  UFUNCTION(BlueprintPure, Category = "In Game Menu")
  FText GetLoadingPercentText() const;

  UFUNCTION(BlueprintCallable, Category = "In Game Menu")
  void RequestResumeGame();

  UFUNCTION(BlueprintCallable, Category = "In Game Menu")
  void RequestSaveGame();

  UFUNCTION(BlueprintCallable, Category = "In Game Menu")
  void RequestOpenSaveSelection();

  UFUNCTION(BlueprintCallable, Category = "In Game Menu")
  void RequestBackToInGameMenu();

  UFUNCTION(BlueprintCallable, Category = "In Game Menu")
  void RequestLoadSaveSlot(const FString &SlotName);

  UFUNCTION(BlueprintCallable, Category = "In Game Menu")
  void RequestOverwriteSaveSlot(const FString &SlotName);

  UFUNCTION(BlueprintCallable, Category = "In Game Menu")
  void RequestDeleteSaveSlot(const FString &SlotName);

  UFUNCTION(BlueprintCallable, Category = "In Game Menu")
  void RequestReturnToMainMenu();

  UFUNCTION(BlueprintCallable, Category = "In Game Menu")
  void RequestExitGame();

  UFUNCTION(BlueprintImplementableEvent, Category = "In Game Menu")
  void BP_OnInGameMenuRefreshed();

  UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "In Game Menu")
  TSubclassOf<UProjectSaveSlotEntryWidgetBase> SaveSlotEntryWidgetClass;

  FProjectInGameMenuActionDelegate OnResumeRequested;
  FProjectInGameMenuActionDelegate OnSaveGameRequested;
  FProjectInGameMenuActionDelegate OnOpenSaveSelectionRequested;
  FProjectInGameMenuActionDelegate OnBackToInGameMenuRequested;
  FProjectInGameMenuActionDelegate OnReturnToMainMenuRequested;
  FProjectInGameMenuActionDelegate OnExitGameRequested;
  FProjectInGameMenuLoadSlotDelegate OnLoadSaveSlotRequested;
  FProjectInGameMenuLoadSlotDelegate OnOverwriteSaveSlotRequested;
  FProjectInGameMenuLoadSlotDelegate OnDeleteSaveSlotRequested;

protected:
  UPROPERTY(BlueprintReadOnly, Category = "In Game Menu")
  EProjectInGameMenuViewState ViewState =
      EProjectInGameMenuViewState::Hidden;

  UPROPERTY(BlueprintReadOnly, Category = "In Game Menu")
  bool bCanSaveGame = true;

  UPROPERTY(BlueprintReadOnly, Category = "In Game Menu")
  bool bCanOpenSaveSelection = false;

  UPROPERTY(BlueprintReadOnly, Category = "In Game Menu")
  TArray<FProjectSaveSlotMetadata> AvailableSaveSlots;

  UPROPERTY(BlueprintReadOnly, Category = "In Game Menu")
  FText LoadingTitle;

  UPROPERTY(BlueprintReadOnly, Category = "In Game Menu")
  FText LoadingStatus;

  UPROPERTY(BlueprintReadOnly, Category = "In Game Menu")
  float LoadingProgress = 0.0f;

private:
  UFUNCTION()
  void HandleSaveButtonClicked();

  UFUNCTION()
  void HandleLoadButtonClicked();

  UFUNCTION()
  void HandleBackButtonClicked();

  UFUNCTION()
  void HandleReturnToMainMenuClicked();

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
  TSubclassOf<UProjectSaveSlotEntryWidgetBase> ResolveSaveSlotEntryWidgetClass()
      const;

  UPROPERTY(Transient)
  TObjectPtr<UWidget> CachedInGameMenuPanel;

  UPROPERTY(Transient)
  TObjectPtr<UWidget> CachedSaveSelectionPanel;

  UPROPERTY(Transient)
  TObjectPtr<UWidget> CachedLoadingPanel;

  UPROPERTY(Transient)
  TObjectPtr<UButton> CachedSaveButton;

  UPROPERTY(Transient)
  TObjectPtr<UButton> CachedLoadButton;

  UPROPERTY(Transient)
  TObjectPtr<UButton> CachedSettingsButton;

  UPROPERTY(Transient)
  TObjectPtr<UButton> CachedReturnToMainMenuButton;

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
