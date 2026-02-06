// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "InteractableInterface.h"
#include "InteractableActor.generated.h"

class UStaticMeshComponent;
class ACharacter;
class UAnimMontage;

/**
 * Enum defining types of interactions for animation selection
 */
UENUM(BlueprintType)
enum class EInteractionType : uint8 {
  None UMETA(DisplayName = "None"),
  Pickup UMETA(DisplayName = "Pickup"),
  Sit UMETA(DisplayName = "Sit/Stand"),
  OpenDoor UMETA(DisplayName = "Open Door"),
  PullLever UMETA(DisplayName = "Pull Lever"),
  PressButton UMETA(DisplayName = "Press Button"),
  Custom UMETA(DisplayName = "Custom")
};

/**
 * Base class for interactable actors
 * Extend this in Blueprint to create custom interactable objects
 * Provides default implementation of InteractableInterface with Blueprint
 * events Supports custom animations for different interaction types
 */
UCLASS(Blueprintable)
class PROJECT_API AInteractableActor : public AActor,
                                          public IInteractableInterface {
  GENERATED_BODY()

public:
  // Constructor
  AInteractableActor();

protected:
  // Called when the game starts or when spawned
  virtual void BeginPlay() override;

public:
  // ========== IInteractableInterface Implementation ==========

  /**
   * Called when player starts looking at this object
   * Override in Blueprint for custom behavior
   */
  virtual void OnInteractionFocusBegin_Implementation(
      ACharacter *PlayerCharacter) override;

  /**
   * Called when player stops looking at this object
   * Override in Blueprint for custom behavior
   */
  virtual void OnInteractionFocusEnd_Implementation(
      ACharacter *PlayerCharacter) override;

  /**
   * Called when player interacts with this object
   * Override in Blueprint for custom interaction logic
   * @return true if interaction was successful
   */
  virtual bool
  OnInteract_Implementation(ACharacter *PlayerCharacter) override;

  /**
   * Get the name to display when player looks at this object
   */
  virtual FText GetInteractionName_Implementation() const override;

  /**
   * Get the action text to display (e.g., "Press E to open")
   */
  virtual FText GetInteractionAction_Implementation() const override;

  /**
   * Check if this object can currently be interacted with
   */
  virtual bool CanInteract_Implementation(
      ACharacter *PlayerCharacter) const override;

  /**
   * Get the maximum interaction distance
   */
  virtual float GetInteractionDistance_Implementation() const override;

protected:
  // ========== Animation System ==========

  /**
   * Play interaction animation on the character
   * @param PlayerCharacter - Character to play animation on
   * @return true if animation started successfully
   */
  UFUNCTION(BlueprintCallable, Category = "Interaction|Animation")
  bool PlayInteractionAnimation(ACharacter *PlayerCharacter);

  /**
   * Get the animation montage for current interaction type
   * @return Animation montage to play, or nullptr if none
   */
  UFUNCTION(BlueprintCallable, BlueprintPure,
            Category = "Interaction|Animation")
  UAnimMontage *GetInteractionAnimationMontage() const;

  /**
   * Called when interaction animation completes
   */
  UFUNCTION(BlueprintCallable, Category = "Interaction|Animation")
  void OnInteractionAnimationComplete(ACharacter *PlayerCharacter);

  /**
   * Blueprint event called when animation starts
   */
  UFUNCTION(BlueprintImplementableEvent, Category = "Interaction|Animation",
            meta = (DisplayName = "On Animation Start"))
  void BP_OnAnimationStart(ACharacter *PlayerCharacter);

  /**
   * Blueprint event called when animation completes
   */
  UFUNCTION(BlueprintImplementableEvent, Category = "Interaction|Animation",
            meta = (DisplayName = "On Animation Complete"))
  void BP_OnAnimationComplete(ACharacter *PlayerCharacter);

  // ========== Blueprint Events ==========

  /**
   * Blueprint event called when player starts looking at this object
   */
  UFUNCTION(BlueprintImplementableEvent, Category = "Interaction",
            meta = (DisplayName = "On Focus Begin"))
  void BP_OnInteractionFocusBegin(ACharacter *PlayerCharacter);

  /**
   * Blueprint event called when player stops looking at this object
   */
  UFUNCTION(BlueprintImplementableEvent, Category = "Interaction",
            meta = (DisplayName = "On Focus End"))
  void BP_OnInteractionFocusEnd(ACharacter *PlayerCharacter);

  /**
   * Blueprint event called when player interacts with this object
   * Return true if interaction was successful
   */
  UFUNCTION(BlueprintImplementableEvent, Category = "Interaction",
            meta = (DisplayName = "On Interact"))
  bool BP_OnInteract(ACharacter *PlayerCharacter);

protected:
  // ========== Components ==========

  /** Root scene component */
  UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
  USceneComponent *SceneRoot;

  /** Static mesh component (optional, for visual representation) */
  UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
  UStaticMeshComponent *MeshComponent;

  // ========== Interaction Settings ==========

  /** Display name for this interactable object */
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction")
  FText InteractionName = FText::FromString(TEXT("Interactable Object"));

  /** Action text to display (e.g., "Open", "Use", "Pickup") */
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction")
  FText InteractionAction = FText::FromString(TEXT("Interact"));

  /** Can this object currently be interacted with? */
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction")
  bool bCanInteract = true;

  /** Maximum distance from which this object can be interacted with (in cm) */
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction")
  float MaxInteractionDistance = 300.0f;

  /** Enable debug logging for this interactable */
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction|Debug")
  bool bEnableDebugLog = false;

  // ========== Animation Settings ==========

  /** Type of interaction for animation selection */
  UPROPERTY(EditAnywhere, BlueprintReadWrite,
            Category = "Interaction|Animation")
  EInteractionType InteractionType = EInteractionType::None;

  /** Should play animation on interaction? */
  UPROPERTY(EditAnywhere, BlueprintReadWrite,
            Category = "Interaction|Animation")
  bool bPlayAnimationOnInteract = false;

  /** Custom animation montage to play (overrides InteractionType if set) */
  UPROPERTY(EditAnywhere, BlueprintReadWrite,
            Category = "Interaction|Animation")
  UAnimMontage *CustomAnimationMontage = nullptr;

  /** Animation play rate multiplier */
  UPROPERTY(EditAnywhere, BlueprintReadWrite,
            Category = "Interaction|Animation",
            meta = (ClampMin = "0.1", ClampMax = "5.0"))
  float AnimationPlayRate = 1.0f;

  /** Should the interaction complete immediately or wait for animation? */
  UPROPERTY(EditAnywhere, BlueprintReadWrite,
            Category = "Interaction|Animation")
  bool bWaitForAnimationComplete = true;

  /** Section name to play in the animation montage (optional) */
  UPROPERTY(EditAnywhere, BlueprintReadWrite,
            Category = "Interaction|Animation")
  FName AnimationSectionName = NAME_None;

  // ========== Animation Montage References ==========

  /** Default pickup animation montage */
  UPROPERTY(EditDefaultsOnly, BlueprintReadOnly,
            Category = "Interaction|Animation|Defaults")
  UAnimMontage *PickupAnimationMontage = nullptr;

  /** Default sit/stand animation montage */
  UPROPERTY(EditDefaultsOnly, BlueprintReadOnly,
            Category = "Interaction|Animation|Defaults")
  UAnimMontage *SitAnimationMontage = nullptr;

  /** Default door opening animation montage */
  UPROPERTY(EditDefaultsOnly, BlueprintReadOnly,
            Category = "Interaction|Animation|Defaults")
  UAnimMontage *OpenDoorAnimationMontage = nullptr;

  /** Default lever pull animation montage */
  UPROPERTY(EditDefaultsOnly, BlueprintReadOnly,
            Category = "Interaction|Animation|Defaults")
  UAnimMontage *PullLeverAnimationMontage = nullptr;

  /** Default button press animation montage */
  UPROPERTY(EditDefaultsOnly, BlueprintReadOnly,
            Category = "Interaction|Animation|Defaults")
  UAnimMontage *PressButtonAnimationMontage = nullptr;

private:
  /** Cache reference to character currently playing animation */
  UPROPERTY()
  ACharacter *AnimatingCharacter = nullptr;

  /** Delegate handle for animation complete callback */
  FTimerHandle AnimationCompleteTimerHandle;
};
