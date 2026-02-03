// InteractionComponent.cpp

#include "InteractionComponent.h"
#include "Camera/CameraComponent.h"
#include "Components/WidgetInteractionComponent.h"
#include "DrawDebugHelpers.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "InteractableInterface.h"

UInteractionComponent::UInteractionComponent() {
  // Enable ticking for this component
  PrimaryComponentTick.bCanEverTick = true;
  PrimaryComponentTick.bStartWithTickEnabled = true;

  InteractionCheckTimer = 0.0f;
}

void UInteractionComponent::BeginPlay() {
  Super::BeginPlay();

  // Cache the owning character (works with any ACharacter, including BP)
  OwningCharacter = Cast<ACharacter>(GetOwner());
  if (!OwningCharacter) {
    UE_LOG(LogTemp, Warning,
           TEXT("UInteractionComponent: Owner is not a Character!"));
    return;
  }

  // Find camera component
  CachedCamera = FindCameraComponent();
  if (!CachedCamera) {
    UE_LOG(LogTemp, Warning,
           TEXT("UInteractionComponent: No CameraComponent found on owner!"));
  }

  // Create widget interaction component if enabled
  if (bEnableWidgetInteraction) {
    WidgetInteraction = NewObject<UWidgetInteractionComponent>(
        OwningCharacter, TEXT("WidgetInteraction"));
    if (WidgetInteraction) {
      WidgetInteraction->RegisterComponent();

      // Attach to camera if found
      if (CachedCamera) {
        WidgetInteraction->AttachToComponent(
            CachedCamera, FAttachmentTransformRules::SnapToTargetNotIncludingScale);
      }

      // Configure widget interaction
      WidgetInteraction->InteractionDistance = DefaultInteractionDistance;
      WidgetInteraction->bEnableHitTesting = true;
      WidgetInteraction->bShowDebug = bShowWidgetDebug;
      WidgetInteraction->InteractionSource =
          EWidgetInteractionSource::CenterScreen;

      UE_LOG(LogTemp, Log,
             TEXT("InteractionComponent: Widget interaction initialized"));
    }
  }
}

UCameraComponent *UInteractionComponent::FindCameraComponent() {
  if (!OwningCharacter) {
    return nullptr;
  }

  // Find any CameraComponent on the character
  return OwningCharacter->FindComponentByClass<UCameraComponent>();
}

void UInteractionComponent::TickComponent(
    float DeltaTime, ELevelTick TickType,
    FActorComponentTickFunction *ThisTickFunction) {
  Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

  // Widget interaction is handled automatically by UWidgetInteractionComponent
  // We only need to check for regular interactable objects

  // Don't check every frame for performance
  InteractionCheckTimer -= DeltaTime;
  if (InteractionCheckTimer > 0.0f) {
    return;
  }
  InteractionCheckTimer = InteractionCheckFrequency;

  // Skip regular interaction check if hovering over widget
  if (IsHoveringWidget()) {
    // Clear regular focused actor if hovering widget
    if (FocusedActor.IsValid()) {
      UpdateFocusedActor(nullptr);
    }
    return;
  }

  // Perform interaction trace for regular objects
  FHitResult HitResult;
  if (PerformInteractionTrace(HitResult)) {
    AActor *HitActor = HitResult.GetActor();
    if (HitActor && HitActor->Implements<UInteractableInterface>()) {
      // Check if we can interact with this object
      IInteractableInterface *Interactable =
          Cast<IInteractableInterface>(HitActor);
      if (Interactable &&
          Interactable->Execute_CanInteract(HitActor, OwningCharacter)) {
        // New valid interactable found
        if (HitActor != FocusedActor.Get()) {
          UpdateFocusedActor(HitActor);
        }
        return;
      }
    }
  }

  // No valid interactable found, clear focus
  if (FocusedActor.IsValid()) {
    UpdateFocusedActor(nullptr);
  }
}

bool UInteractionComponent::PerformInteractionTrace(FHitResult &OutHitResult) {
  if (!OwningCharacter) {
    return false;
  }

  // Get camera for trace start/direction
  if (!CachedCamera) {
    CachedCamera = FindCameraComponent();
    if (!CachedCamera) {
      return false;
    }
  }

  // Setup trace parameters
  FVector TraceStart = CachedCamera->GetComponentLocation();
  FVector TraceDirection = CachedCamera->GetForwardVector();
  FVector TraceEnd = TraceStart + (TraceDirection * DefaultInteractionDistance);

  // Setup collision query parameters
  FCollisionQueryParams QueryParams;
  QueryParams.AddIgnoredActor(OwningCharacter);
  QueryParams.bTraceComplex = false;

  // Perform line trace
  bool bHit = GetWorld()->LineTraceSingleByChannel(
      OutHitResult, TraceStart, TraceEnd, InteractionTraceChannel, QueryParams);

  // Debug visualization
  if (bShowDebugTrace) {
    FColor DebugColor = bHit ? FColor::Green : FColor::Red;
    DrawDebugLine(GetWorld(), TraceStart, TraceEnd, DebugColor, false,
                  DebugTraceDuration, 0, 2.0f);

    if (bHit) {
      DrawDebugPoint(GetWorld(), OutHitResult.ImpactPoint, 10.0f, DebugColor,
                     false, DebugTraceDuration);
    }
  }

  return bHit;
}

void UInteractionComponent::UpdateFocusedActor(AActor *NewFocusActor) {
  // Handle focus end on previous actor
  if (FocusedActor.IsValid() && FocusedActor.Get() != NewFocusActor) {
    IInteractableInterface *OldInteractable =
        Cast<IInteractableInterface>(FocusedActor.Get());
    if (OldInteractable) {
      OldInteractable->Execute_OnInteractionFocusEnd(FocusedActor.Get(),
                                                     OwningCharacter);
    }
  }

  // Update focused actor
  FocusedActor = NewFocusActor;

  // Handle focus begin on new actor
  if (FocusedActor.IsValid()) {
    IInteractableInterface *NewInteractable =
        Cast<IInteractableInterface>(FocusedActor.Get());
    if (NewInteractable) {
      NewInteractable->Execute_OnInteractionFocusBegin(FocusedActor.Get(),
                                                       OwningCharacter);
    }
  }
}

bool UInteractionComponent::TryInteract() {
  // TryInteract is now ONLY for regular objects (E key)
  // Widget interaction is handled separately via PressWidgetInteraction (LMB)

  if (!FocusedActor.IsValid() || !OwningCharacter) {
    return false;
  }

  IInteractableInterface *Interactable =
      Cast<IInteractableInterface>(FocusedActor.Get());
  if (!Interactable) {
    return false;
  }

  // Check if we can interact
  if (!Interactable->Execute_CanInteract(FocusedActor.Get(), OwningCharacter)) {
    return false;
  }

  // Execute interaction
  return Interactable->Execute_OnInteract(FocusedActor.Get(), OwningCharacter);
}


void UInteractionComponent::PressWidgetInteraction() {
  if (WidgetInteraction && IsHoveringWidget()) {
    WidgetInteraction->PressPointerKey(EKeys::LeftMouseButton);
    UE_LOG(LogTemp, Log, TEXT("InteractionComponent: Widget button pressed"));
  }
}

void UInteractionComponent::ReleaseWidgetInteraction() {
  if (WidgetInteraction) {
    WidgetInteraction->ReleasePointerKey(EKeys::LeftMouseButton);
    UE_LOG(LogTemp, Log, TEXT("InteractionComponent: Widget button released"));
  }
}

bool UInteractionComponent::IsHoveringWidget() const {
  if (!WidgetInteraction || !bEnableWidgetInteraction) {
    return false;
  }

  return WidgetInteraction->IsOverInteractableWidget();
}

FText UInteractionComponent::GetFocusedActorName() const {
  if (!FocusedActor.IsValid()) {
    return FText::GetEmpty();
  }

  IInteractableInterface *Interactable =
      Cast<IInteractableInterface>(FocusedActor.Get());
  if (!Interactable) {
    return FText::GetEmpty();
  }

  return Interactable->Execute_GetInteractionName(FocusedActor.Get());
}

FText UInteractionComponent::GetFocusedActorAction() const {
  if (!FocusedActor.IsValid()) {
    return FText::GetEmpty();
  }

  IInteractableInterface *Interactable =
      Cast<IInteractableInterface>(FocusedActor.Get());
  if (!Interactable) {
    return FText::GetEmpty();
  }

  return Interactable->Execute_GetInteractionAction(FocusedActor.Get());
}

bool UInteractionComponent::CanInteractWithFocusedActor() const {
  if (!FocusedActor.IsValid() || !OwningCharacter) {
    return false;
  }

  IInteractableInterface *Interactable =
      Cast<IInteractableInterface>(FocusedActor.Get());
  if (!Interactable) {
    return false;
  }

  return Interactable->Execute_CanInteract(FocusedActor.Get(), OwningCharacter);
}
