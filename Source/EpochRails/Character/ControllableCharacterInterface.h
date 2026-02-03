// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "ControllableCharacterInterface.generated.h"

/**
 * Interface for characters that can be controlled by RailsPlayerController.
 * Implement this interface in your Blueprint Character to receive input from the controller.
 *
 * For FPS movement, the controller handles directional calculation using GetControlRotation(),
 * so your character just needs to call AddMovementInput with the provided direction.
 */
UINTERFACE(MinimalAPI, BlueprintType)
class UControllableCharacterInterface : public UInterface {
  GENERATED_BODY()
};

class IControllableCharacterInterface {
  GENERATED_BODY()

public:
  // ========== Movement ==========

  /**
   * Called when player provides movement input.
   * The controller calculates world direction based on camera rotation.
   * @param WorldDirection - The direction to move in world space (already calculated from camera)
   * @param ScaleValue - The input magnitude (0 to 1)
   */
  UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Input|Movement")
  void HandleMovementInput(const FVector& WorldDirection, float ScaleValue);

  // ========== Sprint ==========

  /**
   * Called when sprint button is pressed
   */
  UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Input|Movement")
  void StartSprint();

  /**
   * Called when sprint button is released
   */
  UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Input|Movement")
  void StopSprint();

  // ========== Interaction ==========

  /**
   * Called when interact button is pressed
   */
  UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Input|Interaction")
  void DoInteract();

  // ========== Combat ==========

  /**
   * Called when fire button is pressed
   */
  UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Input|Combat")
  void StartFire();

  /**
   * Called when fire button is released
   */
  UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Input|Combat")
  void StopFire();
};
