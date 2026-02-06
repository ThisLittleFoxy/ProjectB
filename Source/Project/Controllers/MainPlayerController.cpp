// Copyright Epic Games, Inc. All Rights Reserved.

#include "MainPlayerController.h"
#include "Blueprint/UserWidget.h"
#include "Character/ControllableCharacterInterface.h"
#include "Engine/LocalPlayer.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Project.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "InputAction.h"
#include "InputMappingContext.h"
#include "Widgets/Input/SVirtualJoystick.h"

AMainPlayerController::AMainPlayerController() {
  // Enable widget interaction
  bShowMouseCursor = false;
  bEnableClickEvents = true;
  bEnableTouchEvents = true;
  bEnableMouseOverEvents = false;
  bEnableTouchOverEvents = false;
}

void AMainPlayerController::BeginPlay() {
  Super::BeginPlay();

  UE_LOG(LogProject, Log,
         TEXT("MainPlayerController::BeginPlay - Controller: %s"), *GetName());
  UE_LOG(LogProject, Log, TEXT("IsLocalPlayerController: %s"),
         IsLocalPlayerController() ? TEXT("true") : TEXT("false"));
  UE_LOG(LogProject, Log, TEXT("ShouldUseTouchControls: %s"),
         ShouldUseTouchControls() ? TEXT("true") : TEXT("false"));

  // only spawn touch controls on local player controllers
  if (ShouldUseTouchControls() && IsLocalPlayerController()) {
    // spawn the mobile controls widget
    MobileControlsWidget =
        CreateWidget<UUserWidget>(this, MobileControlsWidgetClass);
    if (MobileControlsWidget) {
      // add the controls to the player screen
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

  // only add IMCs for local player controllers
  if (IsLocalPlayerController()) {
    // Add Input Mapping Contexts
    if (UEnhancedInputLocalPlayerSubsystem *Subsystem =
            ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(
                GetLocalPlayer())) {
      UE_LOG(LogProject, Log, TEXT("Enhanced Input Subsystem found"));
      UE_LOG(LogProject, Log, TEXT("DefaultMappingContexts count: %d"),
             DefaultMappingContexts.Num());

      for (int32 i = 0; i < DefaultMappingContexts.Num(); ++i) {
        UInputMappingContext *CurrentContext = DefaultMappingContexts[i];
        if (CurrentContext) {
          Subsystem->AddMappingContext(CurrentContext, 0);
          UE_LOG(LogProject, Log,
                 TEXT("Added DefaultMappingContext [%d]: %s"), i,
                 *CurrentContext->GetName());
        } else {
          UE_LOG(LogProject, Warning,
                 TEXT("DefaultMappingContext [%d] is NULL"), i);
        }
      }

      // only add these IMCs if we're not using mobile touch input
      if (!ShouldUseTouchControls()) {
        UE_LOG(LogProject, Log,
               TEXT("MobileExcludedMappingContexts count: %d"),
               MobileExcludedMappingContexts.Num());

        for (int32 i = 0; i < MobileExcludedMappingContexts.Num(); ++i) {
          UInputMappingContext *CurrentContext =
              MobileExcludedMappingContexts[i];
          if (CurrentContext) {
            Subsystem->AddMappingContext(CurrentContext, 0);
            UE_LOG(LogProject, Log,
                   TEXT("Added MobileExcludedMappingContext [%d]: %s"), i,
                   *CurrentContext->GetName());
          } else {
            UE_LOG(LogProject, Warning,
                   TEXT("MobileExcludedMappingContext [%d] is NULL"), i);
          }
        }
      }
    } else {
      UE_LOG(LogProject, Error,
             TEXT("Failed to get Enhanced Input Subsystem!"));
    }

    // Automatically bind all input actions from mapping contexts
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

  UE_LOG(LogProject, Log,
         TEXT("EnhancedInputComponent cast successful, binding actions from "
              "IMC..."));

  // Collect all mapping contexts
  TArray<UInputMappingContext *> AllContexts;
  AllContexts.Append(DefaultMappingContexts);
  if (!ShouldUseTouchControls()) {
    AllContexts.Append(MobileExcludedMappingContexts);
  }

  // Iterate through all mapping contexts and bind actions
  TSet<const UInputAction *> BoundActions;
  for (UInputMappingContext *Context : AllContexts) {
    if (!Context) {
      continue;
    }

    UE_LOG(LogProject, Log, TEXT("Processing mapping context: %s"),
           *Context->GetName());

    // Get all mappings from the context
    const TArray<FEnhancedActionKeyMapping> &Mappings = Context->GetMappings();
    UE_LOG(LogProject, Log, TEXT("Found %d mappings in context"),
           Mappings.Num());

    for (const FEnhancedActionKeyMapping &Mapping : Mappings) {
      const UInputAction *Action = Mapping.Action;
      if (!Action || BoundActions.Contains(Action)) {
        continue;
      }

      FString ActionName = Action->GetName();
      UE_LOG(LogProject, Log, TEXT("Found Input Action: %s"), *ActionName);

      // Bind based on action name
      if (ActionName.Contains(TEXT("Move")) ||
          ActionName.Contains(TEXT("IA_Move"))) {
        EnhancedInputComponent->BindAction(Action, ETriggerEvent::Triggered,
                                           this,
                                           &AMainPlayerController::HandleMove);
        UE_LOG(LogProject, Log, TEXT("Bound action '%s' to HandleMove"),
               *ActionName);
        BoundActions.Add(Action);
      } else if (ActionName.Contains(TEXT("Look")) ||
                 ActionName.Contains(TEXT("IA_Look"))) {
        EnhancedInputComponent->BindAction(Action, ETriggerEvent::Triggered,
                                           this,
                                           &AMainPlayerController::HandleLook);
        UE_LOG(LogProject, Log, TEXT("Bound action '%s' to HandleLook"),
               *ActionName);
        BoundActions.Add(Action);
      } else if (ActionName.Contains(TEXT("Jump")) ||
                 ActionName.Contains(TEXT("IA_Jump"))) {
        EnhancedInputComponent->BindAction(
            Action, ETriggerEvent::Started, this,
            &AMainPlayerController::HandleJumpStarted);
        EnhancedInputComponent->BindAction(
            Action, ETriggerEvent::Completed, this,
            &AMainPlayerController::HandleJumpCompleted);
        UE_LOG(LogProject, Log, TEXT("Bound action '%s' to HandleJump"),
               *ActionName);
        BoundActions.Add(Action);
      } else if (ActionName.Contains(TEXT("Sprint")) ||
                 ActionName.Contains(TEXT("IA_Sprint"))) {
        EnhancedInputComponent->BindAction(
            Action, ETriggerEvent::Started, this,
            &AMainPlayerController::HandleSprintStarted);
        EnhancedInputComponent->BindAction(
            Action, ETriggerEvent::Completed, this,
            &AMainPlayerController::HandleSprintCompleted);
        UE_LOG(LogProject, Log, TEXT("Bound action '%s' to HandleSprint"),
               *ActionName);
        BoundActions.Add(Action);
      } else if (ActionName.Contains(TEXT("Interact")) ||
                 ActionName.Contains(TEXT("IA_Interact"))) {
        EnhancedInputComponent->BindAction(
            Action, ETriggerEvent::Started, this,
            &AMainPlayerController::HandleInteract);
        UE_LOG(LogProject, Log, TEXT("Bound action '%s' to HandleInteract"),
               *ActionName);
        BoundActions.Add(Action);
      } else if (ActionName.Contains(TEXT("Fire")) ||
                 ActionName.Contains(TEXT("IA_Fire"))) {
        EnhancedInputComponent->BindAction(
            Action, ETriggerEvent::Started, this,
            &AMainPlayerController::HandleFireStarted);
        EnhancedInputComponent->BindAction(
            Action, ETriggerEvent::Completed, this,
            &AMainPlayerController::HandleFireCompleted);
        UE_LOG(LogProject, Log, TEXT("Bound action '%s' to HandleFire"),
               *ActionName);
        BoundActions.Add(Action);
      } else {
        UE_LOG(LogProject, Warning, TEXT("No handler found for action: %s"),
               *ActionName);
      }
    }
  }

  UE_LOG(LogProject, Log, TEXT("Total actions bound: %d"),
         BoundActions.Num());
}

void AMainPlayerController::OnPossess(APawn *InPawn) {
  Super::OnPossess(InPawn);

  UE_LOG(LogProject, Verbose,
         TEXT("MainPlayerController::OnPossess - Pawn: %s"),
         InPawn ? *InPawn->GetName() : TEXT("NULL"));

  if (InPawn) {
    ACharacter *PossessedCharacter = Cast<ACharacter>(InPawn);
    if (PossessedCharacter) {
      UE_LOG(LogProject, Verbose, TEXT("Possessed Character: %s"),
             *PossessedCharacter->GetName());
      if (UCharacterMovementComponent *MovementComp =
              PossessedCharacter->GetCharacterMovement()) {
        UE_LOG(LogProject, Verbose,
               TEXT("CharacterMovementComponent found, MovementMode: %d"),
               (int32)MovementComp->MovementMode);
      } else {
        UE_LOG(LogProject, Warning,
               TEXT("CharacterMovementComponent not found on Character!"));
      }
    } else {
      UE_LOG(LogProject, Warning,
             TEXT("Possessed pawn is not a Character!"));
    }
  }
}

bool AMainPlayerController::ShouldUseTouchControls() const {
  // are we on a mobile platform? Should we force touch?
  return SVirtualJoystick::ShouldDisplayTouchInterface() || bForceTouchControls;
}

// ========== Movement Handlers ==========

void AMainPlayerController::HandleMove(const FInputActionValue &Value) {
  const FVector2D MovementVector = Value.Get<FVector2D>();

  APawn *ControlledPawn = GetPawn();
  if (!ControlledPawn) {
    return;
  }

  // Calculate direction based on controller rotation (FPS style)
  const FRotator Rotation = GetControlRotation();
  const FRotator YawRotation(0, Rotation.Yaw, 0);

  const FVector ForwardDirection =
      FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
  const FVector RightDirection =
      FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

  // Apply movement input
  ControlledPawn->AddMovementInput(ForwardDirection, MovementVector.Y);
  ControlledPawn->AddMovementInput(RightDirection, MovementVector.X);
}

void AMainPlayerController::HandleLook(const FInputActionValue &Value) {
  const FVector2D LookAxisVector = Value.Get<FVector2D>();

  // Add yaw and pitch input to controller
  AddYawInput(LookAxisVector.X);
  AddPitchInput(LookAxisVector.Y);
}

// ========== Jump Handlers ==========

void AMainPlayerController::HandleJumpStarted(const FInputActionValue &Value) {
  if (ACharacter *ControlledCharacter = Cast<ACharacter>(GetPawn())) {
    ControlledCharacter->Jump();
  }
}

void AMainPlayerController::HandleJumpCompleted(
    const FInputActionValue &Value) {
  if (ACharacter *ControlledCharacter = Cast<ACharacter>(GetPawn())) {
    ControlledCharacter->StopJumping();
  }
}

// ========== Sprint Handlers ==========

void AMainPlayerController::HandleSprintStarted(
    const FInputActionValue &Value) {
  APawn *ControlledPawn = GetPawn();
  if (!ControlledPawn) {
    return;
  }

  // Call interface method if pawn implements it
  if (ControlledPawn->Implements<UControllableCharacterInterface>()) {
    IControllableCharacterInterface::Execute_StartSprint(ControlledPawn);
    UE_LOG(LogProject, Log, TEXT("Sprint started via interface"));
  }
}

void AMainPlayerController::HandleSprintCompleted(
    const FInputActionValue &Value) {
  APawn *ControlledPawn = GetPawn();
  if (!ControlledPawn) {
    return;
  }

  // Call interface method if pawn implements it
  if (ControlledPawn->Implements<UControllableCharacterInterface>()) {
    IControllableCharacterInterface::Execute_StopSprint(ControlledPawn);
    UE_LOG(LogProject, Log, TEXT("Sprint stopped via interface"));
  }
}

// ========== Interact Handler ==========

void AMainPlayerController::HandleInteract(const FInputActionValue &Value) {
  APawn *ControlledPawn = GetPawn();
  if (!ControlledPawn) {
    return;
  }

  // Call interface method if pawn implements it
  if (ControlledPawn->Implements<UControllableCharacterInterface>()) {
    IControllableCharacterInterface::Execute_DoInteract(ControlledPawn);
    UE_LOG(LogProject, Log, TEXT("Interact triggered via interface"));
  }
}

// ========== Fire Handlers ==========

void AMainPlayerController::HandleFireStarted(const FInputActionValue &Value) {
  APawn *ControlledPawn = GetPawn();
  if (!ControlledPawn) {
    return;
  }

  // Call interface method if pawn implements it
  if (ControlledPawn->Implements<UControllableCharacterInterface>()) {
    IControllableCharacterInterface::Execute_StartFire(ControlledPawn);
    UE_LOG(LogProject, Log, TEXT("Fire started via interface"));
  }
}

void AMainPlayerController::HandleFireCompleted(
    const FInputActionValue &Value) {
  APawn *ControlledPawn = GetPawn();
  if (!ControlledPawn) {
    return;
  }

  // Call interface method if pawn implements it
  if (ControlledPawn->Implements<UControllableCharacterInterface>()) {
    IControllableCharacterInterface::Execute_StopFire(ControlledPawn);
    UE_LOG(LogProject, Log, TEXT("Fire stopped via interface"));
  }
}

void AMainPlayerController::SetMouseCursorVisible(bool bVisible) {
  bShowMouseCursor = bVisible;
  bIsInteractingWithUI = bVisible;

  // Set input mode based on cursor visibility
  if (bVisible) {
    // Allow both game and UI input
    FInputModeGameAndUI InputMode;
    InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
    InputMode.SetHideCursorDuringCapture(false);
    SetInputMode(InputMode);

    UE_LOG(LogProject, Log, TEXT("Mouse cursor enabled for UI interaction"));
  } else {
    // Game input only
    FInputModeGameOnly InputMode;
    SetInputMode(InputMode);

    UE_LOG(LogProject, Log, TEXT("Mouse cursor disabled, game mode active"));
  }
}
