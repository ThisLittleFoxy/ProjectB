#pragma once

#include "Components/ActorComponent.h"
#include "Save/ProjectSaveTypes.h"
#include "SaveableActorComponent.generated.h"

class UHealthComponent;

UCLASS(ClassGroup = (Save), meta = (BlueprintSpawnableComponent))
class PROJECT_API USaveableActorComponent : public UActorComponent {
  GENERATED_BODY()

public:
  USaveableActorComponent();

  virtual void BeginPlay() override;
  virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

  UFUNCTION(BlueprintPure, Category = "Save")
  FGuid GetPersistentIdOverride() const { return SaveId; }

  UFUNCTION(BlueprintPure, Category = "Save")
  bool HasPersistentIdOverride() const { return SaveId.IsValid(); }

  UFUNCTION(BlueprintCallable, CallInEditor, Category = "Save")
  void RegeneratePersistentIdOverride();

  bool BuildSaveRecord(const FGuid &PersistentId,
                       FWorldActorSaveData &OutSaveRecord) const;
  void ApplySaveRecord(const FWorldActorSaveData &SaveRecord);
  bool ShouldTrackDestroyedState() const { return bTrackDestroyedState; }

protected:
  UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Save")
  FGuid SaveId;

  UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Save")
  bool bTrackHealthState = true;

  UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Save")
  bool bTrackDestroyedState = true;

  UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Save")
  bool bTrackTransformState = false;

  UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Save")
  bool bDestroyOwnerWhenRestoredAsDestroyed = true;

  UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Save")
  bool bHideOwnerWhenRestoredAsDestroyed = true;

  UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Save")
  bool bDisableCollisionWhenRestoredAsDestroyed = true;

#if WITH_EDITOR
  virtual void PostEditChangeProperty(
      FPropertyChangedEvent &PropertyChangedEvent) override;
#endif

private:
  UPROPERTY(Transient)
  TObjectPtr<UHealthComponent> CachedHealthComponent;

  UPROPERTY(Transient)
  bool bHasCachedDefaultState = false;

  UPROPERTY(Transient)
  bool bDefaultHasHealthState = false;

  UPROPERTY(Transient)
  float DefaultHealth = 0.0f;

  UPROPERTY(Transient)
  bool bDefaultHasTransformState = false;

  UPROPERTY(Transient)
  FTransform DefaultTransform = FTransform::Identity;

  UPROPERTY(Transient)
  FSaveableActorCustomData DefaultCustomData;

  UPROPERTY(Transient)
  bool bLastKnownHasHealthState = false;

  UPROPERTY(Transient)
  float LastKnownHealth = 0.0f;

  void CacheDefaultState();
  void CaptureCustomData(FSaveableActorCustomData &OutCustomData) const;
  bool IsRecordAtDefaultState(const FWorldActorSaveData &SaveRecord) const;

  UFUNCTION()
  void HandleTrackedActorHealthChanged(UHealthComponent *HealthComponent,
                                       float CurrentHealth, float MaxHealth,
                                       float DeltaHealth);
};
