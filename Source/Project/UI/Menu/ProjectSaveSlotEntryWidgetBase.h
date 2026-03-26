#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Save/ProjectSaveIndex.h"
#include "ProjectSaveSlotEntryWidgetBase.generated.h"

class UButton;
class UTextBlock;

DECLARE_MULTICAST_DELEGATE_OneParam(FProjectSaveSlotSelectedDelegate,
                                    const FString &);
DECLARE_MULTICAST_DELEGATE_OneParam(FProjectSaveSlotOverwriteDelegate,
                                    const FString &);
DECLARE_MULTICAST_DELEGATE_OneParam(FProjectSaveSlotDeleteDelegate,
                                    const FString &);

UCLASS(BlueprintType, Blueprintable)
class PROJECT_API UProjectSaveSlotEntryWidgetBase : public UUserWidget {
  GENERATED_BODY()

public:
  UProjectSaveSlotEntryWidgetBase(const FObjectInitializer &ObjectInitializer);

  virtual TSharedRef<SWidget> RebuildWidget() override;
  virtual void NativeConstruct() override;
  virtual void NativeDestruct() override;

  void SetSaveSlotMetadata(const FProjectSaveSlotMetadata &InSaveSlotMetadata);

  UFUNCTION(BlueprintPure, Category = "Startup Menu")
  FProjectSaveSlotMetadata GetSaveSlotMetadata() const {
    return SaveSlotMetadata;
  }

  UFUNCTION(BlueprintPure, Category = "Startup Menu")
  FText GetPrimaryText() const;

  UFUNCTION(BlueprintPure, Category = "Startup Menu")
  FText GetMapText() const;

  UFUNCTION(BlueprintPure, Category = "Startup Menu")
  FText GetMetaText() const;

  UFUNCTION(BlueprintCallable, Category = "Startup Menu")
  void RequestSelectSlot();

  UFUNCTION(BlueprintCallable, Category = "Startup Menu")
  void RequestOverwriteSlot();

  UFUNCTION(BlueprintCallable, Category = "Startup Menu")
  void RequestDeleteSlot();

  UFUNCTION(BlueprintImplementableEvent, Category = "Startup Menu")
  void BP_OnSaveSlotEntryRefreshed();

  FProjectSaveSlotSelectedDelegate OnSaveSlotSelected;
  FProjectSaveSlotOverwriteDelegate OnSaveSlotOverwriteRequested;
  FProjectSaveSlotDeleteDelegate OnSaveSlotDeleteRequested;

protected:
  UPROPERTY(BlueprintReadOnly, Category = "Startup Menu")
  FProjectSaveSlotMetadata SaveSlotMetadata;

private:
  UFUNCTION()
  void HandleSelectButtonClicked();

  UFUNCTION()
  void HandleOverwriteButtonClicked();

  UFUNCTION()
  void HandleDeleteButtonClicked();

  void BuildDefaultLayout();
  void InitializeNamedWidgets();
  void RefreshVisualState();

  UPROPERTY(Transient)
  TObjectPtr<UButton> CachedSelectButton;

  UPROPERTY(Transient)
  TObjectPtr<UTextBlock> CachedPrimaryText;

  UPROPERTY(Transient)
  TObjectPtr<UTextBlock> CachedMapText;

  UPROPERTY(Transient)
  TObjectPtr<UTextBlock> CachedMetaText;

  UPROPERTY(Transient)
  TObjectPtr<UButton> CachedOverwriteButton;

  UPROPERTY(Transient)
  TObjectPtr<UButton> CachedDeleteButton;
};
