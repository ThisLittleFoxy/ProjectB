#include "Combat/WeaponBase.h"

#include "Components/SkeletalMeshComponent.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "GameFramework/Controller.h"
#include "Kismet/GameplayStatics.h"
#include "Net/UnrealNetwork.h"
#include "TimerManager.h"

AWeaponBase::AWeaponBase()
{
    PrimaryActorTick.bCanEverTick = false;
    bReplicates = true;

    WeaponMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("WeaponMesh"));
    SetRootComponent(WeaponMesh);

    WeaponMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

void AWeaponBase::BeginPlay()
{
    Super::BeginPlay();
}

void AWeaponBase::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    SetFireTimerActive(false);
    Super::EndPlay(EndPlayReason);
}


void AWeaponBase::StartFire()
{
    if (!CanLocalFire())
    {
        return;
    }

    bWantsToFire = true;
    SetFireTimerActive(true);

    if (!HasAuthority())
    {
        ServerStartFire();
    }
}

void AWeaponBase::StopFire()
{
    bWantsToFire = false;
    SetFireTimerActive(false);

    if (!HasAuthority())
    {
        ServerStopFire();
    }
}

void AWeaponBase::SetFireTimerActive(bool bShouldFire)
{
    if (!GetWorld())
    {
        return;
    }

    if (!bShouldFire)
    {
        GetWorldTimerManager().ClearTimer(FireTimerHandle);
        return;
    }

    const float Interval = 60.0f / FMath::Max(RoundsPerMinute, 1.0f);

    if (!GetWorldTimerManager().IsTimerActive(FireTimerHandle))
    {
        GetWorldTimerManager().SetTimer(FireTimerHandle, this, &AWeaponBase::FireOnce_LocalOrServer, Interval, true, 0.0f);
    }
}

void AWeaponBase::FireOnce_LocalOrServer()
{
    if (!bWantsToFire || !CanLocalFire())
    {
        return;
    }

    const FVector CamLocation = GetCameraLocation();
    const FVector CamDirection = GetCameraDirection();
    const int32 SpreadSeed = FMath::Rand();

    // Local cosmetic prediction for the owning player.
    const FShotResultData PredictedShot = SimulateShot(CamLocation, CamDirection, SpreadSeed, false);
    BP_PlayFireFX(PredictedShot, true);
    if (PredictedShot.bBlockingHit)
    {
        BP_PlayImpactFX(PredictedShot, true);
    }

    if (HasAuthority())
    {
        ProcessServerShot(CamLocation, CamDirection, SpreadSeed);
    }
    else
    {
        ServerFireOnce(CamLocation, CamDirection, SpreadSeed);
    }
}

void AWeaponBase::ProcessServerShot(const FVector& CamLocation, const FVector& CamDirection, int32 SpreadSeed)
{
    const FShotResultData Result = SimulateShot(CamLocation, CamDirection, SpreadSeed, true);
    MulticastPlayFireFX(Result);
}

FShotResultData AWeaponBase::SimulateShot(const FVector& CamLocation, const FVector& CamDirection, int32 SpreadSeed, bool bApplyDamage)
{
    FShotResultData OutResult;

    UWorld* World = GetWorld();
    APawn* OwnerPawn = Cast<APawn>(GetOwner());
    if (!World || !OwnerPawn)
    {
        return OutResult;
    }

    FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(WeaponTrace), false, this);
    QueryParams.AddIgnoredActor(this);
    QueryParams.AddIgnoredActor(OwnerPawn);

    const FVector SpreadDirection = ApplySpread(CamDirection, SpreadSeed);
    const FVector AimTraceEnd = CamLocation + SpreadDirection * MaxRange;

    FHitResult AimHit;
    World->LineTraceSingleByChannel(AimHit, CamLocation, AimTraceEnd, ECC_Visibility, QueryParams);
    const FVector AimPoint = AimHit.bBlockingHit ? AimHit.ImpactPoint : AimTraceEnd;

    const FVector MuzzleLocation = GetMuzzleLocation();
    const FVector MuzzleDirection = (AimPoint - MuzzleLocation).GetSafeNormal();
    const FVector MuzzleTraceEnd = MuzzleLocation + MuzzleDirection * MaxRange;

    FHitResult HitResult;
    World->LineTraceSingleByChannel(HitResult, MuzzleLocation, MuzzleTraceEnd, ECC_Visibility, QueryParams);

    OutResult.TraceStart = MuzzleLocation;
    OutResult.TraceEnd = HitResult.bBlockingHit ? HitResult.ImpactPoint : MuzzleTraceEnd;
    OutResult.bBlockingHit = HitResult.bBlockingHit;

    if (HitResult.bBlockingHit)
    {
        OutResult.ImpactPoint = HitResult.ImpactPoint;
        OutResult.ImpactNormal = HitResult.ImpactNormal;

        if (bApplyDamage && HasAuthority())
        {
            AController* InstigatorController = OwnerPawn->GetController();
            UGameplayStatics::ApplyPointDamage(HitResult.GetActor(), Damage, MuzzleDirection, HitResult,
                InstigatorController, this, UDamageType::StaticClass());
        }
    }

    return OutResult;
}

void AWeaponBase::ServerStartFire_Implementation()
{
    if (!CanLocalFire())
    {
        return;
    }

    bWantsToFire = true;
    SetFireTimerActive(true);
}

void AWeaponBase::ServerStopFire_Implementation()
{
    bWantsToFire = false;
    SetFireTimerActive(false);
}

void AWeaponBase::ServerFireOnce_Implementation(const FVector_NetQuantize CamLocation,
    const FVector_NetQuantizeNormal CamDirection, int32 SpreadSeed)
{
    if (!bWantsToFire || !CanServerAcceptShot())
    {
        return;
    }

    ProcessServerShot(CamLocation, CamDirection, SpreadSeed);
}

void AWeaponBase::MulticastPlayFireFX_Implementation(const FShotResultData& ShotResult)
{
    const APawn* OwnerPawn = Cast<APawn>(GetOwner());
    if (OwnerPawn && OwnerPawn->IsLocallyControlled())
    {
        return;
    }

    BP_PlayFireFX(ShotResult, false);
    if (ShotResult.bBlockingHit)
    {
        BP_PlayImpactFX(ShotResult, false);
    }
}

bool AWeaponBase::CanLocalFire() const
{
    const APawn* OwnerPawn = Cast<APawn>(GetOwner());
    return OwnerPawn && OwnerPawn->GetController() != nullptr;
}

bool AWeaponBase::CanServerAcceptShot() const
{
    if (!HasAuthority() || !GetWorld())
    {
        return false;
    }

    const float MinInterval = 60.0f / FMath::Max(RoundsPerMinute, 1.0f);
    const float CurrentTime = GetWorld()->GetTimeSeconds();

    if ((CurrentTime - LastServerFireTime) < (MinInterval * 0.9f))
    {
        return false;
    }

    LastServerFireTime = CurrentTime;
    return true;
}

FVector AWeaponBase::GetCameraLocation() const
{
    const APawn* OwnerPawn = Cast<APawn>(GetOwner());
    if (!OwnerPawn)
    {
        return GetActorLocation();
    }

    FVector Location = FVector::ZeroVector;
    FRotator Rotation = FRotator::ZeroRotator;
    OwnerPawn->GetActorEyesViewPoint(Location, Rotation);

    return Location;
}

FVector AWeaponBase::GetCameraDirection() const
{
    const APawn* OwnerPawn = Cast<APawn>(GetOwner());
    if (!OwnerPawn)
    {
        return GetActorForwardVector();
    }

    FVector Location = FVector::ZeroVector;
    FRotator Rotation = FRotator::ZeroRotator;
    OwnerPawn->GetActorEyesViewPoint(Location, Rotation);

    return Rotation.Vector();
}

FVector AWeaponBase::GetMuzzleLocation() const
{
    if (WeaponMesh && WeaponMesh->DoesSocketExist(MuzzleSocketName))
    {
        return WeaponMesh->GetSocketLocation(MuzzleSocketName);
    }

    return GetActorLocation();
}

FVector AWeaponBase::ApplySpread(const FVector& InDirection, int32 SpreadSeed) const
{
    FRandomStream Stream(SpreadSeed);
    return Stream.VRandCone(InDirection, FMath::DegreesToRadians(SpreadAngleDeg));
}

void AWeaponBase::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    DOREPLIFETIME(AWeaponBase, bWantsToFire);
}
