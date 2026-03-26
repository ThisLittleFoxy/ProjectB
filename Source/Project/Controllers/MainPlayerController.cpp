// Copyright Epic Games, Inc. All Rights Reserved.

#include "Controllers/MainPlayerController.h"
#include "Blueprint/UserWidget.h"
#include "Character/ControllableCharacterInterface.h"
#include "Character/CurrencyComponent.h"
#include "Combat/CombatComponent.h"
#include "Engine/LocalPlayer.h"
#include "Engine/World.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "InputAction.h"
#include "InputCoreTypes.h"
#include "InputMappingContext.h"
#include "Interaction/InteractionComponent.h"
#include "Interaction/WeaponShopTerminal.h"
#include "Inventory/PlayerArmoryComponent.h"
#include "ProjectGameViewportClient.h"
#include "Project.h"
#include "Save/ProjectSaveSubsystem.h"
#include "TimerManager.h"
#include "UI/Armory/PlayerInventoryWidgetBase.h"
#include "UI/Armory/WeaponShopWidgetBase.h"
#include "UI/HUD/CrosshairWidgetBase.h"
#include "Utils/ProjectCheatManager.h"
#include "Widgets/Input/SVirtualJoystick.h"

AMainPlayerController::AMainPlayerController() {
  bShowMouseCursor = false;
  bEnableClickEvents = true;
  bEnableTouchEvents = true;
  bEnableMouseOverEvents = false;
  bEnableTouchOverEvents = false;

  CheatClass = UProjectCheatManager::StaticClass();
  PlayerArmoryComponent =
      CreateDefaultSubobject<UPlayerArmoryComponent>(TEXT("PlayerArmoryComponent"));
}

void AMainPlayerController::BeginPlay() {
  Super::BeginPlay();

  ApplyInventoryWidgetLayoutDefaults();

  UE_LOG(LogProject, Log,
         TEXT("MainPlayerController::BeginPlay - Controller: %s"), *GetName());
  UE_LOG(LogProject, Log, TEXT("IsLocalPlayerController: %s"),
         IsLocalPlayerController() ? TEXT("true") : TEXT("false"));
  UE_LOG(LogProject, Log, TEXT("ShouldUseTouchControls: %s"),
         ShouldUseTouchControls() ? TEXT("true") : TEXT("false"));

  if (IsLocalPlayerController() && HUDWidgetClass) {
    HUDWidget = CreateWidget<UCrosshairWidgetBase>(this, HUDWidgetClass);
    if (HUDWidget) {
      HUDWidget->AddToPlayerScreen(HUDWidgetZOrder);
      UE_LOG(LogProject, Log, TEXT("HUD widget created: %s"),
             *GetNameSafe(HUDWidget));
    } else {
      UE_LOG(LogProject, Error, TEXT("Could not spawn HUD widget."));
    }
  }

  if (ShouldUseTouchControls() && IsLocalPlayerController()) {
    MobileControlsWidget = CreateWidget<UUserWidget>(this, MobileControlsWidgetClass);
    if (MobileControlsWidget) {
      MobileControlsWidget->AddToPlayerScreen(0);
      UE_LOG(LogProject, Log,
             TEXT("Mobile controls widget created and added to screen"));
    } else {
      UE_LOG(LogProject, Error,
             TEXT("Could not spawn mobile controls widget."));
    }
  }
}

void AMainPlayerController::SetupInputComponent() {
  Super::SetupInputComponent();

  UE_LOG(LogProject, Log,
         TEXT("MainPlayerController::SetupInputComponent - Controller: %s"),
         *GetName());
  UE_LOG(LogProject, Log, TEXT("IsLocalPlayerController: %s"),
         IsLocalPlayerController() ? TEXT("true") : TEXT("false"));
  UE_LOG(LogProject, Log, TEXT("InputComponent valid: %s"),
         InputComponent ? TEXT("true") : TEXT("false"));

  if (IsLocalPlayerController()) {
    if (UEnhancedInputLocalPlayerSubsystem *Subsystem =
            ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(
                GetLocalPlayer())) {
      UE_LOG(LogProject, Log, TEXT("Enhanced Input Subsystem found"));

      for (int32 Index = 0; Index < DefaultMappingContexts.Num(); ++Index) {
        UInputMappingContext *CurrentContext = DefaultMappingContexts[Index];
        if (CurrentContext) {
          Subsystem->AddMappingContext(CurrentContext, 0);
        }
      }

      if (!ShouldUseTouchControls()) {
        for (UInputMappingContext *CurrentContext : MobileExcludedMappingContexts) {
          if (CurrentContext) {
            Subsystem->AddMappingContext(CurrentContext, 0);
          }
        }
      }
    } else {
      UE_LOG(LogProject, Error,
             TEXT("Failed to get Enhanced Input Subsystem!"));
    }

    BindInputActions();
  } else {
    UE_LOG(LogProject, Warning,
           TEXT("Not a local player controller, skipping input setup"));
  }
}

void AMainPlayerController::BindInputActions() {
  UEnhancedInputComponent *EnhancedInputComponent =
      Cast<UEnhancedInputComponent>(InputComponent);
  if (!EnhancedInputComponent) {
    UE_LOG(LogProject, Error,
           TEXT("Failed to cast InputComponent to EnhancedInputComponent!"));
    return;
  }

  TArray<UInputMappingContext *> AllContexts;
  AllContexts.Append(DefaultMappingContexts);
  if (!ShouldUseTouchControls()) {
    AllContexts.Append(MobileExcludedMappingContexts);
  }

  TSet<const UInputAction *> BoundActions;
  bool bBoundMenuToggleAction = false;
  for (UInputMappingContext *Context : AllContexts) {
    if (!Context) {
      continue;
    }

    const TArray<FEnhancedActionKeyMapping> &Mappings = Context->GetMappings();
    for (const FEnhancedActionKeyMapping &Mapping : Mappings) {
      const UInputAction *Action = Mapping.Action;
      if (!Action || BoundActions.Contains(Action)) {
        continue;
      }

      const FString ActionName = Action->GetName();
      if (ActionName.Contains(TEXT("Move")) ||
          ActionName.Contains(TEXT("IA_Move"))) {
        EnhancedInputComponent->BindAction(Action, ETriggerEvent::Triggered, this,
                                           &AMainPlayerController::HandleMove);
      } else if (ActionName.Contains(TEXT("Look")) ||
                 ActionName.Contains(TEXT("IA_Look"))) {
        EnhancedInputComponent->BindAction(Action, ETriggerEvent::Triggered, this,
                                           &AMainPlayerController::HandleLook);
      } else if (ActionName.Contains(TEXT("Jump")) ||
                 ActionName.Contains(TEXT("IA_Jump"))) {
        EnhancedInputComponent->BindAction(
            Action, ETriggerEvent::Started, this,
            &AMainPlayerController::HandleJumpStarted);
        EnhancedInputComponent->BindAction(
            Action, ETriggerEvent::Completed, this,
            &AMainPlayerController::HandleJumpCompleted);
      } else if (ActionName.Contains(TEXT("Sprint")) ||
                 ActionName.Contains(TEXT("IA_Sprint"))) {
        EnhancedInputComponent->BindAction(
            Action, ETriggerEvent::Started, this,
            &AMainPlayerController::HandleSprintStarted);
        EnhancedInputComponent->BindAction(
            Action, ETriggerEvent::Completed, this,
            &AMainPlayerController::HandleSprintCompleted);
      } else if (ActionName.Contains(TEXT("Interact")) ||
                 ActionName.Contains(TEXT("IA_Interact"))) {
        EnhancedInputComponent->BindAction(
            Action, ETriggerEvent::Started, this,
            &AMainPlayerController::HandleInteract);
      } else if (ActionName.Contains(TEXT("Inventory")) ||
                 ActionName.Contains(TEXT("IA_Inventory"))) {
        EnhancedInputComponent->BindAction(
            Action, ETriggerEvent::Started, this,
            &AMainPlayerController::HandleInventoryToggle);
      } else if (ActionName.Contains(TEXT("ExitSeat")) ||
                 ActionName.Contains(TEXT("Pause")) ||
                 ActionName.Contains(TEXT("Menu"))) {
        UInputAction *MutableAction = const_cast<UInputAction *>(Action);
        if (MutableAction) {
          MutableAction->bTriggerWhenPaused = true;
        }

        EnhancedInputComponent->BindAction(
            Action, ETriggerEvent::Started, this,
            &AMainPlayerController::HandleMenuToggle);
        bBoundMenuToggleAction = true;
      } else if (ActionName.Contains(TEXT("SaveLoad")) ||
                 ActionName.Contains(TEXT("QuickLoad"))) {
        EnhancedInputComponent->BindAction(
            Action, ETriggerEvent::Started, this,
            &AMainPlayerController::HandleQuickLoad);
      } else if (ActionName.Contains(TEXT("IA_Save")) ||
                 ActionName.Contains(TEXT("QuickSave")) ||
                 ActionName.Equals(TEXT("Save"))) {
        EnhancedInputComponent->BindAction(
            Action, ETriggerEvent::Started, this,
            &AMainPlayerController::HandleQuickSave);
      } else if (ActionName.Contains(TEXT("Fire")) ||
                 ActionName.Contains(TEXT("IA_Fire"))) {
        EnhancedInputComponent->BindAction(
            Action, ETriggerEvent::Started, this,
            &AMainPlayerController::HandleFireStarted);
        EnhancedInputComponent->BindAction(
            Action, ETriggerEvent::Completed, this,
            &AMainPlayerController::HandleFireCompleted);
      } else if (ActionName.Contains(TEXT("Reload")) ||
                 ActionName.Contains(TEXT("IA_Reload"))) {
        EnhancedInputComponent->BindAction(
            Action, ETriggerEvent::Started, this,
            &AMainPlayerController::HandleReload);
      } else if (ActionName.Contains(TEXT("Scrol")) ||
                 ActionName.Contains(TEXT("Scroll")) ||
                 ActionName.Contains(TEXT("WeaponCycle"))) {
        EnhancedInputComponent->BindAction(
            Action, ETriggerEvent::Triggered, this,
            &AMainPlayerController::HandleWeaponCycle);
      } else if (ActionName.Contains(TEXT("Scope")) ||
                 ActionName.Contains(TEXT("IA_Scope")) ||
                 ActionName.Contains(TEXT("Aim"))) {
        EnhancedInputComponent->BindAction(
            Action, ETriggerEvent::Started, this,
            &AMainPlayerController::HandleScopeStarted);
        EnhancedInputComponent->BindAction(
            Action, ETriggerEvent::Completed, this,
            &AMainPlayerController::HandleScopeCompleted);
        EnhancedInputComponent->BindAction(
            Action, ETriggerEvent::Canceled, this,
            &AMainPlayerController::HandleScopeCompleted);
      }

      BoundActions.Add(Action);
    }
  }

  if (!bBoundMenuToggleAction) {
    UE_LOG(LogProject, Warning,
           TEXT("No menu toggle input action was found in the active mapping contexts. "
                "Add IA_ExitSeat (or another Pause/Menu action) to IMC_Default."));
  }
}

void AMainPlayerController::OnPossess(APawn *InPawn) {
  Super::OnPossess(InPawn);

  UE_LOG(LogProject, Verbose,
         TEXT("MainPlayerController::OnPossess - Pawn: %s"),
         InPawn ? *InPawn->GetName() : TEXT("NULL"));

  if (!InPawn) {
    return;
  }

  if (ACharacter *PossessedCharacter = Cast<ACharacter>(InPawn)) {
    if (UCharacterMovementComponent *MovementComp =
            PossessedCharacter->GetCharacterMovement()) {
      UE_LOG(LogProject, Verbose,
             TEXT("CharacterMovementComponent found, MovementMode: %d"),
             (int32)MovementComp->MovementMode);
    }
  }

  if (UWorld *World = GetWorld()) {
    TWeakObjectPtr<APawn> WeakPawn = InPawn;
    World->GetTimerManager().SetTimerForNextTick(
        FTimerDelegate::CreateWeakLambda(this, [this, WeakPawn]() {
          ApplyStartupPawnState(WeakPawn.Get());
        }));
  } else {
    ApplyStartupPawnState(InPawn);
  }
}

void AMainPlayerController::ApplyStartupPawnState(APawn *InPawn) {
  if (!InPawn || !PlayerArmoryComponent) {
    return;
  }

  ApplyInventoryWidgetLayoutDefaults();
  PlayerArmoryComponent->BindToPawn(InPawn);

  if (bSkipNextStartupPawnStateApplication) {
    ConsumeSaveRestoreStartupSkip();
    return;
  }

  if (const UProjectSaveSubsystem *SaveSubsystem =
          GetGameInstance() ? GetGameInstance()->GetSubsystem<UProjectSaveSubsystem>()
                            : nullptr) {
    if (SaveSubsystem->HasPendingRestore()) {
      if (UWorld *World = GetWorld()) {
        TWeakObjectPtr<APawn> WeakPawn = InPawn;
        World->GetTimerManager().SetTimerForNextTick(
            FTimerDelegate::CreateWeakLambda(this, [this, WeakPawn]() {
              ApplyStartupPawnState(WeakPawn.Get());
            }));
      }
      return;
    }
  }

  if (!bApplyStartupPawnStateOnPossess) {
    return;
  }

  if (bApplyStartupPawnStateOnlyOnce && bHasAppliedStartupPawnState) {
    return;
  }

  if (bClearStartingLoadoutOnPossess) {
    if (UCombatComponent *CombatComp =
            InPawn->FindComponentByClass<UCombatComponent>()) {
      CombatComp->ClearLoadout();
    }
  }

  int32 InitialCurrency = 0;
  if (bSetStartingCurrencyOnPossess) {
    InitialCurrency = StartingCurrency;
  } else if (const UCurrencyComponent *CurrencyComp =
                 InPawn->FindComponentByClass<UCurrencyComponent>()) {
    InitialCurrency = CurrencyComp->GetCurrency();
  }

  PlayerArmoryComponent->InitializeEmptySession(InitialCurrency);
  bHasAppliedStartupPawnState = true;
}

void AMainPlayerController::ConsumeSaveRestoreStartupSkip() {
  bSkipNextStartupPawnStateApplication = false;
  bHasAppliedStartupPawnState = true;
}

bool AMainPlayerController::ShouldUseTouchControls() const {
  return SVirtualJoystick::ShouldDisplayTouchInterface() || bForceTouchControls;
}

void AMainPlayerController::ApplyInventoryWidgetLayoutDefaults() {
  if (!PlayerArmoryComponent || !InventoryWidgetClass) {
    return;
  }

  const UPlayerInventoryWidgetBase *InventoryWidgetDefaults =
      InventoryWidgetClass->GetDefaultObject<UPlayerInventoryWidgetBase>();
  if (!InventoryWidgetDefaults) {
    return;
  }

  PlayerArmoryComponent->SetStorageGridDimensions(
      InventoryWidgetDefaults->GetConfiguredStorageGridColumns(),
      InventoryWidgetDefaults->GetConfiguredStorageGridRows());
}

void AMainPlayerController::UpdateArmoryOverlayInputState() {
  const bool bOverlayOpen = IsAnyArmoryOverlayOpen();
  SetMouseCursorVisible(bOverlayOpen);
  SetIgnoreMoveInput(bOverlayOpen);
  SetIgnoreLookInput(bOverlayOpen);

  if (HUDWidget) {
    HUDWidget->BP_OnArmoryOverlayStateChanged(bOverlayOpen);
  }
}

void AMainPlayerController::CloseAllArmoryOverlays() {
  CloseWeaponShop();
  CloseInventory();
}

void AMainPlayerController::OpenWeaponShop(AWeaponShopTerminal *ShopTerminal) {
  if (!IsLocalPlayerController() || !WeaponShopWidgetClass || !ShopTerminal) {
    return;
  }

  if (InventoryWidget && InventoryWidget->IsInViewport()) {
    InventoryWidget->RemoveFromParent();
  }

  if (!WeaponShopWidget) {
    WeaponShopWidget = CreateWidget<UWeaponShopWidgetBase>(this, WeaponShopWidgetClass);
  }
  if (!WeaponShopWidget) {
    UE_LOG(LogProject, Warning, TEXT("Weapon shop widget class is not configured"));
    return;
  }

  WeaponShopWidget->SetShopTerminal(ShopTerminal);
  if (!WeaponShopWidget->IsInViewport()) {
    WeaponShopWidget->AddToPlayerScreen(ArmoryWidgetZOrder);
  }

  UpdateArmoryOverlayInputState();
}

void AMainPlayerController::CloseWeaponShop() {
  if (WeaponShopWidget && WeaponShopWidget->IsInViewport()) {
    WeaponShopWidget->RemoveFromParent();
    WeaponShopWidget->SetShopTerminal(nullptr);
  }

  UpdateArmoryOverlayInputState();
}

void AMainPlayerController::OpenInventory() {
  if (!IsLocalPlayerController() || !InventoryWidgetClass) {
    return;
  }

  ApplyInventoryWidgetLayoutDefaults();

  if (WeaponShopWidget && WeaponShopWidget->IsInViewport()) {
    WeaponShopWidget->RemoveFromParent();
  }

  if (!InventoryWidget) {
    InventoryWidget =
        CreateWidget<UPlayerInventoryWidgetBase>(this, InventoryWidgetClass);
  }
  if (!InventoryWidget) {
    UE_LOG(LogProject, Warning, TEXT("Inventory widget class is not configured"));
    return;
  }

  if (!InventoryWidget->IsInViewport()) {
    InventoryWidget->AddToPlayerScreen(ArmoryWidgetZOrder);
  }

  UpdateArmoryOverlayInputState();
}

void AMainPlayerController::CloseInventory() {
  if (InventoryWidget && InventoryWidget->IsInViewport()) {
    InventoryWidget->RemoveFromParent();
  }

  UpdateArmoryOverlayInputState();
}

void AMainPlayerController::ToggleInventory() {
  if (WeaponShopWidget && WeaponShopWidget->IsInViewport()) {
    CloseWeaponShop();
    OpenInventory();
    return;
  }

  if (InventoryWidget && InventoryWidget->IsInViewport()) {
    CloseInventory();
    return;
  }

  OpenInventory();
}

bool AMainPlayerController::IsAnyArmoryOverlayOpen() const {
  return bHasExternalArmoryOverlayOpen ||
         (WeaponShopWidget && WeaponShopWidget->IsInViewport()) ||
         (InventoryWidget && InventoryWidget->IsInViewport());
}

void AMainPlayerController::SetExternalArmoryOverlayOpen(bool bIsOpen) {
  bHasExternalArmoryOverlayOpen = bIsOpen;
  UpdateArmoryOverlayInputState();
}

void AMainPlayerController::HandleMove(const FInputActionValue &Value) {
  const FVector2D MovementVector = Value.Get<FVector2D>();

  APawn *ControlledPawn = GetPawn();
  if (!ControlledPawn) {
    return;
  }

  const FRotator Rotation = GetControlRotation();
  const FRotator YawRotation(0, Rotation.Yaw, 0);
  const FVector ForwardDirection =
      FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
  const FVector RightDirection =
      FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

  ControlledPawn->AddMovementInput(ForwardDirection, MovementVector.Y);
  ControlledPawn->AddMovementInput(RightDirection, MovementVector.X);
}

void AMainPlayerController::HandleLook(const FInputActionValue &Value) {
  const FVector2D LookAxisVector = Value.Get<FVector2D>();
  AddYawInput(LookAxisVector.X);
  AddPitchInput(LookAxisVector.Y);
}

void AMainPlayerController::HandleJumpStarted(const FInputActionValue &Value) {
  if (ACharacter *ControlledCharacter = Cast<ACharacter>(GetPawn())) {
    ControlledCharacter->Jump();
  }
}

void AMainPlayerController::HandleJumpCompleted(const FInputActionValue &Value) {
  if (ACharacter *ControlledCharacter = Cast<ACharacter>(GetPawn())) {
    ControlledCharacter->StopJumping();
  }
}

void AMainPlayerController::HandleSprintStarted(const FInputActionValue &Value) {
  APawn *ControlledPawn = GetPawn();
  if (!ControlledPawn || IsAnyArmoryOverlayOpen()) {
    return;
  }

  if (ControlledPawn->Implements<UControllableCharacterInterface>()) {
    IControllableCharacterInterface::Execute_StartSprint(ControlledPawn);
  }
}

void AMainPlayerController::HandleSprintCompleted(const FInputActionValue &Value) {
  APawn *ControlledPawn = GetPawn();
  if (!ControlledPawn) {
    return;
  }

  if (ControlledPawn->Implements<UControllableCharacterInterface>()) {
    IControllableCharacterInterface::Execute_StopSprint(ControlledPawn);
  }
}

void AMainPlayerController::HandleInteract(const FInputActionValue &Value) {
  if (IsAnyArmoryOverlayOpen()) {
    return;
  }

  APawn *ControlledPawn = GetPawn();
  if (!ControlledPawn) {
    return;
  }

  if (UInteractionComponent *InteractionComp =
          ControlledPawn->FindComponentByClass<UInteractionComponent>()) {
    InteractionComp->TryInteract();
    return;
  }

  if (ControlledPawn->Implements<UControllableCharacterInterface>()) {
    IControllableCharacterInterface::Execute_DoInteract(ControlledPawn);
  }
}

void AMainPlayerController::HandleFireStarted(const FInputActionValue &Value) {
  if (IsAnyArmoryOverlayOpen()) {
    return;
  }

  APawn *ControlledPawn = GetPawn();
  if (!ControlledPawn) {
    return;
  }

  if (UCombatComponent *CombatComp =
          ControlledPawn->FindComponentByClass<UCombatComponent>()) {
    CombatComp->StartFire();
    return;
  }

  if (ControlledPawn->Implements<UControllableCharacterInterface>()) {
    IControllableCharacterInterface::Execute_StartFire(ControlledPawn);
  }
}

void AMainPlayerController::HandleFireCompleted(const FInputActionValue &Value) {
  if (IsAnyArmoryOverlayOpen()) {
    return;
  }

  APawn *ControlledPawn = GetPawn();
  if (!ControlledPawn) {
    return;
  }

  if (UCombatComponent *CombatComp =
          ControlledPawn->FindComponentByClass<UCombatComponent>()) {
    CombatComp->StopFire();
    return;
  }

  if (ControlledPawn->Implements<UControllableCharacterInterface>()) {
    IControllableCharacterInterface::Execute_StopFire(ControlledPawn);
  }
}

void AMainPlayerController::HandleReload(const FInputActionValue &Value) {
  if (IsAnyArmoryOverlayOpen()) {
    return;
  }

  APawn *ControlledPawn = GetPawn();
  if (!ControlledPawn) {
    return;
  }

  if (UCombatComponent *CombatComp =
          ControlledPawn->FindComponentByClass<UCombatComponent>()) {
    CombatComp->Reload();
  }
}

void AMainPlayerController::HandleScopeStarted(const FInputActionValue &Value) {
  if (IsAnyArmoryOverlayOpen()) {
    return;
  }

  APawn *ControlledPawn = GetPawn();
  if (!ControlledPawn) {
    return;
  }

  if (UCombatComponent *CombatComp =
          ControlledPawn->FindComponentByClass<UCombatComponent>()) {
    CombatComp->StartScope();
  }
}

void AMainPlayerController::HandleScopeCompleted(const FInputActionValue &Value) {
  if (IsAnyArmoryOverlayOpen()) {
    return;
  }

  APawn *ControlledPawn = GetPawn();
  if (!ControlledPawn) {
    return;
  }

  if (UCombatComponent *CombatComp =
          ControlledPawn->FindComponentByClass<UCombatComponent>()) {
    CombatComp->StopScope();
  }
}

void AMainPlayerController::HandleWeaponCycle(const FInputActionValue &Value) {
  if (IsAnyArmoryOverlayOpen()) {
    return;
  }

  APawn *ControlledPawn = GetPawn();
  if (!ControlledPawn) {
    return;
  }

  const float CycleValue = Value.Get<float>();
  if (FMath::Abs(CycleValue) <= KINDA_SMALL_NUMBER) {
    return;
  }

  if (const UWorld *World = GetWorld()) {
    const float TimeSeconds = World->GetTimeSeconds();
    if (TimeSeconds - LastWeaponCycleInputTimeSeconds <
        FMath::Max(0.0f, WeaponCycleInputCooldown)) {
      return;
    }
    LastWeaponCycleInputTimeSeconds = TimeSeconds;
  }

  if (UCombatComponent *CombatComp =
          ControlledPawn->FindComponentByClass<UCombatComponent>()) {
    if (CycleValue > 0.0f) {
      CombatComp->EquipNextWeapon();
    } else {
      CombatComp->EquipPreviousWeapon();
    }
  }
}

void AMainPlayerController::HandleInventoryToggle(const FInputActionValue &Value) {
  ToggleInventory();
}

void AMainPlayerController::HandleQuickSave(const FInputActionValue &Value) {
  RequestQuickSave();
}

void AMainPlayerController::HandleQuickLoad(const FInputActionValue &Value) {
  RequestQuickLoad();
}

void AMainPlayerController::HandleMenuToggle(const FInputActionValue &Value) {
  HandleCloseOverlayKey();
}

void AMainPlayerController::HandleCloseOverlayKey() {
  if (WeaponShopWidget && WeaponShopWidget->IsInViewport()) {
    CloseWeaponShop();
    return;
  }

  if (InventoryWidget && InventoryWidget->IsInViewport()) {
    CloseInventory();
    return;
  }

  if (UProjectGameViewportClient *ProjectViewportClient =
          Cast<UProjectGameViewportClient>(GetWorld()
                                               ? GetWorld()->GetGameViewport()
                                               : nullptr)) {
    if (ProjectViewportClient->HandleEscapeMenuAction()) {
      return;
    }
  }
}

void AMainPlayerController::SetApplyStartupPawnStateOnPossess(
    bool bShouldApply) {
  bApplyStartupPawnStateOnPossess = bShouldApply;
}

void AMainPlayerController::ResetStartupPawnStateTracking() {
  bHasAppliedStartupPawnState = false;
}

void AMainPlayerController::MarkStartupStateRestoredFromSave() {
  bSkipNextStartupPawnStateApplication = true;
  bHasAppliedStartupPawnState = true;
}

void AMainPlayerController::SetMouseCursorVisible(bool bVisible) {
  bShowMouseCursor = bVisible;
  bIsInteractingWithUI = bVisible;

  if (bVisible) {
    FInputModeGameAndUI InputMode;
    InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
    InputMode.SetHideCursorDuringCapture(false);
    SetInputMode(InputMode);
  } else {
    FInputModeGameOnly InputMode;
    SetInputMode(InputMode);
  }
}

void AMainPlayerController::SetStartupMenuEnabled(bool bEnabled) {
  if (UProjectGameViewportClient *ProjectViewportClient =
          Cast<UProjectGameViewportClient>(GetWorld()
                                               ? GetWorld()->GetGameViewport()
                                               : nullptr)) {
    ProjectViewportClient->SetStartupMenuEnabled(bEnabled);
  }
}

bool AMainPlayerController::IsStartupMenuEnabled() const {
  if (const UProjectGameViewportClient *ProjectViewportClient =
          Cast<UProjectGameViewportClient>(GetWorld()
                                               ? GetWorld()->GetGameViewport()
                                               : nullptr)) {
    return ProjectViewportClient->IsStartupMenuEnabled();
  }

  return false;
}

void AMainPlayerController::SetInGameMenuEnabled(bool bEnabled) {
  if (UProjectGameViewportClient *ProjectViewportClient =
          Cast<UProjectGameViewportClient>(GetWorld()
                                               ? GetWorld()->GetGameViewport()
                                               : nullptr)) {
    ProjectViewportClient->SetInGameMenuEnabled(bEnabled);
  }
}

bool AMainPlayerController::IsInGameMenuEnabled() const {
  if (const UProjectGameViewportClient *ProjectViewportClient =
          Cast<UProjectGameViewportClient>(GetWorld()
                                               ? GetWorld()->GetGameViewport()
                                               : nullptr)) {
    return ProjectViewportClient->IsInGameMenuEnabled();
  }

  return false;
}

bool AMainPlayerController::RequestQuickSave() {
  if (UProjectSaveSubsystem *SaveSubsystem =
          GetGameInstance() ? GetGameInstance()->GetSubsystem<UProjectSaveSubsystem>()
                            : nullptr) {
    return SaveSubsystem->QuickSave();
  }

  return false;
}

bool AMainPlayerController::RequestQuickLoad() {
  if (UProjectSaveSubsystem *SaveSubsystem =
          GetGameInstance() ? GetGameInstance()->GetSubsystem<UProjectSaveSubsystem>()
                            : nullptr) {
    return SaveSubsystem->QuickLoad();
  }

  return false;
}

bool AMainPlayerController::RequestManualSave() {
  if (UProjectSaveSubsystem *SaveSubsystem =
          GetGameInstance() ? GetGameInstance()->GetSubsystem<UProjectSaveSubsystem>()
                            : nullptr) {
    return SaveSubsystem->ManualSave();
  }

  return false;
}

bool AMainPlayerController::RequestManualLoad() {
  if (UProjectSaveSubsystem *SaveSubsystem =
          GetGameInstance() ? GetGameInstance()->GetSubsystem<UProjectSaveSubsystem>()
                            : nullptr) {
    return SaveSubsystem->ManualLoad();
  }

  return false;
}

bool AMainPlayerController::RequestOverwriteSaveSlot(const FString &SlotName) {
  if (UProjectSaveSubsystem *SaveSubsystem =
          GetGameInstance() ? GetGameInstance()->GetSubsystem<UProjectSaveSubsystem>()
                            : nullptr) {
    return SaveSubsystem->OverwriteSaveSlot(SlotName);
  }

  return false;
}

bool AMainPlayerController::RequestDeleteSaveSlot(const FString &SlotName) {
  if (UProjectSaveSubsystem *SaveSubsystem =
          GetGameInstance() ? GetGameInstance()->GetSubsystem<UProjectSaveSubsystem>()
                            : nullptr) {
    return SaveSubsystem->DeleteSaveSlot(SlotName);
  }

  return false;
}
