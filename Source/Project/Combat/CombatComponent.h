// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Combat/WeaponLoadoutTypes.h"
#include "Components/ActorComponent.h"
#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "CombatComponent.generated.h"

class ACharacter;
class AWeaponBase;
class UCameraComponent;
class UInteractionComponent;
class USkeletalMeshComponent;
class UTexture2D;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_FourParams(
    FOnCurrentWeaponChanged, AWeaponBase *, PreviousWeapon, AWeaponBase *,
    NewWeapon, int32, NewSlotIndex, FGameplayTag, NewWeaponTypeTag);

UCLASS(ClassGroup = (Combat), meta = (BlueprintSpawnableComponent))
class PROJECT_API UCombatComponent : public UActorComponent {
  GENERATED_BODY()

public:
  UCombatComponent();

  virtual void BeginPlay() override;
  virtual void GetLifetimeReplicatedProps(
      TArray<FLifetimeProperty> &OutLifetimeProps) const override;

  UFUNCTION(BlueprintCallable, Category = "Combat|Loadout")
  bool InitializeLoadout();

  UFUNCTION(BlueprintCallable, Category = "Combat|Loadout")
  void ClearLoadout();

  UFUNCTION(BlueprintCallable, Category = "Combat|Loadout")
  bool EquipWeaponSlot(int32 SlotIndex);

  UFUNCTION(BlueprintCallable, Category = "Combat|Loadout")
  bool EquipNextWeapon();

  UFUNCTION(BlueprintCallable, Category = "Combat|Loadout")
  bool EquipPreviousWeapon();

  UFUNCTION(BlueprintCallable, Category = "Combat|Loadout")
  bool SetWeaponForSlot(EWeaponLoadoutSlot Slot,
                        TSubclassOf<AWeaponBase> WeaponClass);

  UFUNCTION(BlueprintCallable, Category = "Combat|Loadout")
  void ClearWeaponSlot(EWeaponLoadoutSlot Slot);

  UFUNCTION(BlueprintCallable, Category = "Combat|Loadout")
  bool SetActiveLoadoutSlot(EWeaponLoadoutSlot Slot);

  UFUNCTION(BlueprintCallable, Category = "Combat|Loadout")
  bool EquipActiveSlot();

  UFUNCTION(BlueprintPure, Category = "Combat|Loadout")
  AWeaponBase *GetWeaponInSlot(EWeaponLoadoutSlot Slot) const;

  UFUNCTION(BlueprintPure, Category = "Combat|Loadout")
  bool IsLoadoutSlotOccupied(EWeaponLoadoutSlot Slot) const;

  UFUNCTION(BlueprintCallable, Category = "Combat")
  bool EquipWeapon(TSubclassOf<AWeaponBase> WeaponClass);

  UFUNCTION(BlueprintCallable, Category = "Combat")
  bool EquipSpawnedWeapon(AWeaponBase *NewWeapon);

  UFUNCTION(BlueprintCallable, Category = "Combat")
  void UnequipCurrentWeapon(bool bDestroyWeapon = true);

  UFUNCTION(BlueprintCallable, Category = "Combat")
  void StartFire();

  UFUNCTION(BlueprintCallable, Category = "Combat")
  void StopFire();

  UFUNCTION(BlueprintCallable, Category = "Combat")
  bool Reload();

  UFUNCTION(BlueprintPure, Category = "Combat|Ammo")
  int32 GetAmmoInMagazine() const;

  UFUNCTION(BlueprintPure, Category = "Combat|Ammo")
  int32 GetAmmoInReserve() const;

  UFUNCTION(BlueprintPure, Category = "Combat|Ammo")
  int32 GetAmmoTotalAvailable() const;

  UFUNCTION(BlueprintCallable, Category = "Combat|Aim")
  void StartScope();

  UFUNCTION(BlueprintCallable, Category = "Combat|Aim")
  void StopScope();

  UFUNCTION(BlueprintPure, Category = "Combat|Aim")
  bool IsScoping() const { return bIsScoping; }

  // Legacy helper kept only so existing Blueprint assets load.
  // Always returns false until a new scoped HUD path is implemented.
  UFUNCTION(BlueprintPure, Category = "Combat|Aim|Scope")
  bool IsCurrentWeaponScopeType() const;

  // Legacy helper kept only so existing Blueprint assets load.
  // Always returns false until a new scoped HUD path is implemented.
  UFUNCTION(BlueprintPure, Category = "Combat|Aim|Scope")
  bool IsScopeOverlayActive() const;

  UFUNCTION(BlueprintPure, Category = "Combat|Aim|Scope")
  bool IsUsingPhysicalScope() const;

  UFUNCTION(BlueprintPure, Category = "Combat")
  AWeaponBase *GetCurrentWeapon() const { return CurrentWeapon; }

  UFUNCTION(BlueprintPure, Category = "Combat|Loadout")
  int32 GetCurrentWeaponSlotIndex() const { return CurrentWeaponSlotIndex; }

  UFUNCTION(BlueprintPure, Category = "Combat|Loadout")
  int32 GetLoadoutCount() const;

  UFUNCTION(BlueprintPure, Category = "Combat|Loadout")
  FGameplayTag GetCurrentWeaponTypeTag() const;

  UFUNCTION(BlueprintPure, Category = "Combat|UI")
  UTexture2D *GetCurrentWeaponIcon() const;

  UPROPERTY(BlueprintAssignable, Category = "Combat|Events")
  FOnCurrentWeaponChanged OnCurrentWeaponChanged;

protected:
  UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat|Loadout")
  TArray<TSubclassOf<AWeaponBase>> LoadoutWeaponClasses;

  UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat|Loadout")
  int32 InitialEquippedSlotIndex = 0;

  UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat|Loadout")
  TSubclassOf<AWeaponBase> StarterWeaponClass;

  UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat|Loadout")
  bool bSpawnStarterWeaponOnBeginPlay = true;

  UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat|Loadout")
  FName WeaponAttachSocketName = TEXT("hand_r");

  UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat|Loadout")
  FName PreferredFirstPersonMeshName = TEXT("FirstPersonMesh");

  UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat|Loadout")
  bool bPreferFirstPersonMesh = true;

  UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat|Input")
  bool bFireInteractsWithWidgets = true;

  UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat|Aim")
  bool bEnableScopeFov = true;

  UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat|Aim",
            meta = (EditCondition = "bEnableScopeFov"))
  bool bSmoothScopeFov = true;

  UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat|Aim",
            meta = (ClampMin = "1.0", ClampMax = "179.0",
                    EditCondition = "bEnableScopeFov"))
  float ScopedFieldOfView = 70.0f;

  UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat|Aim",
            meta = (ClampMin = "0.1", EditCondition = "bEnableScopeFov && bSmoothScopeFov"))
  float ScopeFovInterpolationSpeed = 18.0f;

  // Legacy property preserved for Blueprint compatibility. No longer used.
  UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat|Aim|Scope")
  FGameplayTag ScopeOverlayWeaponTypeTag;

private:
  UPROPERTY(Transient)
  TObjectPtr<ACharacter> OwningCharacter;

  UPROPERTY(Transient)
  TObjectPtr<UInteractionComponent> CachedInteractionComponent;

  UPROPERTY(Transient)
  TObjectPtr<UCameraComponent> CachedCameraComponent;

  UPROPERTY(Transient)
  TObjectPtr<USkeletalMeshComponent> CachedAttachMesh;

  UPROPERTY(Transient, BlueprintReadOnly, Category = "Combat",
            meta = (AllowPrivateAccess = "true"))
  TObjectPtr<AWeaponBase> CurrentWeapon;

  UPROPERTY(Transient, BlueprintReadOnly, Category = "Combat",
            meta = (AllowPrivateAccess = "true"))
  int32 CurrentWeaponSlotIndex = INDEX_NONE;

  UPROPERTY(Transient)
  TArray<TObjectPtr<AWeaponBase>> SpawnedLoadoutWeapons;

  UPROPERTY(ReplicatedUsing = OnRep_ReplicatedWeaponPresence, Transient)
  TArray<TSubclassOf<AWeaponBase>> ReplicatedLoadoutWeaponClasses;

  UPROPERTY(ReplicatedUsing = OnRep_ReplicatedWeaponPresence, Transient)
  int32 ReplicatedCurrentWeaponSlotIndex = INDEX_NONE;

  UPROPERTY(Transient)
  float DefaultFieldOfView = 0.0f;

  UPROPERTY(Transient)
  bool bIsScoping = false;

  UFUNCTION()
  void OnRep_ReplicatedWeaponPresence();

  void AttachWeaponToOwner(AWeaponBase *Weapon);
  void SetWeaponActiveState(AWeaponBase *Weapon, bool bShouldBeActive);
  void BroadcastCurrentWeaponChanged(AWeaponBase *PreviousWeapon,
                                     AWeaponBase *NewWeapon, int32 NewSlotIndex);
  void DestroyAllLoadoutWeapons();
  void UpdateReplicatedWeaponPresenceFromLocalState();
  void ApplyReplicatedWeaponPresence();
  void CacheOwnerReferences();
  void EnsureLoadoutArraySize();
  int32 CountOccupiedSlots() const;
  int32 FindNextOccupiedSlotIndex(int32 StartSlotIndex, int32 Direction) const;
  AWeaponBase *SpawnWeaponForSlot(int32 SlotIndex,
                                  TSubclassOf<AWeaponBase> WeaponClass);
  void UpdateCurrentWeaponAimState();
  UCameraComponent *ResolveCameraComponent() const;
  UInteractionComponent *ResolveInteractionComponent() const;
  USkeletalMeshComponent *ResolveAttachMesh() const;
};
