// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "InteractableInterface.generated.h"

class ACharacter;

/**
 * Interface for objects that can be interacted with by the player
 * Implement this interface in Blueprint or C++ to create interactable objects
 */
UINTERFACE(MinimalAPI, BlueprintType)
class UInteractableInterface : public UInterface {
  GENERATED_BODY()
};

class IInteractableInterface {
  GENERATED_BODY()

public:
  /**
   * Called when player starts looking at the interactable object
   * Use this to show visual feedback (highlight, outline, etc.)
   * @param PlayerCharacter - The player character looking at this object
   */
  UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Interaction")
  void OnInteractionFocusBegin(ACharacter *PlayerCharacter);

  /**
   * Called when player stops looking at the interactable object
   * Use this to remove visual feedback
   * @param PlayerCharacter - The player character that was looking at this object
   */
  UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Interaction")
  void OnInteractionFocusEnd(ACharacter *PlayerCharacter);

  /**
   * Called when player presses the interact button while looking at this object
   * This is where you implement the actual interaction logic
   * @param PlayerCharacter - The player character interacting with this object
   * @return true if interaction was successful, false otherwise
   */
  UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Interaction")
  bool OnInteract(ACharacter *PlayerCharacter);

  /**
   * Get the name/description to display when player looks at this object
   * @return The display name of this interactable object
   */
  UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Interaction")
  FText GetInteractionName() const;

  /**
   * Get the action text to display (e.g., "Press E to open door")
   * @return The interaction prompt text
   */
  UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Interaction")
  FText GetInteractionAction() const;

  /**
   * Check if this object can currently be interacted with
   * @param PlayerCharacter - The player character attempting to interact
   * @return true if interaction is currently possible, false otherwise
   */
  UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Interaction")
  bool CanInteract(ACharacter *PlayerCharacter) const;

  /**
   * Get the maximum distance from which this object can be interacted with
   * @return Maximum interaction distance in cm
   */
  UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Interaction")
  float GetInteractionDistance() const;
};
