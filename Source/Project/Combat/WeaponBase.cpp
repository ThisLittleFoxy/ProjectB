// Copyright Epic Games, Inc. All Rights Reserved.

#include "Combat/WeaponBase.h"
#include "Components/SkeletalMeshComponent.h"
#include "DrawDebugHelpers.h"
#include "Engine/World.h"
#include "GameFramework/Controller.h"
#include "GameFramework/DamageType.h"
#include "GameFramework/Pawn.h"
#include "Kismet/GameplayStatics.h"
#include "Particles/ParticleSystem.h"
#include "Project.h"
#include "Sound/SoundBase.h"
#include "TimerManager.h"
#include "Utils/AimTraceService.h"

AWeaponBase::AWeaponBase() {
  PrimaryActorTick.bCanEverTick = false;

  WeaponMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("WeaponMesh"));
  SetRootComponent(WeaponMesh);

  WeaponMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
  WeaponMesh->SetGenerateOverlapEvents(false);
}

void AWeaponBase::BeginPlay() {
  Super::BeginPlay();

  if (!OwningPawn.IsValid()) {
    if (APawn *OwnerAsPawn = Cast<APawn>(GetOwner())) {
      SetOwningPawn(OwnerAsPawn);
    }
  }

  CurrentAmmoInMagazine = FMath::Clamp(CurrentAmmoInMagazine, 0, MagazineSize);
  ReserveAmmo = FMath::Max(ReserveAmmo, 0);
}

void AWeaponBase::SetOwningPawn(APawn *NewOwningPawn) {
  OwningPawn = NewOwningPawn;
  SetOwner(NewOwningPawn);
  SetInstigator(NewOwningPawn);
}

bool AWeaponBase::CanFire() const {
  return bInfiniteAmmo || CurrentAmmoInMagazine > 0;
}

void AWeaponBase::StartFire() {
  bIsTriggerHeld = true;

  if (FireMode == EWeaponFireMode::SemiAuto) {
    FireOnce();
    return;
  }

  FireOnce();

  if (!GetWorld()) {
    return;
  }

  const float TimeBetweenShots = GetTimeBetweenShots();
  GetWorld()->GetTimerManager().SetTimer(AutoFireTimerHandle, this,
                                          &AWeaponBase::HandleAutoFireTick,
                                          TimeBetweenShots, true,
                                          TimeBetweenShots);
}

void AWeaponBase::StopFire() {
  bIsTriggerHeld = false;

  if (GetWorld()) {
    GetWorld()->GetTimerManager().ClearTimer(AutoFireTimerHandle);
  }
}

bool AWeaponBase::FireOnce() {
  if (!CanFire()) {
    if (DryFireSound) {
      UGameplayStatics::PlaySoundAtLocation(this, DryFireSound,
                                            GetActorLocation());
    }
    return false;
  }

  FHitResult HitResult;
  FVector TraceStart = FVector::ZeroVector;
  FVector TraceEnd = FVector::ZeroVector;

  const bool bHit = MakeShotTrace(HitResult, TraceStart, TraceEnd);

  if (FireSound) {
    UGameplayStatics::PlaySoundAtLocation(this, FireSound, GetMuzzleLocation());
  }

  if (MuzzleEffect) {
    UGameplayStatics::SpawnEmitterAtLocation(
        GetWorld(), MuzzleEffect, GetMuzzleLocation(), GetActorRotation(), true);
  }

  if (bHit) {
    if (AActor *HitActor = HitResult.GetActor()) {
      const FVector ShotDirection = (TraceEnd - TraceStart).GetSafeNormal();
      UGameplayStatics::ApplyPointDamage(HitActor, Damage, ShotDirection,
                                         HitResult, GetOwningController(), this,
                                         UDamageType::StaticClass());
    }

    if (ImpactEffect) {
      UGameplayStatics::SpawnEmitterAtLocation(
          GetWorld(), ImpactEffect, HitResult.ImpactPoint,
          HitResult.ImpactNormal.Rotation(), true);
    }
  }

  ConsumeAmmo();
  return true;
}

bool AWeaponBase::Reload() {
  if (bInfiniteAmmo || CurrentAmmoInMagazine >= MagazineSize || ReserveAmmo <= 0) {
    return false;
  }

  const int32 NeededAmmo = MagazineSize - CurrentAmmoInMagazine;
  const int32 AmmoToLoad = FMath::Min(NeededAmmo, ReserveAmmo);

  CurrentAmmoInMagazine += AmmoToLoad;
  ReserveAmmo -= AmmoToLoad;
  return AmmoToLoad > 0;
}

float AWeaponBase::GetTimeBetweenShots() const {
  return 60.0f / FMath::Max(FireRateRpm, 1.0f);
}

bool AWeaponBase::HasAmmo() const {
  return bInfiniteAmmo || CurrentAmmoInMagazine > 0;
}

bool AWeaponBase::ConsumeAmmo() {
  if (bInfiniteAmmo) {
    return true;
  }

  if (CurrentAmmoInMagazine <= 0) {
    return false;
  }

  --CurrentAmmoInMagazine;
  return true;
}

void AWeaponBase::HandleAutoFireTick() {
  if (!bIsTriggerHeld) {
    StopFire();
    return;
  }

  if (!FireOnce()) {
    StopFire();
  }
}

AController *AWeaponBase::GetOwningController() const {
  if (OwningPawn.IsValid()) {
    return OwningPawn->GetController();
  }

  if (const APawn *OwnerPawn = Cast<APawn>(GetOwner())) {
    return OwnerPawn->GetController();
  }

  return nullptr;
}

FVector AWeaponBase::GetMuzzleLocation() const {
  if (WeaponMesh && WeaponMesh->DoesSocketExist(MuzzleSocketName)) {
    return WeaponMesh->GetSocketLocation(MuzzleSocketName);
  }

  return GetActorLocation();
}

bool AWeaponBase::MakeShotTrace(FHitResult &OutHit, FVector &OutTraceStart,
                                FVector &OutTraceEnd) const {
  UWorld *World = GetWorld();
  if (!World) {
    return false;
  }

  FVector ViewStart = GetActorLocation();
  FVector ViewEnd = ViewStart + GetActorForwardVector() * TraceDistance;

  if (AController *Controller = GetOwningController()) {
    FVector NewViewStart = FVector::ZeroVector;
    FVector NewViewEnd = FVector::ZeroVector;
    if (FAimTraceService::MakeViewPointRay(Controller, TraceDistance, NewViewStart,
                                           NewViewEnd)) {
      ViewStart = NewViewStart;
      ViewEnd = NewViewEnd;
    }
  }

  FVector ShotDirection = (ViewEnd - ViewStart).GetSafeNormal();
  if (SpreadAngleDeg > KINDA_SMALL_NUMBER) {
    const float ConeHalfAngle = FMath::DegreesToRadians(SpreadAngleDeg);
    ShotDirection = FMath::VRandCone(ShotDirection, ConeHalfAngle);
  }
  ViewEnd = ViewStart + ShotDirection * TraceDistance;

  FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(WeaponTrace), true);
  QueryParams.AddIgnoredActor(this);

  if (const AActor *OwnerActor = GetOwner()) {
    QueryParams.AddIgnoredActor(OwnerActor);
  }
  if (OwningPawn.IsValid()) {
    QueryParams.AddIgnoredActor(OwningPawn.Get());
  }

  FHitResult AimHit;
  const bool bAimHit = World->LineTraceSingleByChannel(
      AimHit, ViewStart, ViewEnd, TraceChannel, QueryParams);
  const FVector AimPoint = bAimHit ? AimHit.ImpactPoint : ViewEnd;

  OutTraceStart = GetMuzzleLocation();
  OutTraceEnd = AimPoint;

  const bool bHit = World->LineTraceSingleByChannel(OutHit, OutTraceStart,
                                                     OutTraceEnd, TraceChannel,
                                                     QueryParams);

  if (bDrawDebugTrace) {
    const FColor TraceColor = bHit ? FColor::Green : FColor::Red;
    const FVector DebugEnd = bHit ? OutHit.ImpactPoint : OutTraceEnd;
    DrawDebugLine(World, OutTraceStart, DebugEnd, TraceColor, false,
                  DebugTraceDuration, 0, 1.5f);
  }

  return bHit;
}
