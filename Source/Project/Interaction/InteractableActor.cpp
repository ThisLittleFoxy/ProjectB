// Copyright Epic Games, Inc. All Rights Reserved.

#include "InteractableActor.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/Character.h"
#include "TimerManager.h"

AInteractableActor::AInteractableActor() {
  // Enable ticking if needed
  PrimaryActorTick.bCanEverTick = false;

  // Create root component
  SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
  RootComponent = SceneRoot;

  // Create mesh component (optional)
  MeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
  MeshComponent->SetupAttachment(RootComponent);
}

void AInteractableActor::BeginPlay() { Super::BeginPlay(); }

void AInteractableActor::OnInteractionFocusBegin_Implementation(
    ACharacter *PlayerCharacter) {
  if (bEnableDebugLog) {
    UE_LOG(LogTemp, Log, TEXT("%s: Player started looking at this object"),
           *GetName());
  }

  // Call Blueprint event
  BP_OnInteractionFocusBegin(PlayerCharacter);
}

void AInteractableActor::OnInteractionFocusEnd_Implementation(
    ACharacter *PlayerCharacter) {
  if (bEnableDebugLog) {
    UE_LOG(LogTemp, Log, TEXT("%s: Player stopped looking at this object"),
           *GetName());
  }

  // Call Blueprint event
  BP_OnInteractionFocusEnd(PlayerCharacter);
}

bool AInteractableActor::OnInteract_Implementation(
    ACharacter *PlayerCharacter) {
  if (bEnableDebugLog) {
    UE_LOG(LogTemp, Log, TEXT("%s: Player interacted with this object"),
           *GetName());
  }

  // Play animation if enabled
  if (bPlayAnimationOnInteract) {
    bool bAnimationStarted = PlayInteractionAnimation(PlayerCharacter);

    if (bAnimationStarted && bWaitForAnimationComplete) {
      // Animation will trigger BP_OnInteract when complete
      return true;
    }
  }

  // Call Blueprint event
  bool bSuccess = BP_OnInteract(PlayerCharacter);

  // Default behavior: print interaction message
  if (GEngine && bSuccess) {
    FString Message = FString::Printf(TEXT("Interacted with: %s"),
                                      *InteractionName.ToString());
    GEngine->AddOnScreenDebugMessage(-1, 2.0f, FColor::Green, Message);
  }

  return bSuccess;
}

FText AInteractableActor::GetInteractionName_Implementation() const {
  return InteractionName;
}

FText AInteractableActor::GetInteractionAction_Implementation() const {
  return InteractionAction;
}

bool AInteractableActor::CanInteract_Implementation(
    ACharacter *PlayerCharacter) const {
  return bCanInteract;
}

float AInteractableActor::GetInteractionDistance_Implementation() const {
  return MaxInteractionDistance;
}

// ========== Animation System Implementation ==========

bool AInteractableActor::PlayInteractionAnimation(
    ACharacter *PlayerCharacter) {
  if (!PlayerCharacter) {
    UE_LOG(LogTemp, Warning,
           TEXT("%s: Cannot play animation - PlayerCharacter is null"),
           *GetName());
    return false;
  }

  // Get the animation montage to play
  UAnimMontage *MontageToPlay = GetInteractionAnimationMontage();

  if (!MontageToPlay) {
    if (bEnableDebugLog) {
      UE_LOG(LogTemp, Warning,
             TEXT("%s: No animation montage assigned for interaction type"),
             *GetName());
    }
    return false;
  }

  // Get character's animation instance
  UAnimInstance *AnimInstance = PlayerCharacter->GetMesh()->GetAnimInstance();
  if (!AnimInstance) {
    UE_LOG(LogTemp, Warning, TEXT("%s: Cannot get AnimInstance from character"),
           *GetName());
    return false;
  }

  // Stop any currently playing montage
  if (AnimInstance->IsAnyMontagePlaying()) {
    AnimInstance->Montage_Stop(0.2f);
  }

  // Play the animation montage
  float MontageLength =
      AnimInstance->Montage_Play(MontageToPlay, AnimationPlayRate,
                                 EMontagePlayReturnType::MontageLength, 0.0f,
                                 true // Stop all montages
      );

  if (MontageLength > 0.0f) {
    // Jump to specific section if specified
    if (AnimationSectionName != NAME_None) {
      AnimInstance->Montage_JumpToSection(AnimationSectionName, MontageToPlay);
    }

    // Cache the character reference
    AnimatingCharacter = PlayerCharacter;

    // Call Blueprint event
    BP_OnAnimationStart(PlayerCharacter);

    if (bEnableDebugLog) {
      UE_LOG(LogTemp, Log,
             TEXT("%s: Playing animation montage '%s' (Duration: %.2fs)"),
             *GetName(), *MontageToPlay->GetName(), MontageLength);
    }

    // Setup animation complete callback if needed
    if (bWaitForAnimationComplete) {
      // Calculate actual playback duration accounting for play rate
      float ActualDuration = MontageLength / AnimationPlayRate;

      // Set timer to call completion callback
      GetWorld()->GetTimerManager().SetTimer(
          AnimationCompleteTimerHandle,
          [this, PlayerCharacter]() {
            OnInteractionAnimationComplete(PlayerCharacter);
          },
          ActualDuration, false);
    }

    return true;
  }

  if (bEnableDebugLog) {
    UE_LOG(LogTemp, Warning, TEXT("%s: Failed to play animation montage"),
           *GetName());
  }

  return false;
}

UAnimMontage *AInteractableActor::GetInteractionAnimationMontage() const {
  // Custom animation montage overrides everything
  if (CustomAnimationMontage) {
    return CustomAnimationMontage;
  }

  // Return appropriate default montage based on interaction type
  switch (InteractionType) {
  case EInteractionType::Pickup:
    return PickupAnimationMontage;

  case EInteractionType::Sit:
    return SitAnimationMontage;

  case EInteractionType::OpenDoor:
    return OpenDoorAnimationMontage;

  case EInteractionType::PullLever:
    return PullLeverAnimationMontage;

  case EInteractionType::PressButton:
    return PressButtonAnimationMontage;

  case EInteractionType::Custom:
    return CustomAnimationMontage;

  case EInteractionType::None:
  default:
    return nullptr;
  }
}

void AInteractableActor::OnInteractionAnimationComplete(
    ACharacter *PlayerCharacter) {
  if (bEnableDebugLog) {
    UE_LOG(LogTemp, Log, TEXT("%s: Animation completed"), *GetName());
  }

  // Clear cached character reference
  AnimatingCharacter = nullptr;

  // Clear timer handle
  GetWorld()->GetTimerManager().ClearTimer(AnimationCompleteTimerHandle);

  // Call Blueprint event
  BP_OnAnimationComplete(PlayerCharacter);

  // Now trigger the actual interaction logic
  if (PlayerCharacter) {
    BP_OnInteract(PlayerCharacter);
  }
}
