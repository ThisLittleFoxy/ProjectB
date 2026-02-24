// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Combat/HitZoneComponent.h"
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GameplayTagContainer.h"
#include "WeaponBase.generated.h"

class APawn;
class AController;
class APlayerController;
class USkeletalMeshComponent;
class UParticleSystem;
class USoundBase;
class UTexture2D;

UENUM(BlueprintType)
enum class EWeaponFireMode : uint8 {
  SemiAuto UMETA(DisplayName = "Semi Auto"),
  FullAuto UMETA(DisplayName = "Full Auto")
};

/**
 * Data-driven hitscan weapon.
 * Configure in a Blueprint child and use via UCombatComponent.
 */
UCLASS(Blueprintable)
class PROJECT_API AWeaponBase : public AActor {
  GENERATED_BODY()

public:
  AWeaponBase();

  virtual void BeginPlay() override;
  virtual void Tick(float DeltaSeconds) override;

  /** Sets owning pawn/controller context for traces and damage instigator */
  UFUNCTION(BlueprintCallable, Category = "Weapon")
  void SetOwningPawn(APawn *NewOwningPawn);

  /** Press trigger */
  UFUNCTION(BlueprintCallable, Category = "Weapon|Fire")
  void StartFire();

  /** Release trigger */
  UFUNCTION(BlueprintCallable, Category = "Weapon|Fire")
  void StopFire();

  /** Single shot (used for semi-auto and auto timer tick) */
  UFUNCTION(BlueprintCallable, Category = "Weapon|Fire")
  bool FireOnce();

  /** Simple instant reload from reserve */
  UFUNCTION(BlueprintCallable, Category = "Weapon|Ammo")
  bool Reload();

  UFUNCTION(BlueprintPure, Category = "Weapon|Ammo")
  int32 GetAmmoInMagazine() const { return CurrentAmmoInMagazine; }

  UFUNCTION(BlueprintPure, Category = "Weapon|Ammo")
  int32 GetReserveAmmo() const { return ReserveAmmo; }

  UFUNCTION(BlueprintPure, Category = "Weapon|Ammo")
  int32 GetMagazineSize() const { return MagazineSize; }

  UFUNCTION(BlueprintPure, Category = "Weapon|Fire")
  bool CanFire() const;

  UFUNCTION(BlueprintPure, Category = "Weapon")
  USkeletalMeshComponent *GetWeaponMesh() const { return WeaponMesh; }

  UFUNCTION(BlueprintCallable, Category = "Weapon|Aim")
  void SetAiming(bool bNewAiming);

  UFUNCTION(BlueprintPure, Category = "Weapon|Aim")
  bool IsAiming() const { return bIsAiming; }

  UFUNCTION(BlueprintPure, Category = "Weapon|Identity")
  FGameplayTag GetWeaponTypeTag() const { return WeaponTypeTag; }

  UFUNCTION(BlueprintPure, Category = "Weapon|Loadout")
  FName GetAttachSocketNameOverride() const { return AttachSocketNameOverride; }

  UFUNCTION(BlueprintPure, Category = "Weapon|UI")
  UTexture2D *GetWeaponIcon() const { return WeaponIcon; }

  UFUNCTION(BlueprintPure, Category = "Weapon|Damage")
  float GetDamageMultiplierForZone(EHitZone Zone) const;

  /** Called after each successful shot recoil calculation (for BP visual effects) */
  UFUNCTION(BlueprintImplementableEvent, Category = "Weapon|Recoil")
  void BP_OnRecoilApplied(float RecoilYawDeg, float RecoilPitchDeg);

protected:
  UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Weapon")
  TObjectPtr<USkeletalMeshComponent> WeaponMesh;

  // ========== Fire ==========

  UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon|Fire")
  EWeaponFireMode FireMode = EWeaponFireMode::FullAuto;

  /** Rounds per minute */
  UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon|Fire",
            meta = (ClampMin = "1.0"))
  float FireRateRpm = 600.0f;

  UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon|Fire",
            meta = (ClampMin = "0.0"))
  float Damage = 20.0f;

  UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon|Identity")
  FGameplayTag WeaponTypeTag;

  UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon|Loadout")
  FName AttachSocketNameOverride = NAME_None;

  UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon|UI")
  TObjectPtr<UTexture2D> WeaponIcon;

  UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon|Damage")
  TMap<EHitZone, float> DamageMultiplierByZone;

  UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon|Damage",
            meta = (ClampMin = "0.0"))
  float DefaultZoneDamageMultiplier = 1.0f;

  UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon|Fire",
            meta = (ClampMin = "100.0"))
  float TraceDistance = 15000.0f;

  /** Small random cone (in degrees) for weapon spread */
  UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon|Fire",
            meta = (ClampMin = "0.0", ClampMax = "15.0"))
  float SpreadAngleDeg = 0.35f;

  /** Multiplier applied to spread while aiming (scope) */
  UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon|Aim",
            meta = (ClampMin = "0.01", ClampMax = "2.0"))
  float AimingSpreadMultiplier = 0.5f;

  // ========== Recoil / Spray ==========

  /**
   * Spray pattern in degrees per shot.
   * X = Yaw (left/right), Y = Pitch (up/down).
   */
  UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon|Recoil")
  TArray<FVector2D> SprayPattern;

  /**
   * If true, pattern loops when shot index exceeds array size.
   * If false, weapon keeps using the last pattern point.
   */
  UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon|Recoil")
  bool bLoopSprayPattern = false;

  /** Extra random yaw jitter (degrees) added on top of spray pattern */
  UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon|Recoil",
            meta = (ClampMin = "0.0", ClampMax = "10.0"))
  float RandomRecoilYawDeg = 0.08f;

  /** Extra random pitch jitter (degrees) added on top of spray pattern */
  UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon|Recoil",
            meta = (ClampMin = "0.0", ClampMax = "10.0"))
  float RandomRecoilPitchDeg = 0.08f;

  /** Random recoil multiplier while aiming (scope) */
  UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon|Recoil",
            meta = (ClampMin = "0.0", ClampMax = "1.0"))
  float AimingRecoilRandomMultiplier = 0.4f;

  /** If true, player camera receives recoil kick each shot */
  UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon|Recoil")
  bool bApplyCameraRecoil = true;

  /** Camera pitch kick multiplier from recoil pattern/jitter */
  UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon|Recoil",
            meta = (ClampMin = "0.0", ClampMax = "5.0",
                    EditCondition = "bApplyCameraRecoil"))
  float CameraRecoilPitchMultiplier = 1.0f;

  /** Camera yaw kick multiplier from recoil pattern/jitter */
  UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon|Recoil",
            meta = (ClampMin = "0.0", ClampMax = "5.0",
                    EditCondition = "bApplyCameraRecoil"))
  float CameraRecoilYawMultiplier = 1.0f;

  /** Additional camera recoil reduction while aiming (scope) */
  UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon|Recoil",
            meta = (ClampMin = "0.0", ClampMax = "1.0",
                    EditCondition = "bApplyCameraRecoil"))
  float AimingCameraRecoilMultiplier = 0.65f;

  /**
   * How quickly camera approaches recoil target (kick smoothness).
   * Higher = snappier kick.
   */
  UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon|Recoil",
            meta = (ClampMin = "0.0", EditCondition = "bApplyCameraRecoil"))
  float CameraRecoilKickInterpSpeed = 24.0f;

  /**
   * How quickly recoil target returns back to zero.
   * Higher = faster recovery.
   */
  UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon|Recoil",
            meta = (ClampMin = "0.0", EditCondition = "bApplyCameraRecoil"))
  float CameraRecoilReturnSpeed = 4.0f;

  /**
   * If false, recoil does not return while trigger is held (more persistent spray feel).
   * If true, recoil slowly returns even during continuous fire.
   */
  UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon|Recoil",
            meta = (EditCondition = "bApplyCameraRecoil"))
  bool bReturnCameraRecoilWhileFiring = false;

  /** Maximum accumulated pitch recoil offset (degrees) */
  UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon|Recoil",
            meta = (ClampMin = "0.0", EditCondition = "bApplyCameraRecoil"))
  float MaxAccumulatedCameraPitchRecoil = 12.0f;

  /** Maximum accumulated yaw recoil offset (degrees) */
  UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon|Recoil",
            meta = (ClampMin = "0.0", EditCondition = "bApplyCameraRecoil"))
  float MaxAccumulatedCameraYawRecoil = 8.0f;

  /** Reset spray shot index if we did not fire for this time (seconds) */
  UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon|Recoil",
            meta = (ClampMin = "0.0"))
  float SprayResetDelay = 0.3f;

  UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon|Fire")
  TEnumAsByte<ECollisionChannel> TraceChannel = ECC_Visibility;

  // ========== Ammo ==========

  UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon|Ammo",
            meta = (ClampMin = "1"))
  int32 MagazineSize = 30;

  UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon|Ammo",
            meta = (ClampMin = "0"))
  int32 CurrentAmmoInMagazine = 30;

  UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon|Ammo",
            meta = (ClampMin = "0"))
  int32 ReserveAmmo = 90;

  UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon|Ammo")
  bool bInfiniteAmmo = false;

  // ========== FX ==========

  UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon|FX")
  FName MuzzleSocketName = TEXT("Muzzle");

  UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon|FX")
  TObjectPtr<UParticleSystem> MuzzleEffect;

  UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon|FX")
  TObjectPtr<UParticleSystem> ImpactEffect;

  UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon|FX")
  TObjectPtr<USoundBase> FireSound;

  UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon|FX")
  TObjectPtr<USoundBase> DryFireSound;

  UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon|Debug")
  bool bDrawDebugTrace = false;

  UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon|Debug",
            meta = (ClampMin = "0.0", EditCondition = "bDrawDebugTrace"))
  float DebugTraceDuration = 1.0f;

private:
  TWeakObjectPtr<APawn> OwningPawn;
  TWeakObjectPtr<APlayerController> CachedLocalPlayerController;
  FTimerHandle AutoFireTimerHandle;
  bool bIsTriggerHeld = false;
  bool bIsAiming = false;
  int32 BurstShotIndex = 0;
  float LastShotTimeSeconds = -1000.0f;
  FVector2D CameraRecoilTargetOffsetDeg = FVector2D::ZeroVector;
  FVector2D CameraRecoilAppliedOffsetDeg = FVector2D::ZeroVector;
  FRotator LastControlRotationBeforeRecoil = FRotator::ZeroRotator;
  bool bHasLastControlRotationBeforeRecoil = false;

  float GetTimeBetweenShots() const;
  bool HasAmmo() const;
  bool ConsumeAmmo();
  void HandleAutoFireTick();
  FVector2D BuildRecoilOffsetForShot();
  FVector2D GetPatternOffsetForShot(int32 ShotIndex) const;
  void ApplyCameraRecoil(const FVector2D &RecoilOffsetDeg);
  APlayerController *ResolveLocalPlayerController();
  AController *GetOwningController() const;
  FVector GetMuzzleLocation() const;
  bool MakeShotTrace(FHitResult &OutHit, FVector &OutTraceStart,
                     FVector &OutTraceEnd,
                     const FVector2D &RecoilOffsetDeg) const;
};
