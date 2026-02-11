#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "WeaponBase.generated.h"

class USkeletalMeshComponent;

USTRUCT(BlueprintType)
struct FShotResultData
{
    GENERATED_BODY()

    UPROPERTY()
    FVector_NetQuantize TraceStart = FVector::ZeroVector;

    UPROPERTY()
    FVector_NetQuantize TraceEnd = FVector::ZeroVector;

    UPROPERTY()
    FVector_NetQuantize ImpactPoint = FVector::ZeroVector;

    UPROPERTY()
    FVector_NetQuantizeNormal ImpactNormal = FVector::ForwardVector;

    UPROPERTY()
    uint8 bBlockingHit : 1 = false;
};

UCLASS(Abstract, Blueprintable)
class PROJECT_API AWeaponBase : public AActor
{
    GENERATED_BODY()

public:
    AWeaponBase();

    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

    UFUNCTION(BlueprintCallable, Category="Weapon")
    virtual void StartFire();

    UFUNCTION(BlueprintCallable, Category="Weapon")
    virtual void StopFire();

    UFUNCTION(BlueprintPure, Category="Weapon")
    bool IsFiring() const { return bWantsToFire; }

    UFUNCTION(BlueprintPure, Category="Weapon")
    USkeletalMeshComponent* GetWeaponMesh() const { return WeaponMesh; }

protected:
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Weapon")
    TObjectPtr<USkeletalMeshComponent> WeaponMesh;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Weapon|Stats")
    float Damage = 20.0f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Weapon|Stats")
    float MaxRange = 20000.0f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Weapon|Stats", meta=(ClampMin="10.0"))
    float RoundsPerMinute = 600.0f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Weapon|Stats", meta=(ClampMin="0.0", ClampMax="20.0"))
    float SpreadAngleDeg = 0.6f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Weapon|Recoil")
    float RecoilPitch = 0.7f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Weapon|Recoil")
    float RecoilYaw = 0.25f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Weapon|Sockets")
    FName MuzzleSocketName = TEXT("Muzzle");

    UPROPERTY(Replicated, VisibleInstanceOnly, BlueprintReadOnly, Category="Weapon")
    bool bWantsToFire = false;

    UPROPERTY(Transient)
    FTimerHandle FireTimerHandle;

    UPROPERTY(Transient)
    float LastServerFireTime = -1000.0f;

    UFUNCTION(Server, Reliable)
    void ServerStartFire();

    UFUNCTION(Server, Reliable)
    void ServerStopFire();

    UFUNCTION(Server, Unreliable)
    void ServerFireOnce(const FVector_NetQuantize CamLocation, const FVector_NetQuantizeNormal CamDirection, int32 SpreadSeed);

    UFUNCTION(NetMulticast, Unreliable)
    void MulticastPlayFireFX(const FShotResultData& ShotResult);

    void SetFireTimerActive(bool bShouldFire);
    void FireOnce_LocalOrServer();
    void ProcessServerShot(const FVector& CamLocation, const FVector& CamDirection, int32 SpreadSeed);
    FShotResultData SimulateShot(const FVector& CamLocation, const FVector& CamDirection, int32 SpreadSeed, bool bApplyDamage);

    bool CanLocalFire() const;
    bool CanServerAcceptShot();

    FVector GetCameraLocation() const;
    FVector GetCameraDirection() const;
    FVector GetMuzzleLocation() const;
    FVector ApplySpread(const FVector& InDirection, int32 SpreadSeed) const;

    UFUNCTION(BlueprintImplementableEvent, Category="Weapon|FX")
    void BP_PlayFireFX(const FShotResultData& ShotResult, bool bPredictedLocal);

    UFUNCTION(BlueprintImplementableEvent, Category="Weapon|FX")
    void BP_PlayImpactFX(const FShotResultData& ShotResult, bool bPredictedLocal);
};
