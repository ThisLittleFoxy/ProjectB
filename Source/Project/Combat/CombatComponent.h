// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Components/ActorComponent.h"
#include "CoreMinimal.h"
#include "CombatComponent.generated.h"

class ACharacter;
class AWeaponBase;
class UCameraComponent;
class UInteractionComponent;
class USkeletalMeshComponent;

/**
 * Component-driven combat setup:
 * - spawns starter weapon
 * - keeps it attached to character mesh/socket
 * - routes Fire press/release to weapon (or widget interaction)
 */
UCLASS(ClassGroup = (Combat), meta = (BlueprintSpawnableComponent))
class PROJECT_API UCombatComponent : public UActorComponent {
  GENERATED_BODY()

public:
  UCombatComponent();

  virtual void BeginPlay() override;

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

  /** Current magazine + reserve ammo */
  UFUNCTION(BlueprintPure, Category = "Combat|Ammo")
  int32 GetAmmoTotalAvailable() const;

  UFUNCTION(BlueprintCallable, Category = "Combat|Aim")
  void StartScope();

  UFUNCTION(BlueprintCallable, Category = "Combat|Aim")
  void StopScope();

  UFUNCTION(BlueprintPure, Category = "Combat|Aim")
  bool IsScoping() const { return bIsScoping; }

  UFUNCTION(BlueprintPure, Category = "Combat")
  AWeaponBase *GetCurrentWeapon() const { return CurrentWeapon; }

protected:
  // ========== Setup ==========

  UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat|Loadout")
  TSubclassOf<AWeaponBase> StarterWeaponClass;

  UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat|Loadout")
  bool bSpawnStarterWeaponOnBeginPlay = true;

  /**
   * Weapon attach point on selected skeletal mesh.
   * If socket is missing, UE will attach to mesh root.
   */
  UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat|Loadout")
  FName WeaponAttachSocketName = TEXT("hand_r");

  /**
   * Try to find this mesh first (useful for first-person setup).
   * Default name matches BP_Character component.
   */
  UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat|Loadout")
  FName PreferredFirstPersonMeshName = TEXT("FirstPersonMesh");

  UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat|Loadout")
  bool bPreferFirstPersonMesh = true;

  // ========== Input ==========

  /**
   * If true, fire input clicks UMG widgets when crosshair hovers a widget.
   * Gameplay shot is skipped for that press.
   */
  UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat|Input")
  bool bFireInteractsWithWidgets = true;

  // ========== Aim / Scope ==========

  UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat|Aim")
  bool bEnableScopeFov = true;

  UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat|Aim",
            meta = (ClampMin = "1.0", ClampMax = "179.0",
                    EditCondition = "bEnableScopeFov"))
  float ScopedFieldOfView = 70.0f;

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

  UPROPERTY(Transient)
  float DefaultFieldOfView = 0.0f;

  UPROPERTY(Transient)
  bool bIsScoping = false;

  void CacheOwnerReferences();
  UCameraComponent *ResolveCameraComponent() const;
  UInteractionComponent *ResolveInteractionComponent() const;
  USkeletalMeshComponent *ResolveAttachMesh() const;
};
