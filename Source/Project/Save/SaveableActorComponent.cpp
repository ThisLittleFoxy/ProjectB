#include "Save/SaveableActorComponent.h"

#include "Character/HealthComponent.h"
#include "Project.h"
#include "Save/SaveableActorInterface.h"

USaveableActorComponent::USaveableActorComponent() {
  PrimaryComponentTick.bCanEverTick = false;
}

void USaveableActorComponent::BeginPlay() {
  Super::BeginPlay();

  CachedHealthComponent = GetOwner() ? GetOwner()->FindComponentByClass<UHealthComponent>()
                                     : nullptr;
  if (CachedHealthComponent) {
    CachedHealthComponent->OnHealthChanged.AddDynamic(
        this, &USaveableActorComponent::HandleTrackedActorHealthChanged);
    bLastKnownHasHealthState = true;
    LastKnownHealth = CachedHealthComponent->GetCurrentHealth();
  }

  CacheDefaultState();
}

void USaveableActorComponent::EndPlay(
    const EEndPlayReason::Type EndPlayReason) {
  if (CachedHealthComponent) {
    CachedHealthComponent->OnHealthChanged.RemoveDynamic(
        this, &USaveableActorComponent::HandleTrackedActorHealthChanged);
  }

  Super::EndPlay(EndPlayReason);
}

void USaveableActorComponent::RegeneratePersistentIdOverride() {
  SaveId = FGuid::NewGuid();
}

bool USaveableActorComponent::BuildSaveRecord(
    const FGuid &PersistentId, FWorldActorSaveData &OutSaveRecord) const {
  if (!PersistentId.IsValid() || !GetOwner()) {
    return false;
  }

  OutSaveRecord = FWorldActorSaveData();
  OutSaveRecord.PersistentId = PersistentId;
  OutSaveRecord.CustomData = FSaveableActorCustomData();

  if (bTrackHealthState) {
    const UHealthComponent *HealthComponent =
        CachedHealthComponent ? CachedHealthComponent.Get()
                              : GetOwner()->FindComponentByClass<UHealthComponent>();
    if (HealthComponent) {
      OutSaveRecord.bHasHealthState = true;
      OutSaveRecord.CurrentHealth = HealthComponent->GetCurrentHealth();
      OutSaveRecord.bDestroyedOrDead = OutSaveRecord.CurrentHealth <= 0.0f;
    } else if (bLastKnownHasHealthState) {
      OutSaveRecord.bHasHealthState = true;
      OutSaveRecord.CurrentHealth = LastKnownHealth;
      OutSaveRecord.bDestroyedOrDead = LastKnownHealth <= 0.0f;
    }
  }

  if (bTrackTransformState) {
    OutSaveRecord.bHasTransformState = true;
    OutSaveRecord.ActorTransform = GetOwner()->GetActorTransform();
  }

  CaptureCustomData(OutSaveRecord.CustomData);
  return !IsRecordAtDefaultState(OutSaveRecord);
}

void USaveableActorComponent::ApplySaveRecord(
    const FWorldActorSaveData &SaveRecord) {
  AActor *OwnerActor = GetOwner();
  if (!OwnerActor) {
    return;
  }

  if (SaveRecord.bHasTransformState) {
    OwnerActor->SetActorTransform(SaveRecord.ActorTransform, false, nullptr,
                                  ETeleportType::TeleportPhysics);
  }

  bool bHandledDestroyedStateViaHealth = false;
  if (SaveRecord.bHasHealthState) {
    if (!CachedHealthComponent) {
      CachedHealthComponent = OwnerActor->FindComponentByClass<UHealthComponent>();
    }

    if (CachedHealthComponent) {
      CachedHealthComponent->RestoreHealthFromSave(SaveRecord.CurrentHealth);
      bHandledDestroyedStateViaHealth =
          SaveRecord.bDestroyedOrDead && SaveRecord.CurrentHealth <= 0.0f;
    }
  }

  if (!IsValid(OwnerActor)) {
    return;
  }

  if (OwnerActor->GetClass()->ImplementsInterface(
          USaveableActorInterface::StaticClass())) {
    ISaveableActorInterface::Execute_ApplySaveCustomData(OwnerActor,
                                                         SaveRecord.CustomData);
  }

  if (SaveRecord.bDestroyedOrDead && bTrackDestroyedState &&
      !bHandledDestroyedStateViaHealth) {
    if (bDisableCollisionWhenRestoredAsDestroyed) {
      OwnerActor->SetActorEnableCollision(false);
    }

    if (bHideOwnerWhenRestoredAsDestroyed) {
      OwnerActor->SetActorHiddenInGame(true);
    }

    if (bDestroyOwnerWhenRestoredAsDestroyed) {
      OwnerActor->Destroy();
    }
  }
}

void USaveableActorComponent::CacheDefaultState() {
  if (bHasCachedDefaultState) {
    return;
  }

  bHasCachedDefaultState = true;
  DefaultCustomData = FSaveableActorCustomData();
  CaptureCustomData(DefaultCustomData);

  const UHealthComponent *HealthComponent =
      CachedHealthComponent ? CachedHealthComponent.Get()
                            : GetOwner() ? GetOwner()->FindComponentByClass<UHealthComponent>()
                                         : nullptr;
  if (HealthComponent && bTrackHealthState) {
    bDefaultHasHealthState = true;
    DefaultHealth = HealthComponent->GetCurrentHealth();
  } else {
    bDefaultHasHealthState = false;
    DefaultHealth = 0.0f;
  }

  if (GetOwner() && bTrackTransformState) {
    bDefaultHasTransformState = true;
    DefaultTransform = GetOwner()->GetActorTransform();
  } else {
    bDefaultHasTransformState = false;
    DefaultTransform = FTransform::Identity;
  }
}

void USaveableActorComponent::CaptureCustomData(
    FSaveableActorCustomData &OutCustomData) const {
  OutCustomData = FSaveableActorCustomData();

  const AActor *OwnerActor = GetOwner();
  if (!OwnerActor || !OwnerActor->GetClass()->ImplementsInterface(
                         USaveableActorInterface::StaticClass())) {
    return;
  }

  ISaveableActorInterface::Execute_GatherSaveCustomData(
      const_cast<AActor *>(OwnerActor), OutCustomData);
}

bool USaveableActorComponent::IsRecordAtDefaultState(
    const FWorldActorSaveData &SaveRecord) const {
  if (SaveRecord.bDestroyedOrDead) {
    return false;
  }

  if (SaveRecord.bHasHealthState != bDefaultHasHealthState) {
    return false;
  }

  if (SaveRecord.bHasHealthState &&
      !FMath::IsNearlyEqual(SaveRecord.CurrentHealth, DefaultHealth,
                            KINDA_SMALL_NUMBER)) {
    return false;
  }

  if (SaveRecord.bHasTransformState != bDefaultHasTransformState) {
    return false;
  }

  if (SaveRecord.bHasTransformState &&
      !SaveRecord.ActorTransform.Equals(DefaultTransform, KINDA_SMALL_NUMBER)) {
    return false;
  }

  return SaveRecord.CustomData.IsEquivalentTo(DefaultCustomData);
}

void USaveableActorComponent::HandleTrackedActorHealthChanged(
    UHealthComponent *HealthComponent, float CurrentHealth, float MaxHealth,
    float DeltaHealth) {
  bLastKnownHasHealthState = true;
  LastKnownHealth = CurrentHealth;
}

#if WITH_EDITOR
void USaveableActorComponent::PostEditChangeProperty(
    FPropertyChangedEvent &PropertyChangedEvent) {
  Super::PostEditChangeProperty(PropertyChangedEvent);

  if (!SaveId.IsValid() && !IsTemplate()) {
    SaveId = FGuid::NewGuid();
  }
}
#endif
