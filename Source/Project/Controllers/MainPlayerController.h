// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "MainPlayerController.generated.h"

class AWeaponShopTerminal;
class UCrosshairWidgetBase;
class UInputMappingContext;
class UPlayerArmoryComponent;
class UPlayerInventoryWidgetBase;
class UUserWidget;
class UWeaponShopWidgetBase;
struct FInputActionValue;

UCLASS(abstract)
class AMainPlayerController : public APlayerController {
  GENERATED_BODY()

public:
  AMainPlayerController();

  UFUNCTION(BlueprintPure, Category = "Gameplay|Armory")
  UPlayerArmoryComponent *GetPlayerArmoryComponent() const {
    return PlayerArmoryComponent;
  }

  UFUNCTION(BlueprintCallable, Category = "UI|Armory")
  void OpenWeaponShop(AWeaponShopTerminal *ShopTerminal);

  UFUNCTION(BlueprintCallable, Category = "UI|Armory")
  void CloseWeaponShop();

  UFUNCTION(BlueprintCallable, Category = "UI|Armory")
  void OpenInventory();

  UFUNCTION(BlueprintCallable, Category = "UI|Armory")
  void CloseInventory();

  UFUNCTION(BlueprintCallable, Category = "UI|Armory")
  void ToggleInventory();

  UFUNCTION(BlueprintPure, Category = "UI|Armory")
  bool IsAnyArmoryOverlayOpen() const;

  UFUNCTION(BlueprintCallable, Category = "UI|Armory")
  void SetExternalArmoryOverlayOpen(bool bIsOpen);

  UFUNCTION(BlueprintCallable, Category = "Save")
  bool RequestQuickSave();

  UFUNCTION(BlueprintCallable, Category = "Save")
  bool RequestQuickLoad();

  UFUNCTION(BlueprintCallable, Category = "Save")
  bool RequestManualSave();

  UFUNCTION(BlueprintCallable, Category = "Save")
  bool RequestManualLoad();

  UFUNCTION(BlueprintCallable, Category = "Save")
  bool RequestOverwriteSaveSlot(const FString &SlotName);

  UFUNCTION(BlueprintCallable, Category = "Save")
  bool RequestDeleteSaveSlot(const FString &SlotName);

protected:
  UPROPERTY(EditAnywhere, Category = "Input|Input Mappings")
  TArray<UInputMappingContext *> DefaultMappingContexts;

  UPROPERTY(EditAnywhere, Category = "Input|Input Mappings")
  TArray<UInputMappingContext *> MobileExcludedMappingContexts;

  UPROPERTY(EditAnywhere, Category = "Input|Touch Controls")
  TSubclassOf<UUserWidget> MobileControlsWidgetClass;

  UPROPERTY()
  TObjectPtr<UUserWidget> MobileControlsWidget;

  UPROPERTY(EditAnywhere, Config, Category = "Input|Touch Controls")
  bool bForceTouchControls = false;

  UPROPERTY(EditAnywhere, Category = "UI|HUD")
  TSubclassOf<UCrosshairWidgetBase> HUDWidgetClass;

  UPROPERTY(EditAnywhere, Category = "UI|HUD")
  int32 HUDWidgetZOrder = 0;

  UPROPERTY(Transient, BlueprintReadOnly, Category = "UI|HUD",
            meta = (AllowPrivateAccess = "true"))
  TObjectPtr<UCrosshairWidgetBase> HUDWidget;

  UPROPERTY(EditAnywhere, Category = "UI|Armory")
  TSubclassOf<UWeaponShopWidgetBase> WeaponShopWidgetClass;

  UPROPERTY(EditAnywhere, Category = "UI|Armory")
  TSubclassOf<UPlayerInventoryWidgetBase> InventoryWidgetClass;

  UPROPERTY(EditAnywhere, Category = "UI|Armory")
  int32 ArmoryWidgetZOrder = 10;

  UPROPERTY(Transient)
  TObjectPtr<UWeaponShopWidgetBase> WeaponShopWidget;

  UPROPERTY(Transient)
  TObjectPtr<UPlayerInventoryWidgetBase> InventoryWidget;

  UPROPERTY(BlueprintReadOnly, Category = "UI")
  bool bIsInteractingWithUI = false;

  UPROPERTY(Transient, BlueprintReadOnly, Category = "UI|Armory")
  bool bHasExternalArmoryOverlayOpen = false;

  UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Gameplay|Armory",
            meta = (AllowPrivateAccess = "true"))
  TObjectPtr<UPlayerArmoryComponent> PlayerArmoryComponent;

  UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Gameplay|Startup")
  bool bApplyStartupPawnStateOnPossess = true;

  UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Gameplay|Startup")
  bool bApplyStartupPawnStateOnlyOnce = true;

  UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Gameplay|Startup")
  bool bClearStartingLoadoutOnPossess = true;

  UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Gameplay|Startup")
  bool bSetStartingCurrencyOnPossess = true;

  UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Gameplay|Startup",
            meta = (ClampMin = "0", EditCondition = "bSetStartingCurrencyOnPossess"))
  int32 StartingCurrency = 500;

  UPROPERTY(Transient)
  bool bHasAppliedStartupPawnState = false;

  UPROPERTY(Transient)
  bool bSkipNextStartupPawnStateApplication = false;

  virtual void BeginPlay() override;
  virtual void SetupInputComponent() override;
  virtual void OnPossess(APawn *InPawn) override;

  bool ShouldUseTouchControls() const;
  void BindInputActions();
  void ApplyStartupPawnState(APawn *InPawn);
  void ConsumeSaveRestoreStartupSkip();
  void ApplyInventoryWidgetLayoutDefaults();
  void UpdateArmoryOverlayInputState();
  void CloseAllArmoryOverlays();

  UFUNCTION()
  void HandleMove(const FInputActionValue &Value);

  UFUNCTION()
  void HandleLook(const FInputActionValue &Value);

  UFUNCTION()
  void HandleJumpStarted(const FInputActionValue &Value);

  UFUNCTION()
  void HandleJumpCompleted(const FInputActionValue &Value);

  UFUNCTION()
  void HandleSprintStarted(const FInputActionValue &Value);

  UFUNCTION()
  void HandleSprintCompleted(const FInputActionValue &Value);

  UFUNCTION()
  void HandleInteract(const FInputActionValue &Value);

  UFUNCTION()
  void HandleFireStarted(const FInputActionValue &Value);

  UFUNCTION()
  void HandleFireCompleted(const FInputActionValue &Value);

  UFUNCTION()
  void HandleReload(const FInputActionValue &Value);

  UFUNCTION()
  void HandleScopeStarted(const FInputActionValue &Value);

  UFUNCTION()
  void HandleScopeCompleted(const FInputActionValue &Value);

  UFUNCTION()
  void HandleWeaponCycle(const FInputActionValue &Value);

  UFUNCTION()
  void HandleInventoryToggle(const FInputActionValue &Value);

  UFUNCTION()
  void HandleQuickSave(const FInputActionValue &Value);

  UFUNCTION()
  void HandleQuickLoad(const FInputActionValue &Value);

  UFUNCTION()
  void HandleMenuToggle(const FInputActionValue &Value);

  void HandleCloseOverlayKey();

  UPROPERTY(EditAnywhere, Category = "Input|Combat", meta = (ClampMin = "0.0"))
  float WeaponCycleInputCooldown = 0.12f;

  UPROPERTY(Transient)
  float LastWeaponCycleInputTimeSeconds = -1000.0f;

public:
  UFUNCTION(BlueprintCallable, Category = "Gameplay|Startup")
  void SetApplyStartupPawnStateOnPossess(bool bShouldApply);

  UFUNCTION(BlueprintCallable, Category = "Gameplay|Startup")
  void ResetStartupPawnStateTracking();

  UFUNCTION(BlueprintCallable, Category = "Gameplay|Startup")
  void MarkStartupStateRestoredFromSave();

  UFUNCTION(BlueprintCallable, Category = "UI")
  void SetMouseCursorVisible(bool bVisible);
};
