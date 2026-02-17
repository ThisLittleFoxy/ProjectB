// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "WeaponBase.generated.h"

class APawn;
class AController;
class USkeletalMeshComponent;
class UParticleSystem;
class USoundBase;

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

  UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon|Fire",
            meta = (ClampMin = "100.0"))
  float TraceDistance = 15000.0f;

  /** Small random cone (in degrees) for weapon spread */
  UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon|Fire",
            meta = (ClampMin = "0.0", ClampMax = "15.0"))
  float SpreadAngleDeg = 0.35f;

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
  FTimerHandle AutoFireTimerHandle;
  bool bIsTriggerHeld = false;

  float GetTimeBetweenShots() const;
  bool HasAmmo() const;
  bool ConsumeAmmo();
  void HandleAutoFireTick();
  AController *GetOwningController() const;
  FVector GetMuzzleLocation() const;
  bool MakeShotTrace(FHitResult &OutHit, FVector &OutTraceStart,
                     FVector &OutTraceEnd) const;
};
