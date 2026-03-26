#include "UI/Menu/ProjectStartupMenuWidgetBase.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/PanelSlot.h"
#include "Components/PanelWidget.h"
#include "Components/ProgressBar.h"
#include "Components/ScrollBox.h"
#include "Components/ScrollBoxSlot.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Engine/GameInstance.h"
#include "UI/Menu/ProjectSaveSlotEntryWidgetBase.h"

#define LOCTEXT_NAMESPACE "ProjectStartupMenuWidgetBase"

namespace {
void ApplyTextStyle(UTextBlock *TextBlock, int32 FontSize,
                    const FLinearColor &Color, FName Typeface,
                    ETextJustify::Type Justification = ETextJustify::Left) {
  if (!TextBlock) {
    return;
  }

  FSlateFontInfo FontInfo = TextBlock->GetFont();
  FontInfo.Size = FontSize;
  FontInfo.TypefaceFontName = Typeface;
  TextBlock->SetFont(FontInfo);
  TextBlock->SetColorAndOpacity(Color);
  TextBlock->SetAutoWrapText(true);
  TextBlock->SetJustification(Justification);
}

UButton *CreateMenuButton(UWidgetTree *WidgetTree, UVerticalBox *ParentBox,
                          const FName &ButtonName, const FName &LabelName,
                          const FText &LabelText, float BottomPadding) {
  if (!WidgetTree || !ParentBox) {
    return nullptr;
  }

  USizeBox *ButtonSizeBox =
      WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), NAME_None);
  ButtonSizeBox->SetHeightOverride(54.0f);
  if (UVerticalBoxSlot *BoxSlot = ParentBox->AddChildToVerticalBox(ButtonSizeBox)) {
    BoxSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, BottomPadding));
  }

  UButton *Button =
      WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), ButtonName);
  ButtonSizeBox->SetContent(Button);

  UTextBlock *Label = WidgetTree->ConstructWidget<UTextBlock>(
      UTextBlock::StaticClass(), LabelName);
  Label->SetText(LabelText);
  ApplyTextStyle(Label, 18, FLinearColor::White, TEXT("Bold"),
                 ETextJustify::Center);
  Button->SetContent(Label);

  return Button;
}

bool AreSaveSlotsEqual(const FProjectSaveSlotMetadata &Left,
                       const FProjectSaveSlotMetadata &Right) {
  return Left.SlotName == Right.SlotName &&
         Left.SlotKind == Right.SlotKind &&
         Left.SavedAtUtc == Right.SavedAtUtc &&
         Left.MapName == Right.MapName &&
         Left.RunMeta.TotalPlayTimeSeconds == Right.RunMeta.TotalPlayTimeSeconds;
}
} // namespace

UProjectStartupMenuWidgetBase::UProjectStartupMenuWidgetBase(
    const FObjectInitializer &ObjectInitializer)
    : Super(ObjectInitializer) {
  SetIsFocusable(true);
}

TSharedRef<SWidget> UProjectStartupMenuWidgetBase::RebuildWidget() {
  if (WidgetTree && WidgetTree->RootWidget == nullptr) {
    BuildDefaultLayout();
  }

  return Super::RebuildWidget();
}

void UProjectStartupMenuWidgetBase::NativeConstruct() {
  Super::NativeConstruct();

  InitializeNamedWidgets();
  RefreshVisualState();
}

void UProjectStartupMenuWidgetBase::NativeDestruct() {
  CleanupButtonBindings();

  Super::NativeDestruct();
}

void UProjectStartupMenuWidgetBase::SetViewState(
    EProjectStartupMenuViewState NewState) {
  if (ViewState == NewState) {
    return;
  }

  ViewState = NewState;
  RefreshVisualState();
}

void UProjectStartupMenuWidgetBase::SetCanContinueGame(
    bool bInCanContinueGame) {
  if (bCanContinueGame == bInCanContinueGame) {
    return;
  }

  bCanContinueGame = bInCanContinueGame;
  RefreshVisualState();
}

void UProjectStartupMenuWidgetBase::SetCanOpenSaveSelection(
    bool bInCanOpenSaveSelection) {
  if (bCanOpenSaveSelection == bInCanOpenSaveSelection) {
    return;
  }

  bCanOpenSaveSelection = bInCanOpenSaveSelection;
  RefreshVisualState();
}

void UProjectStartupMenuWidgetBase::SetAvailableSaveSlots(
    const TArray<FProjectSaveSlotMetadata> &NewSaveSlots) {
  if (AreSaveSlotsEquivalent(NewSaveSlots)) {
    return;
  }

  AvailableSaveSlots = NewSaveSlots;
  RefreshSaveSlotList();
  RefreshVisualState();
}

void UProjectStartupMenuWidgetBase::SetLoadingState(const FText &InTitle,
                                                    const FText &InStatus,
                                                    float InProgress) {
  const float ClampedProgress = FMath::Clamp(InProgress, 0.0f, 1.0f);
  if (LoadingTitle.EqualTo(InTitle) && LoadingStatus.EqualTo(InStatus) &&
      FMath::IsNearlyEqual(LoadingProgress, ClampedProgress)) {
    return;
  }

  LoadingTitle = InTitle;
  LoadingStatus = InStatus;
  LoadingProgress = ClampedProgress;
  RefreshLoadingState();
  RefreshVisualState();
}

FText UProjectStartupMenuWidgetBase::GetLoadingPercentText() const {
  return FText::Format(
      LOCTEXT("LoadingPercentText", "{0}%"),
      FText::AsNumber(FMath::RoundToInt(LoadingProgress * 100.0f)));
}

void UProjectStartupMenuWidgetBase::RequestStartGame() {
  OnStartGameRequested.Broadcast();
}

void UProjectStartupMenuWidgetBase::RequestContinueGame() {
  OnContinueGameRequested.Broadcast();
}

void UProjectStartupMenuWidgetBase::RequestOpenSaveSelection() {
  OnOpenSaveSelectionRequested.Broadcast();
}

void UProjectStartupMenuWidgetBase::RequestBackToStartupMenu() {
  OnBackToStartupMenuRequested.Broadcast();
}

void UProjectStartupMenuWidgetBase::RequestLoadSaveSlot(
    const FString &SlotName) {
  if (!SlotName.IsEmpty()) {
    OnLoadSaveSlotRequested.Broadcast(SlotName);
  }
}

void UProjectStartupMenuWidgetBase::RequestOverwriteSaveSlot(
    const FString &SlotName) {
  if (!SlotName.IsEmpty()) {
    OnOverwriteSaveSlotRequested.Broadcast(SlotName);
  }
}

void UProjectStartupMenuWidgetBase::RequestDeleteSaveSlot(
    const FString &SlotName) {
  if (!SlotName.IsEmpty()) {
    OnDeleteSaveSlotRequested.Broadcast(SlotName);
  }
}

void UProjectStartupMenuWidgetBase::RequestExitGame() {
  OnExitGameRequested.Broadcast();
}

void UProjectStartupMenuWidgetBase::HandleStartButtonClicked() {
  RequestStartGame();
}

void UProjectStartupMenuWidgetBase::HandleContinueButtonClicked() {
  RequestContinueGame();
}

void UProjectStartupMenuWidgetBase::HandleLoadButtonClicked() {
  RequestOpenSaveSelection();
}

void UProjectStartupMenuWidgetBase::HandleBackButtonClicked() {
  RequestBackToStartupMenu();
}

void UProjectStartupMenuWidgetBase::HandleExitButtonClicked() {
  RequestExitGame();
}

void UProjectStartupMenuWidgetBase::BuildDefaultLayout() {
  if (!WidgetTree || WidgetTree->RootWidget) {
    return;
  }

  UOverlay *RootOverlay = WidgetTree->ConstructWidget<UOverlay>(
      UOverlay::StaticClass(), TEXT("Root"));
  WidgetTree->RootWidget = RootOverlay;

  auto BuildFullScreenPanel =
      [this, RootOverlay](const FName &PanelName, float WidthOverride,
                          const FLinearColor &BackdropColor,
                          const FLinearColor &PanelColor) -> UVerticalBox * {
    UBorder *Backdrop = WidgetTree->ConstructWidget<UBorder>(
        UBorder::StaticClass(), PanelName);
    Backdrop->SetPadding(FMargin(24.0f));
    Backdrop->SetBrushColor(BackdropColor);
    if (UOverlaySlot *BackdropSlot = RootOverlay->AddChildToOverlay(Backdrop)) {
      BackdropSlot->SetHorizontalAlignment(HAlign_Fill);
      BackdropSlot->SetVerticalAlignment(VAlign_Fill);
    }

    UOverlay *PanelOverlay = WidgetTree->ConstructWidget<UOverlay>(
        UOverlay::StaticClass(), NAME_None);
    Backdrop->SetContent(PanelOverlay);

    USizeBox *CenterBox =
        WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), NAME_None);
    CenterBox->SetWidthOverride(WidthOverride);
    if (UOverlaySlot *CenterSlot = PanelOverlay->AddChildToOverlay(CenterBox)) {
      CenterSlot->SetHorizontalAlignment(HAlign_Center);
      CenterSlot->SetVerticalAlignment(VAlign_Center);
    }

    UBorder *PanelBorder = WidgetTree->ConstructWidget<UBorder>(
        UBorder::StaticClass(), NAME_None);
    PanelBorder->SetPadding(FMargin(36.0f));
    PanelBorder->SetBrushColor(PanelColor);
    CenterBox->SetContent(PanelBorder);

    UVerticalBox *ContentBox = WidgetTree->ConstructWidget<UVerticalBox>(
        UVerticalBox::StaticClass(), NAME_None);
    PanelBorder->SetContent(ContentBox);
    return ContentBox;
  };

  UVerticalBox *StartupMenuBox =
      BuildFullScreenPanel(TEXT("Panel_StartupMenu"), 460.0f,
                           FLinearColor(0.01f, 0.02f, 0.03f, 0.96f),
                           FLinearColor(0.08f, 0.11f, 0.14f, 0.98f));
  UBorder *StartupAccent = WidgetTree->ConstructWidget<UBorder>(
      UBorder::StaticClass(), NAME_None);
  StartupAccent->SetBrushColor(FLinearColor(0.76f, 0.30f, 0.08f, 1.0f));
  USizeBox *StartupAccentSize =
      WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), NAME_None);
  StartupAccentSize->SetHeightOverride(4.0f);
  StartupAccent->SetContent(StartupAccentSize);
  if (UVerticalBoxSlot *AccentSlot =
          StartupMenuBox->AddChildToVerticalBox(StartupAccent)) {
    AccentSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 16.0f));
  }

  UTextBlock *MenuTitle = WidgetTree->ConstructWidget<UTextBlock>(
      UTextBlock::StaticClass(), TEXT("Text_MenuTitle"));
  MenuTitle->SetText(LOCTEXT("MenuTitle", "Project B"));
  ApplyTextStyle(MenuTitle, 34, FLinearColor::White, TEXT("Bold"));
  if (UVerticalBoxSlot *TitleSlot =
          StartupMenuBox->AddChildToVerticalBox(MenuTitle)) {
    TitleSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 10.0f));
  }

  UTextBlock *MenuSubtitle = WidgetTree->ConstructWidget<UTextBlock>(
      UTextBlock::StaticClass(), TEXT("Text_MenuSubtitle"));
  MenuSubtitle->SetText(LOCTEXT("MenuSubtitle", "Стартовое меню"));
  ApplyTextStyle(MenuSubtitle, 16, FLinearColor(0.72f, 0.77f, 0.82f, 1.0f),
                 TEXT("Regular"));
  if (UVerticalBoxSlot *SubtitleSlot =
          StartupMenuBox->AddChildToVerticalBox(MenuSubtitle)) {
    SubtitleSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 28.0f));
  }

  CreateMenuButton(WidgetTree, StartupMenuBox, TEXT("Button_StartGame"),
                   TEXT("Text_StartGame"), LOCTEXT("StartGameButton", "Начать игру"),
                   12.0f);
  CreateMenuButton(WidgetTree, StartupMenuBox, TEXT("Button_ContinueGame"),
                   TEXT("Text_ContinueGame"),
                   LOCTEXT("ContinueGameButton", "Продолжить игру"), 12.0f);
  CreateMenuButton(WidgetTree, StartupMenuBox, TEXT("Button_LoadGame"),
                   TEXT("Text_LoadGame"),
                   LOCTEXT("LoadGameButton", "Загрузить сохранение"), 12.0f);
  CreateMenuButton(WidgetTree, StartupMenuBox, TEXT("Button_Settings"),
                   TEXT("Text_Settings"), LOCTEXT("SettingsButton", "Настройки"),
                   18.0f);
  CreateMenuButton(WidgetTree, StartupMenuBox, TEXT("Button_ExitGame"),
                   TEXT("Text_ExitGame"), LOCTEXT("ExitGameButton", "Выход"),
                   0.0f);

  UVerticalBox *SaveSelectionBox =
      BuildFullScreenPanel(TEXT("Panel_SaveSelection"), 760.0f,
                           FLinearColor(0.01f, 0.02f, 0.03f, 0.96f),
                           FLinearColor(0.08f, 0.11f, 0.14f, 0.98f));

  UBorder *SaveAccent = WidgetTree->ConstructWidget<UBorder>(
      UBorder::StaticClass(), NAME_None);
  SaveAccent->SetBrushColor(FLinearColor(0.16f, 0.56f, 0.78f, 1.0f));
  USizeBox *SaveAccentSize =
      WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), NAME_None);
  SaveAccentSize->SetHeightOverride(4.0f);
  SaveAccent->SetContent(SaveAccentSize);
  if (UVerticalBoxSlot *AccentSlot =
          SaveSelectionBox->AddChildToVerticalBox(SaveAccent)) {
    AccentSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 16.0f));
  }

  UTextBlock *SaveTitle = WidgetTree->ConstructWidget<UTextBlock>(
      UTextBlock::StaticClass(), TEXT("Text_SaveSelectionTitle"));
  SaveTitle->SetText(LOCTEXT("SaveSelectionTitle", "Выбор сохранения"));
  ApplyTextStyle(SaveTitle, 30, FLinearColor::White, TEXT("Bold"));
  if (UVerticalBoxSlot *TitleSlot =
          SaveSelectionBox->AddChildToVerticalBox(SaveTitle)) {
    TitleSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 8.0f));
  }

  UTextBlock *SaveSubtitle = WidgetTree->ConstructWidget<UTextBlock>(
      UTextBlock::StaticClass(), TEXT("Text_SaveSelectionSubtitle"));
  SaveSubtitle->SetText(
      LOCTEXT("SaveSelectionSubtitle", "Выберите сохранение, которое хотите загрузить."));
  ApplyTextStyle(SaveSubtitle, 15,
                 FLinearColor(0.72f, 0.77f, 0.82f, 1.0f), TEXT("Regular"));
  if (UVerticalBoxSlot *SubtitleSlot =
          SaveSelectionBox->AddChildToVerticalBox(SaveSubtitle)) {
    SubtitleSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 16.0f));
  }

  UTextBlock *EmptySaveSlotsText = WidgetTree->ConstructWidget<UTextBlock>(
      UTextBlock::StaticClass(), TEXT("Text_EmptySaveSlots"));
  EmptySaveSlotsText->SetText(
      LOCTEXT("EmptySaveSlotsText", "Доступных сохранений пока нет."));
  ApplyTextStyle(EmptySaveSlotsText, 16,
                 FLinearColor(0.73f, 0.78f, 0.84f, 1.0f), TEXT("Regular"));
  if (UVerticalBoxSlot *EmptySlot =
          SaveSelectionBox->AddChildToVerticalBox(EmptySaveSlotsText)) {
    EmptySlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 12.0f));
  }

  UScrollBox *SaveScrollBox = WidgetTree->ConstructWidget<UScrollBox>(
      UScrollBox::StaticClass(), TEXT("ScrollBox_SaveSlots"));
  if (UVerticalBoxSlot *ScrollSlot =
          SaveSelectionBox->AddChildToVerticalBox(SaveScrollBox)) {
    ScrollSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
    ScrollSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 18.0f));
  }

  UVerticalBox *SaveSlotList = WidgetTree->ConstructWidget<UVerticalBox>(
      UVerticalBox::StaticClass(), TEXT("Panel_SaveSlots"));
  SaveScrollBox->AddChild(SaveSlotList);

  CreateMenuButton(WidgetTree, SaveSelectionBox, TEXT("Button_BackToMenu"),
                   TEXT("Text_BackToMenu"),
                   LOCTEXT("BackToMenuButton", "Назад"), 0.0f);

  UVerticalBox *LoadingBox =
      BuildFullScreenPanel(TEXT("Panel_Loading"), 520.0f,
                           FLinearColor(0.01f, 0.02f, 0.03f, 0.96f),
                           FLinearColor(0.05f, 0.08f, 0.12f, 0.98f));

  UTextBlock *LoadingTitleText = WidgetTree->ConstructWidget<UTextBlock>(
      UTextBlock::StaticClass(), TEXT("Text_LoadingTitle"));
  LoadingTitleText->SetText(LOCTEXT("DefaultLoadingTitle", "Загрузка"));
  ApplyTextStyle(LoadingTitleText, 28, FLinearColor::White, TEXT("Bold"),
                 ETextJustify::Center);
  if (UVerticalBoxSlot *TitleSlot =
          LoadingBox->AddChildToVerticalBox(LoadingTitleText)) {
    TitleSlot->SetHorizontalAlignment(HAlign_Center);
    TitleSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 18.0f));
  }

  UTextBlock *LoadingPercentText = WidgetTree->ConstructWidget<UTextBlock>(
      UTextBlock::StaticClass(), TEXT("Text_LoadingPercent"));
  LoadingPercentText->SetText(LOCTEXT("DefaultLoadingPercent", "0%"));
  ApplyTextStyle(LoadingPercentText, 42,
                 FLinearColor(0.97f, 0.98f, 1.0f, 1.0f), TEXT("Bold"),
                 ETextJustify::Center);
  if (UVerticalBoxSlot *PercentSlot =
          LoadingBox->AddChildToVerticalBox(LoadingPercentText)) {
    PercentSlot->SetHorizontalAlignment(HAlign_Center);
    PercentSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 18.0f));
  }

  UProgressBar *LoadingProgressBar = WidgetTree->ConstructWidget<UProgressBar>(
      UProgressBar::StaticClass(), TEXT("ProgressBar_Loading"));
  if (UVerticalBoxSlot *ProgressSlot =
          LoadingBox->AddChildToVerticalBox(LoadingProgressBar)) {
    ProgressSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 12.0f));
  }

  UTextBlock *LoadingStatusText = WidgetTree->ConstructWidget<UTextBlock>(
      UTextBlock::StaticClass(), TEXT("Text_LoadingStatus"));
  LoadingStatusText->SetText(
      LOCTEXT("DefaultLoadingStatus", "Подождите, идёт подготовка..."));
  ApplyTextStyle(LoadingStatusText, 15,
                 FLinearColor(0.72f, 0.77f, 0.82f, 1.0f), TEXT("Regular"),
                 ETextJustify::Center);
  LoadingBox->AddChildToVerticalBox(LoadingStatusText);
}

void UProjectStartupMenuWidgetBase::InitializeNamedWidgets() {
  CleanupButtonBindings();

  CachedStartupMenuPanel = GetWidgetFromName(TEXT("Panel_StartupMenu"));
  CachedSaveSelectionPanel = GetWidgetFromName(TEXT("Panel_SaveSelection"));
  CachedLoadingPanel = GetWidgetFromName(TEXT("Panel_Loading"));
  CachedStartButton = Cast<UButton>(GetWidgetFromName(TEXT("Button_StartGame")));
  CachedContinueButton =
      Cast<UButton>(GetWidgetFromName(TEXT("Button_ContinueGame")));
  CachedLoadButton = Cast<UButton>(GetWidgetFromName(TEXT("Button_LoadGame")));
  CachedSettingsButton = Cast<UButton>(GetWidgetFromName(TEXT("Button_Settings")));
  CachedExitButton = Cast<UButton>(GetWidgetFromName(TEXT("Button_ExitGame")));
  CachedBackButton = Cast<UButton>(GetWidgetFromName(TEXT("Button_BackToMenu")));
  CachedSaveSlotList = Cast<UPanelWidget>(GetWidgetFromName(TEXT("Panel_SaveSlots")));
  CachedEmptySaveSlotsText =
      Cast<UTextBlock>(GetWidgetFromName(TEXT("Text_EmptySaveSlots")));
  CachedLoadingTitleText =
      Cast<UTextBlock>(GetWidgetFromName(TEXT("Text_LoadingTitle")));
  CachedLoadingStatusText =
      Cast<UTextBlock>(GetWidgetFromName(TEXT("Text_LoadingStatus")));
  CachedLoadingPercentText =
      Cast<UTextBlock>(GetWidgetFromName(TEXT("Text_LoadingPercent")));
  CachedLoadingProgressBar =
      Cast<UProgressBar>(GetWidgetFromName(TEXT("ProgressBar_Loading")));

  if (CachedStartButton) {
    CachedStartButton->OnClicked.AddDynamic(
        this, &UProjectStartupMenuWidgetBase::HandleStartButtonClicked);
  }

  if (CachedContinueButton) {
    CachedContinueButton->OnClicked.AddDynamic(
        this, &UProjectStartupMenuWidgetBase::HandleContinueButtonClicked);
  }

  if (CachedLoadButton) {
    CachedLoadButton->OnClicked.AddDynamic(
        this, &UProjectStartupMenuWidgetBase::HandleLoadButtonClicked);
  }

  if (CachedBackButton) {
    CachedBackButton->OnClicked.AddDynamic(
        this, &UProjectStartupMenuWidgetBase::HandleBackButtonClicked);
  }

  if (CachedExitButton) {
    CachedExitButton->OnClicked.AddDynamic(
        this, &UProjectStartupMenuWidgetBase::HandleExitButtonClicked);
  }

  RefreshSaveSlotList();
  RefreshLoadingState();
}

void UProjectStartupMenuWidgetBase::RefreshVisualState() {
  SetVisibility(ViewState == EProjectStartupMenuViewState::Hidden
                    ? ESlateVisibility::Collapsed
                    : ESlateVisibility::Visible);

  if (CachedStartupMenuPanel) {
    CachedStartupMenuPanel->SetVisibility(
        ViewState == EProjectStartupMenuViewState::StartupMenu
            ? ESlateVisibility::Visible
            : ESlateVisibility::Collapsed);
  }

  if (CachedSaveSelectionPanel) {
    CachedSaveSelectionPanel->SetVisibility(
        ViewState == EProjectStartupMenuViewState::SaveSelection
            ? ESlateVisibility::Visible
            : ESlateVisibility::Collapsed);
  }

  if (CachedLoadingPanel) {
    CachedLoadingPanel->SetVisibility(
        ViewState == EProjectStartupMenuViewState::LoadingScreen
            ? ESlateVisibility::Visible
            : ESlateVisibility::Collapsed);
  }

  if (CachedContinueButton) {
    CachedContinueButton->SetIsEnabled(bCanContinueGame);
  }

  if (CachedLoadButton) {
    CachedLoadButton->SetIsEnabled(bCanOpenSaveSelection);
  }

  if (CachedSettingsButton) {
    CachedSettingsButton->SetIsEnabled(false);
  }

  if (CachedBackButton) {
    CachedBackButton->SetIsEnabled(true);
  }

  if (CachedEmptySaveSlotsText) {
    CachedEmptySaveSlotsText->SetVisibility(AvailableSaveSlots.IsEmpty()
                                               ? ESlateVisibility::Visible
                                               : ESlateVisibility::Collapsed);
  }

  RefreshLoadingState();
  BP_OnStartupMenuRefreshed();
}

void UProjectStartupMenuWidgetBase::RefreshSaveSlotList() {
  if (!CachedSaveSlotList) {
    return;
  }

  CachedSaveSlotList->ClearChildren();

  TSubclassOf<UProjectSaveSlotEntryWidgetBase> EntryWidgetClass =
      ResolveSaveSlotEntryWidgetClass();

  for (const FProjectSaveSlotMetadata &SaveSlot : AvailableSaveSlots) {
    UProjectSaveSlotEntryWidgetBase *EntryWidget = nullptr;
    if (APlayerController *OwningPlayer = GetOwningPlayer()) {
      EntryWidget = CreateWidget<UProjectSaveSlotEntryWidgetBase>(
          OwningPlayer, EntryWidgetClass);
    } else if (UGameInstance *GameInstance = GetGameInstance()) {
      EntryWidget = CreateWidget<UProjectSaveSlotEntryWidgetBase>(
          GameInstance, EntryWidgetClass);
    }

    if (!EntryWidget) {
      continue;
    }

    EntryWidget->SetSaveSlotMetadata(SaveSlot);
    EntryWidget->OnSaveSlotSelected.AddUObject(
        this, &UProjectStartupMenuWidgetBase::RequestLoadSaveSlot);
    EntryWidget->OnSaveSlotOverwriteRequested.AddUObject(
        this, &UProjectStartupMenuWidgetBase::RequestOverwriteSaveSlot);
    EntryWidget->OnSaveSlotDeleteRequested.AddUObject(
        this, &UProjectStartupMenuWidgetBase::RequestDeleteSaveSlot);

    if (UPanelSlot *PanelSlot = CachedSaveSlotList->AddChild(EntryWidget)) {
      if (UVerticalBoxSlot *VerticalBoxSlot = Cast<UVerticalBoxSlot>(PanelSlot)) {
        VerticalBoxSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 12.0f));
      } else if (UScrollBoxSlot *ScrollBoxSlot = Cast<UScrollBoxSlot>(PanelSlot)) {
        ScrollBoxSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 12.0f));
      }
    }
  }

  if (CachedEmptySaveSlotsText) {
    CachedEmptySaveSlotsText->SetVisibility(AvailableSaveSlots.IsEmpty()
                                               ? ESlateVisibility::Visible
                                               : ESlateVisibility::Collapsed);
  }
}

void UProjectStartupMenuWidgetBase::RefreshLoadingState() {
  if (CachedLoadingTitleText) {
    CachedLoadingTitleText->SetText(
        LoadingTitle.IsEmpty() ? LOCTEXT("FallbackLoadingTitle", "Загрузка")
                               : LoadingTitle);
  }

  if (CachedLoadingStatusText) {
    CachedLoadingStatusText->SetText(
        LoadingStatus.IsEmpty()
            ? LOCTEXT("FallbackLoadingStatus", "Подождите, идёт подготовка...")
            : LoadingStatus);
  }

  if (CachedLoadingPercentText) {
    CachedLoadingPercentText->SetText(GetLoadingPercentText());
  }

  if (CachedLoadingProgressBar) {
    CachedLoadingProgressBar->SetPercent(LoadingProgress);
  }
}

void UProjectStartupMenuWidgetBase::CleanupButtonBindings() {
  if (CachedStartButton) {
    CachedStartButton->OnClicked.RemoveDynamic(
        this, &UProjectStartupMenuWidgetBase::HandleStartButtonClicked);
  }

  if (CachedContinueButton) {
    CachedContinueButton->OnClicked.RemoveDynamic(
        this, &UProjectStartupMenuWidgetBase::HandleContinueButtonClicked);
  }

  if (CachedLoadButton) {
    CachedLoadButton->OnClicked.RemoveDynamic(
        this, &UProjectStartupMenuWidgetBase::HandleLoadButtonClicked);
  }

  if (CachedBackButton) {
    CachedBackButton->OnClicked.RemoveDynamic(
        this, &UProjectStartupMenuWidgetBase::HandleBackButtonClicked);
  }

  if (CachedExitButton) {
    CachedExitButton->OnClicked.RemoveDynamic(
        this, &UProjectStartupMenuWidgetBase::HandleExitButtonClicked);
  }
}

bool UProjectStartupMenuWidgetBase::AreSaveSlotsEquivalent(
    const TArray<FProjectSaveSlotMetadata> &OtherSaveSlots) const {
  if (AvailableSaveSlots.Num() != OtherSaveSlots.Num()) {
    return false;
  }

  for (int32 Index = 0; Index < AvailableSaveSlots.Num(); ++Index) {
    if (!AreSaveSlotsEqual(AvailableSaveSlots[Index], OtherSaveSlots[Index])) {
      return false;
    }
  }

  return true;
}

TSubclassOf<UProjectSaveSlotEntryWidgetBase>
UProjectStartupMenuWidgetBase::ResolveSaveSlotEntryWidgetClass() const {
  if (SaveSlotEntryWidgetClass) {
    return SaveSlotEntryWidgetClass;
  }

  return TSubclassOf<UProjectSaveSlotEntryWidgetBase>(
      UProjectSaveSlotEntryWidgetBase::StaticClass());
}

#undef LOCTEXT_NAMESPACE
