// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "MainPlayerController.generated.h"

class UInputMappingContext;
class UUserWidget;
struct FInputActionValue;

/**
 * FPS PlayerController that handles all input and delegates to the controlled pawn.
 *
 * The controller processes input and calls methods on the pawn via IControllableCharacterInterface.
 * For basic movement (Move, Look, Jump), it uses standard Pawn/Character methods.
 * For extended actions (Sprint, Interact, Fire), it uses the interface.
 *
 * Your Blueprint Character should implement IControllableCharacterInterface to receive
 * Sprint, Interact, and Fire inputs.
 */
UCLASS(abstract)
class AMainPlayerController : public APlayerController {
  GENERATED_BODY()

public:
  AMainPlayerController();

protected:
  // ========== Input Mapping Contexts ==========

  /** Input Mapping Contexts applied to all platforms */
  UPROPERTY(EditAnywhere, Category = "Input|Input Mappings")
  TArray<UInputMappingContext *> DefaultMappingContexts;

  /** Input Mapping Contexts excluded on mobile (e.g., mouse look) */
  UPROPERTY(EditAnywhere, Category = "Input|Input Mappings")
  TArray<UInputMappingContext *> MobileExcludedMappingContexts;

  // ========== Touch Controls ==========

  /** Mobile controls widget to spawn */
  UPROPERTY(EditAnywhere, Category = "Input|Touch Controls")
  TSubclassOf<UUserWidget> MobileControlsWidgetClass;

  /** Pointer to the mobile controls widget */
  UPROPERTY()
  TObjectPtr<UUserWidget> MobileControlsWidget;

  /** If true, force UMG touch controls even on non-mobile platforms */
  UPROPERTY(EditAnywhere, Config, Category = "Input|Touch Controls")
  bool bForceTouchControls = false;

  // ========== UI State ==========

  /** Currently interacting with UI widget */
  UPROPERTY(BlueprintReadOnly, Category = "UI")
  bool bIsInteractingWithUI = false;

  // ========== Lifecycle ==========

  virtual void BeginPlay() override;
  virtual void SetupInputComponent() override;
  virtual void OnPossess(APawn *InPawn) override;

  // ========== Input Setup ==========

  /** Returns true if the player should use UMG touch controls */
  bool ShouldUseTouchControls() const;

  /** Bind all input actions from mapping contexts automatically */
  void BindInputActions();

  // ========== Input Handlers ==========

  /** Movement input - calculates direction from camera and applies to pawn */
  UFUNCTION()
  void HandleMove(const FInputActionValue &Value);

  /** Look input - applies yaw/pitch to controller rotation */
  UFUNCTION()
  void HandleLook(const FInputActionValue &Value);

  /** Jump started */
  UFUNCTION()
  void HandleJumpStarted(const FInputActionValue &Value);

  /** Jump completed */
  UFUNCTION()
  void HandleJumpCompleted(const FInputActionValue &Value);

  /** Sprint started */
  UFUNCTION()
  void HandleSprintStarted(const FInputActionValue &Value);

  /** Sprint completed */
  UFUNCTION()
  void HandleSprintCompleted(const FInputActionValue &Value);

  /** Interact pressed */
  UFUNCTION()
  void HandleInteract(const FInputActionValue &Value);

  /** Fire started */
  UFUNCTION()
  void HandleFireStarted(const FInputActionValue &Value);

  /** Fire completed */
  UFUNCTION()
  void HandleFireCompleted(const FInputActionValue &Value);

public:
  /** Show/hide mouse cursor and set appropriate input mode */
  UFUNCTION(BlueprintCallable, Category = "UI")
  void SetMouseCursorVisible(bool bVisible);
};
