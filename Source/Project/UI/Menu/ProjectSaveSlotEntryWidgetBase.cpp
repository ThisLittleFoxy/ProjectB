#include "UI/Menu/ProjectSaveSlotEntryWidgetBase.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"

#define LOCTEXT_NAMESPACE "ProjectSaveSlotEntryWidgetBase"

namespace {
FText GetSaveSlotKindText(EProjectSaveSlotKind SlotKind) {
  return SlotKind == EProjectSaveSlotKind::Manual
             ? LOCTEXT("SaveSlotKindManual", "Ручное сохранение")
             : LOCTEXT("SaveSlotKindQuick", "Быстрое сохранение");
}

FText GetSaveSlotTimestampText(const FProjectSaveSlotMetadata &Metadata) {
  return FText::FromString(Metadata.SavedAtUtc.ToString(TEXT("%d.%m.%Y %H:%M:%S UTC")));
}

FText GetSaveSlotPlayTimeText(const FProjectSaveSlotMetadata &Metadata) {
  const int64 TotalSeconds = FMath::Max<int64>(0, Metadata.RunMeta.TotalPlayTimeSeconds);
  const int32 Hours = static_cast<int32>(TotalSeconds / 3600);
  const int32 Minutes = static_cast<int32>((TotalSeconds % 3600) / 60);
  const int32 Seconds = static_cast<int32>(TotalSeconds % 60);

  return FText::FromString(
      FString::Printf(TEXT("%02d:%02d:%02d"), Hours, Minutes, Seconds));
}

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
} // namespace

UProjectSaveSlotEntryWidgetBase::UProjectSaveSlotEntryWidgetBase(
    const FObjectInitializer &ObjectInitializer)
    : Super(ObjectInitializer) {
  SetIsFocusable(false);
}

TSharedRef<SWidget> UProjectSaveSlotEntryWidgetBase::RebuildWidget() {
  if (WidgetTree && WidgetTree->RootWidget == nullptr) {
    BuildDefaultLayout();
  }

  return Super::RebuildWidget();
}

void UProjectSaveSlotEntryWidgetBase::NativeConstruct() {
  Super::NativeConstruct();

  InitializeNamedWidgets();
  RefreshVisualState();
}

void UProjectSaveSlotEntryWidgetBase::NativeDestruct() {
  if (CachedSelectButton) {
    CachedSelectButton->OnClicked.RemoveDynamic(
        this, &UProjectSaveSlotEntryWidgetBase::HandleSelectButtonClicked);
  }

  if (CachedOverwriteButton) {
    CachedOverwriteButton->OnClicked.RemoveDynamic(
        this, &UProjectSaveSlotEntryWidgetBase::HandleOverwriteButtonClicked);
  }

  if (CachedDeleteButton) {
    CachedDeleteButton->OnClicked.RemoveDynamic(
        this, &UProjectSaveSlotEntryWidgetBase::HandleDeleteButtonClicked);
  }

  Super::NativeDestruct();
}

void UProjectSaveSlotEntryWidgetBase::SetSaveSlotMetadata(
    const FProjectSaveSlotMetadata &InSaveSlotMetadata) {
  SaveSlotMetadata = InSaveSlotMetadata;
  RefreshVisualState();
}

FText UProjectSaveSlotEntryWidgetBase::GetPrimaryText() const {
  return FText::Format(LOCTEXT("SaveSlotPrimaryText", "{0} | {1}"),
                       GetSaveSlotKindText(SaveSlotMetadata.SlotKind),
                       GetSaveSlotTimestampText(SaveSlotMetadata));
}

FText UProjectSaveSlotEntryWidgetBase::GetMapText() const {
  return FText::Format(LOCTEXT("SaveSlotMapText", "Карта: {0}"),
                       FText::FromString(SaveSlotMetadata.MapName));
}

FText UProjectSaveSlotEntryWidgetBase::GetMetaText() const {
  return FText::Format(LOCTEXT("SaveSlotMetaText", "Слот: {0} | Наиграно: {1}"),
                       FText::FromString(SaveSlotMetadata.SlotName),
                       GetSaveSlotPlayTimeText(SaveSlotMetadata));
}

void UProjectSaveSlotEntryWidgetBase::RequestSelectSlot() {
  if (!SaveSlotMetadata.SlotName.IsEmpty()) {
    OnSaveSlotSelected.Broadcast(SaveSlotMetadata.SlotName);
  }
}

void UProjectSaveSlotEntryWidgetBase::RequestOverwriteSlot() {
  if (!SaveSlotMetadata.SlotName.IsEmpty()) {
    OnSaveSlotOverwriteRequested.Broadcast(SaveSlotMetadata.SlotName);
  }
}

void UProjectSaveSlotEntryWidgetBase::RequestDeleteSlot() {
  if (!SaveSlotMetadata.SlotName.IsEmpty()) {
    OnSaveSlotDeleteRequested.Broadcast(SaveSlotMetadata.SlotName);
  }
}

void UProjectSaveSlotEntryWidgetBase::HandleSelectButtonClicked() {
  RequestSelectSlot();
}

void UProjectSaveSlotEntryWidgetBase::HandleOverwriteButtonClicked() {
  RequestOverwriteSlot();
}

void UProjectSaveSlotEntryWidgetBase::HandleDeleteButtonClicked() {
  RequestDeleteSlot();
}

void UProjectSaveSlotEntryWidgetBase::BuildDefaultLayout() {
  if (!WidgetTree || WidgetTree->RootWidget) {
    return;
  }

  UBorder *RootBorder =
      WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("Root"));
  RootBorder->SetPadding(FMargin(10.0f));
  RootBorder->SetBrushColor(FLinearColor(0.09f, 0.12f, 0.15f, 0.96f));
  WidgetTree->RootWidget = RootBorder;

  UVerticalBox *ContentBox = WidgetTree->ConstructWidget<UVerticalBox>(
      UVerticalBox::StaticClass(), TEXT("VBox_Content"));
  RootBorder->SetContent(ContentBox);

  UButton *SelectButton = WidgetTree->ConstructWidget<UButton>(
      UButton::StaticClass(), TEXT("Button_Select"));
  if (UVerticalBoxSlot *SelectSlot = ContentBox->AddChildToVerticalBox(SelectButton)) {
    SelectSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 10.0f));
  }

  UBorder *ButtonBorder = WidgetTree->ConstructWidget<UBorder>(
      UBorder::StaticClass(), TEXT("Border_Content"));
  ButtonBorder->SetPadding(FMargin(16.0f));
  ButtonBorder->SetBrushColor(FLinearColor(0.13f, 0.16f, 0.20f, 1.0f));
  SelectButton->SetContent(ButtonBorder);

  UVerticalBox *InfoBox = WidgetTree->ConstructWidget<UVerticalBox>(
      UVerticalBox::StaticClass(), TEXT("VBox_Info"));
  ButtonBorder->SetContent(InfoBox);

  UTextBlock *PrimaryText = WidgetTree->ConstructWidget<UTextBlock>(
      UTextBlock::StaticClass(), TEXT("Text_Primary"));
  ApplyTextStyle(PrimaryText, 16, FLinearColor::White, TEXT("Bold"));
  if (UVerticalBoxSlot *PrimarySlot =
          InfoBox->AddChildToVerticalBox(PrimaryText)) {
    PrimarySlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 6.0f));
  }

  UTextBlock *MapText = WidgetTree->ConstructWidget<UTextBlock>(
      UTextBlock::StaticClass(), TEXT("Text_Map"));
  ApplyTextStyle(MapText, 14, FLinearColor(0.80f, 0.84f, 0.89f, 1.0f),
                 TEXT("Regular"));
  if (UVerticalBoxSlot *MapSlot = InfoBox->AddChildToVerticalBox(MapText)) {
    MapSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 4.0f));
  }

  UTextBlock *MetaText = WidgetTree->ConstructWidget<UTextBlock>(
      UTextBlock::StaticClass(), TEXT("Text_Meta"));
  ApplyTextStyle(MetaText, 13, FLinearColor(0.67f, 0.72f, 0.78f, 1.0f),
                 TEXT("Regular"));
  InfoBox->AddChildToVerticalBox(MetaText);

  UHorizontalBox *ActionsBox = WidgetTree->ConstructWidget<UHorizontalBox>(
      UHorizontalBox::StaticClass(), TEXT("HBox_Actions"));
  ContentBox->AddChildToVerticalBox(ActionsBox);

  UButton *OverwriteButton = WidgetTree->ConstructWidget<UButton>(
      UButton::StaticClass(), TEXT("Button_Overwrite"));
  if (UHorizontalBoxSlot *OverwriteSlot =
          ActionsBox->AddChildToHorizontalBox(OverwriteButton)) {
    OverwriteSlot->SetPadding(FMargin(0.0f, 0.0f, 8.0f, 0.0f));
    OverwriteSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
  }

  UTextBlock *OverwriteText = WidgetTree->ConstructWidget<UTextBlock>(
      UTextBlock::StaticClass(), TEXT("Text_Overwrite"));
  OverwriteText->SetText(LOCTEXT("OverwriteButtonText", "Перезаписать"));
  ApplyTextStyle(OverwriteText, 13, FLinearColor::White, TEXT("Regular"),
                 ETextJustify::Center);
  OverwriteButton->SetContent(OverwriteText);

  UButton *DeleteButton = WidgetTree->ConstructWidget<UButton>(
      UButton::StaticClass(), TEXT("Button_Delete"));
  if (UHorizontalBoxSlot *DeleteSlot =
          ActionsBox->AddChildToHorizontalBox(DeleteButton)) {
    DeleteSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
  }

  UTextBlock *DeleteText = WidgetTree->ConstructWidget<UTextBlock>(
      UTextBlock::StaticClass(), TEXT("Text_Delete"));
  DeleteText->SetText(LOCTEXT("DeleteButtonText", "Удалить"));
  ApplyTextStyle(DeleteText, 13, FLinearColor::White, TEXT("Regular"),
                 ETextJustify::Center);
  DeleteButton->SetContent(DeleteText);
}

void UProjectSaveSlotEntryWidgetBase::InitializeNamedWidgets() {
  CachedSelectButton = Cast<UButton>(GetWidgetFromName(TEXT("Button_Select")));
  CachedPrimaryText = Cast<UTextBlock>(GetWidgetFromName(TEXT("Text_Primary")));
  CachedMapText = Cast<UTextBlock>(GetWidgetFromName(TEXT("Text_Map")));
  CachedMetaText = Cast<UTextBlock>(GetWidgetFromName(TEXT("Text_Meta")));
  CachedOverwriteButton = Cast<UButton>(GetWidgetFromName(TEXT("Button_Overwrite")));
  CachedDeleteButton = Cast<UButton>(GetWidgetFromName(TEXT("Button_Delete")));

  if (CachedSelectButton &&
      !CachedSelectButton->OnClicked.IsAlreadyBound(
          this, &UProjectSaveSlotEntryWidgetBase::HandleSelectButtonClicked)) {
    CachedSelectButton->OnClicked.AddDynamic(
        this, &UProjectSaveSlotEntryWidgetBase::HandleSelectButtonClicked);
  }

  if (CachedOverwriteButton &&
      !CachedOverwriteButton->OnClicked.IsAlreadyBound(
          this, &UProjectSaveSlotEntryWidgetBase::HandleOverwriteButtonClicked)) {
    CachedOverwriteButton->OnClicked.AddDynamic(
        this, &UProjectSaveSlotEntryWidgetBase::HandleOverwriteButtonClicked);
  }

  if (CachedDeleteButton &&
      !CachedDeleteButton->OnClicked.IsAlreadyBound(
          this, &UProjectSaveSlotEntryWidgetBase::HandleDeleteButtonClicked)) {
    CachedDeleteButton->OnClicked.AddDynamic(
        this, &UProjectSaveSlotEntryWidgetBase::HandleDeleteButtonClicked);
  }
}

void UProjectSaveSlotEntryWidgetBase::RefreshVisualState() {
  if (CachedPrimaryText) {
    CachedPrimaryText->SetText(GetPrimaryText());
  }

  if (CachedMapText) {
    CachedMapText->SetText(GetMapText());
  }

  if (CachedMetaText) {
    CachedMetaText->SetText(GetMetaText());
  }

  if (CachedSelectButton) {
    CachedSelectButton->SetIsEnabled(!SaveSlotMetadata.SlotName.IsEmpty());
  }

  if (CachedOverwriteButton) {
    CachedOverwriteButton->SetIsEnabled(!SaveSlotMetadata.SlotName.IsEmpty());
  }

  if (CachedDeleteButton) {
    CachedDeleteButton->SetIsEnabled(!SaveSlotMetadata.SlotName.IsEmpty());
  }

  BP_OnSaveSlotEntryRefreshed();
}

#undef LOCTEXT_NAMESPACE
