#include "Save/SaveableActorComponent.h"

#include "Character/HealthComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Project.h"
#include "Save/ProjectSaveSubsystem.h"
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

  if (!SaveId.IsValid()) {
    UE_LOG(LogProject, Warning,
           TEXT("SaveableActorComponent: owner '%s' has no SaveId and will be ignored by the save system"),
           *GetNameSafe(GetOwner()));
  }

  if (UProjectSaveSubsystem *SaveSubsystem =
          GetWorld() && GetWorld()->GetGameInstance()
              ? GetWorld()->GetGameInstance()->GetSubsystem<UProjectSaveSubsystem>()
              : nullptr) {
    SaveSubsystem->ClearDestroyedActorRecord(GetOwnerMapName(), SaveId);
  }
}

void USaveableActorComponent::EndPlay(
    const EEndPlayReason::Type EndPlayReason) {
  if (CachedHealthComponent) {
    CachedHealthComponent->OnHealthChanged.RemoveDynamic(
        this, &USaveableActorComponent::HandleTrackedActorHealthChanged);
  }

  if (bTrackDestroyedState && SaveId.IsValid() &&
      EndPlayReason == EEndPlayReason::Destroyed) {
    FWorldActorSaveData SaveRecord;
    if (BuildSaveRecord(SaveRecord)) {
      SaveRecord.bDestroyedOrDead = true;
    } else {
      SaveRecord.SaveId = SaveId;
      SaveRecord.bDestroyedOrDead = true;
      SaveRecord.bHasHealthState = bLastKnownHasHealthState;
      SaveRecord.CurrentHealth = LastKnownHealth;
      CaptureCustomData(SaveRecord.CustomData);
    }

    if (UProjectSaveSubsystem *SaveSubsystem =
            GetWorld() && GetWorld()->GetGameInstance()
                ? GetWorld()->GetGameInstance()->GetSubsystem<UProjectSaveSubsystem>()
                : nullptr) {
      SaveSubsystem->RegisterDestroyedActorRecord(GetOwnerMapName(), SaveRecord);
    }
  }

  Super::EndPlay(EndPlayReason);
}

void USaveableActorComponent::RegenerateSaveId() { SaveId = FGuid::NewGuid(); }

bool USaveableActorComponent::BuildSaveRecord(
    FWorldActorSaveData &OutSaveRecord) const {
  if (!SaveId.IsValid() || !GetOwner()) {
    return false;
  }

  OutSaveRecord = FWorldActorSaveData();
  OutSaveRecord.SaveId = SaveId;
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

  CaptureCustomData(OutSaveRecord.CustomData);
  return !IsRecordAtDefaultState(OutSaveRecord);
}

void USaveableActorComponent::ApplySaveRecord(
    const FWorldActorSaveData &SaveRecord) {
  AActor *OwnerActor = GetOwner();
  if (!OwnerActor || SaveRecord.SaveId != SaveId) {
    return;
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
}

FString USaveableActorComponent::GetOwnerMapName() const {
  return GetWorld() ? UGameplayStatics::GetCurrentLevelName(GetWorld(), true)
                    : FString();
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
