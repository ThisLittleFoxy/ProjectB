// Copyright Epic Games, Inc. All Rights Reserved.

#include "Combat/WeaponBase.h"
#include "Combat/HitZoneComponent.h"
#include "Character/HealthComponent.h"
#include "CollisionQueryParams.h"
#include "Components/SkeletalMeshComponent.h"
#include "DrawDebugHelpers.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameFramework/Controller.h"
#include "GameFramework/DamageType.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerState.h"
#include "Camera/PlayerCameraManager.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Particles/ParticleSystem.h"
#include "Project.h"
#include "Sound/SoundBase.h"
#include "TimerManager.h"
#include "Utils/AimTraceService.h"

namespace {
const TCHAR *GetHitZoneDebugName(EHitZone HitZone) {
  switch (HitZone) {
  case EHitZone::Head:
    return TEXT("Head");
  case EHitZone::Torso:
    return TEXT("Torso");
  case EHitZone::Limb:
    return TEXT("Limb");
  default:
    return TEXT("Unknown");
  }
}
} // namespace

AWeaponBase::AWeaponBase() {
  PrimaryActorTick.bCanEverTick = true;
  PrimaryActorTick.bStartWithTickEnabled = false;

  WeaponMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("WeaponMesh"));
  SetRootComponent(WeaponMesh);

  WeaponMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
  WeaponMesh->SetGenerateOverlapEvents(false);

  SprayPattern = {
      FVector2D(0.00f, 0.10f),  FVector2D(0.05f, 0.18f),
      FVector2D(-0.04f, 0.24f), FVector2D(0.08f, 0.30f),
      FVector2D(-0.10f, 0.36f), FVector2D(0.12f, 0.45f),
      FVector2D(-0.14f, 0.55f), FVector2D(0.16f, 0.65f)};
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

  CameraRecoilTargetOffsetDeg = FVector2D::ZeroVector;
  CameraRecoilAppliedOffsetDeg = FVector2D::ZeroVector;
  LastControlRotationBeforeRecoil = FRotator::ZeroRotator;
  bHasLastControlRotationBeforeRecoil = false;
  SetActorTickEnabled(false);
}

void AWeaponBase::Tick(float DeltaSeconds) {
  Super::Tick(DeltaSeconds);
  if (!UpdateCameraRecoilTick(DeltaSeconds)) {
    SetActorTickEnabled(false);
  }
}

bool AWeaponBase::UpdateCameraRecoilTick(float DeltaSeconds) {
  if (!bApplyCameraRecoil) {
    CameraRecoilTargetOffsetDeg = FVector2D::ZeroVector;
    CameraRecoilAppliedOffsetDeg = FVector2D::ZeroVector;
    bHasLastControlRotationBeforeRecoil = false;
    return false;
  }

  APlayerController *PlayerController = ResolveLocalPlayerController();
  if (!PlayerController) {
    CameraRecoilTargetOffsetDeg = FVector2D::ZeroVector;
    CameraRecoilAppliedOffsetDeg = FVector2D::ZeroVector;
    bHasLastControlRotationBeforeRecoil = false;
    return false;
  }

  const FRotator CurrentControlRotation = PlayerController->GetControlRotation();
  if (bHasLastControlRotationBeforeRecoil) {
    const double ExternalYawDeltaDeg = FMath::FindDeltaAngleDegrees(
        LastControlRotationBeforeRecoil.Yaw, CurrentControlRotation.Yaw);
    const double ExternalPitchDeltaDeg = FMath::FindDeltaAngleDegrees(
        LastControlRotationBeforeRecoil.Pitch, CurrentControlRotation.Pitch);

    auto ApplyCounterInputToRecoilAxis =
        [](double ExternalDeltaDeg, double MaxAxisRecoilDeg,
           double &TargetAxisDeg, double &AppliedAxisDeg) {
          if (FMath::Abs(ExternalDeltaDeg) <= KINDA_SMALL_NUMBER) {
            return;
          }

          const double OutstandingRecoilDeg =
              FMath::Abs(TargetAxisDeg) >= FMath::Abs(AppliedAxisDeg)
                  ? TargetAxisDeg
                  : AppliedAxisDeg;
          if (FMath::Abs(OutstandingRecoilDeg) <= KINDA_SMALL_NUMBER) {
            return;
          }

          // Only opposite-direction input is treated as recoil compensation.
          if (FMath::Sign(ExternalDeltaDeg) == FMath::Sign(OutstandingRecoilDeg)) {
            return;
          }

          const double CompensationAbsDeg =
              FMath::Min(FMath::Abs(ExternalDeltaDeg), FMath::Abs(OutstandingRecoilDeg));
          const double CompensationDeltaDeg =
              -FMath::Sign(OutstandingRecoilDeg) * CompensationAbsDeg;

          TargetAxisDeg = FMath::Clamp(TargetAxisDeg + CompensationDeltaDeg,
                                       -MaxAxisRecoilDeg, MaxAxisRecoilDeg);
          AppliedAxisDeg = FMath::Clamp(AppliedAxisDeg + CompensationDeltaDeg,
                                        -MaxAxisRecoilDeg, MaxAxisRecoilDeg);
        };

    ApplyCounterInputToRecoilAxis(
        ExternalYawDeltaDeg, MaxAccumulatedCameraYawRecoil,
        CameraRecoilTargetOffsetDeg.X, CameraRecoilAppliedOffsetDeg.X);
    ApplyCounterInputToRecoilAxis(
        ExternalPitchDeltaDeg, MaxAccumulatedCameraPitchRecoil,
        CameraRecoilTargetOffsetDeg.Y, CameraRecoilAppliedOffsetDeg.Y);
  } else {
    bHasLastControlRotationBeforeRecoil = true;
  }

  const bool bCanReturnNow = !bIsTriggerHeld || bReturnCameraRecoilWhileFiring;
  const float ReturnSpeed = FMath::Max(0.0f, CameraRecoilReturnSpeed);
  if (bCanReturnNow) {
    if (ReturnSpeed > KINDA_SMALL_NUMBER) {
      CameraRecoilTargetOffsetDeg.X = FMath::FInterpTo(
          CameraRecoilTargetOffsetDeg.X, 0.0f, DeltaSeconds, ReturnSpeed);
      CameraRecoilTargetOffsetDeg.Y = FMath::FInterpTo(
          CameraRecoilTargetOffsetDeg.Y, 0.0f, DeltaSeconds, ReturnSpeed);
    } else {
      CameraRecoilTargetOffsetDeg = FVector2D::ZeroVector;
    }
  }

  const float KickSpeed = FMath::Max(0.0f, CameraRecoilKickInterpSpeed);
  FVector2D NewAppliedOffsetDeg = CameraRecoilAppliedOffsetDeg;
  if (KickSpeed > KINDA_SMALL_NUMBER) {
    NewAppliedOffsetDeg.X =
        FMath::FInterpTo(NewAppliedOffsetDeg.X, CameraRecoilTargetOffsetDeg.X,
                         DeltaSeconds, KickSpeed);
    NewAppliedOffsetDeg.Y =
        FMath::FInterpTo(NewAppliedOffsetDeg.Y, CameraRecoilTargetOffsetDeg.Y,
                         DeltaSeconds, KickSpeed);
  } else {
    NewAppliedOffsetDeg = CameraRecoilTargetOffsetDeg;
  }

  const FVector2D DeltaOffsetDeg =
      NewAppliedOffsetDeg - CameraRecoilAppliedOffsetDeg;
  CameraRecoilAppliedOffsetDeg = NewAppliedOffsetDeg;

  FRotator FinalControlRotation = CurrentControlRotation;
  if (!DeltaOffsetDeg.IsNearlyZero(KINDA_SMALL_NUMBER)) {
    FRotator NewControlRotation = CurrentControlRotation;
    NewControlRotation.Pitch += DeltaOffsetDeg.Y;
    NewControlRotation.Yaw += DeltaOffsetDeg.X;

    if (const APlayerCameraManager *CameraManager =
            PlayerController->PlayerCameraManager) {
      NewControlRotation.Pitch = FMath::ClampAngle(
          NewControlRotation.Pitch, CameraManager->ViewPitchMin,
          CameraManager->ViewPitchMax);
    } else {
      NewControlRotation.Pitch =
          FMath::ClampAngle(NewControlRotation.Pitch, -89.0f, 89.0f);
    }

    PlayerController->SetControlRotation(NewControlRotation);
    FinalControlRotation = NewControlRotation;
  }

  LastControlRotationBeforeRecoil = FinalControlRotation;
  bHasLastControlRotationBeforeRecoil = true;

  const float StopThreshold = 0.005f;
  if (CameraRecoilTargetOffsetDeg.SizeSquared() <= StopThreshold * StopThreshold &&
      CameraRecoilAppliedOffsetDeg.SizeSquared() <=
          StopThreshold * StopThreshold) {
    CameraRecoilTargetOffsetDeg = FVector2D::ZeroVector;
    CameraRecoilAppliedOffsetDeg = FVector2D::ZeroVector;
    bHasLastControlRotationBeforeRecoil = false;
    return false;
  }

  return true;
}

void AWeaponBase::SetOwningPawn(APawn *NewOwningPawn) {
  OwningPawn = NewOwningPawn;
  CachedLocalPlayerController.Reset();
  bHasLastControlRotationBeforeRecoil = false;
  SetOwner(NewOwningPawn);
  SetInstigator(NewOwningPawn);
}

bool AWeaponBase::CanFire() const {
  return bInfiniteAmmo || CurrentAmmoInMagazine > 0;
}

void AWeaponBase::SetAiming(bool bNewAiming) {
  bIsAiming = bNewAiming;
}

void AWeaponBase::PlayFireCosmetics(
    const FWeaponFireCosmeticEvent &FireEvent) {
  if (FireEvent.bDryFire) {
    if (DryFireSound) {
      UGameplayStatics::PlaySoundAtLocation(this, DryFireSound,
                                            FireEvent.Location);
    }
    return;
  }

  if (FireSound) {
    UGameplayStatics::PlaySoundAtLocation(this, FireSound, GetMuzzleLocation());
  }

  if (MuzzleEffect) {
    UGameplayStatics::SpawnEmitterAtLocation(
        GetWorld(), MuzzleEffect, GetMuzzleLocation(), GetActorRotation(), true);
  }

  if (FireEvent.bHit && ImpactEffect) {
    UGameplayStatics::SpawnEmitterAtLocation(
        GetWorld(), ImpactEffect, FireEvent.Location, FireEvent.Normal.Rotation(),
        true);
  }
}

void AWeaponBase::PlayReloadCosmetics() {
  if (ReloadSound) {
    UGameplayStatics::PlaySoundAtLocation(this, ReloadSound, GetActorLocation());
  }
}

float AWeaponBase::GetDamageMultiplierForZone(EHitZone Zone) const {
  if (const float *FoundMultiplier = DamageMultiplierByZone.Find(Zone)) {
    return FMath::Max(0.0f, *FoundMultiplier);
  }

  return FMath::Max(0.0f, DefaultZoneDamageMultiplier);
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
    FWeaponFireCosmeticEvent FireEvent;
    FireEvent.bDryFire = true;
    FireEvent.Location = GetActorLocation();
    FireEvent.Normal = GetActorForwardVector();
    PlayFireCosmetics(FireEvent);
    OnWeaponFireCosmeticEvent.Broadcast(this, FireEvent);
    return false;
  }

  FHitResult HitResult;
  FVector TraceStart = FVector::ZeroVector;
  FVector TraceEnd = FVector::ZeroVector;
  const FVector2D RecoilOffsetDeg = BuildRecoilOffsetForShot();

  const bool bHit = MakeShotTrace(HitResult, TraceStart, TraceEnd, RecoilOffsetDeg);
  ApplyCameraRecoil(RecoilOffsetDeg);
  BP_OnRecoilApplied(RecoilOffsetDeg.X, RecoilOffsetDeg.Y);

  FWeaponFireCosmeticEvent FireEvent;
  FireEvent.bHit = bHit;
  FireEvent.Location = bHit ? HitResult.ImpactPoint : TraceEnd;
  FireEvent.Normal = bHit ? HitResult.ImpactNormal : -GetActorForwardVector();
  PlayFireCosmetics(FireEvent);
  OnWeaponFireCosmeticEvent.Broadcast(this, FireEvent);

  if (bHit) {
    if (AActor *HitActor = HitResult.GetActor()) {
      const FVector ShotDirection = (TraceEnd - TraceStart).GetSafeNormal();
      EHitZone HitZone = EHitZone::Unknown;
      bool bHasHitZoneComponent = false;
      if (const UHitZoneComponent *HitZoneComponent =
              HitActor->FindComponentByClass<UHitZoneComponent>()) {
        bHasHitZoneComponent = true;
        HitZone = HitZoneComponent->ResolveHitZone(HitResult);
      }
      const float ZoneDamageMultiplier = GetDamageMultiplierForZone(HitZone);
      const float FinalDamage = Damage * ZoneDamageMultiplier;

      bool bAppliedDamage = false;
      FString DamageRejectReason;
      if (ShouldApplyAuthoritativeDamage() &&
          ShouldApplyDamageToActor(HitActor, DamageRejectReason)) {
        UGameplayStatics::ApplyPointDamage(
            HitActor, FinalDamage, ShotDirection, HitResult,
            GetOwningController(), this, UDamageType::StaticClass());
        bAppliedDamage = true;
        UE_LOG(LogProject, Verbose,
               TEXT("Weapon damage applied. Weapon=%s Instigator=%s Target=%s Damage=%.2f"),
               *GetNameSafe(this), *GetNameSafe(OwningPawn.Get()),
               *GetNameSafe(HitActor), FinalDamage);
      } else if (ShouldApplyAuthoritativeDamage()) {
        UE_LOG(LogProject, Verbose,
               TEXT("Weapon damage rejected. Weapon=%s Instigator=%s Target=%s Damage=%.2f Reason=%s"),
               *GetNameSafe(this), *GetNameSafe(OwningPawn.Get()),
               *GetNameSafe(HitActor), FinalDamage,
               DamageRejectReason.IsEmpty() ? TEXT("Unknown") : *DamageRejectReason);
      }

      if (bAppliedDamage && bHasHitZoneComponent && GEngine &&
          FinalDamage > 0.0f) {
        const FString HitBoneName = HitResult.BoneName.IsNone()
                                        ? TEXT("none")
                                        : HitResult.BoneName.ToString();
        const FString WeaponTypeTagString =
            WeaponTypeTag.IsValid() ? WeaponTypeTag.ToString() : TEXT("None");
        const FString Message = FString::Printf(
            TEXT("%s received %.1f damage [weapon=%s, zone=%s, x%.2f, bone=%s]"),
            *GetNameSafe(HitActor), FinalDamage, *WeaponTypeTagString,
            GetHitZoneDebugName(HitZone), ZoneDamageMultiplier, *HitBoneName);
        GEngine->AddOnScreenDebugMessage(-1, 1.2f, FColor::Red, Message);
      }
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
  BurstShotIndex = 0;
  PlayReloadCosmetics();
  return AmmoToLoad > 0;
}

FWeaponAmmoSaveData AWeaponBase::GetAmmoSaveData() const {
  FWeaponAmmoSaveData AmmoSaveData;
  AmmoSaveData.AmmoInMagazine = CurrentAmmoInMagazine;
  AmmoSaveData.ReserveAmmo = ReserveAmmo;
  return AmmoSaveData;
}

void AWeaponBase::ApplyAmmoSaveData(const FWeaponAmmoSaveData &AmmoSaveData) {
  CurrentAmmoInMagazine =
      FMath::Clamp(AmmoSaveData.AmmoInMagazine, 0, FMath::Max(1, MagazineSize));
  ReserveAmmo = FMath::Max(0, AmmoSaveData.ReserveAmmo);
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

FVector2D AWeaponBase::BuildRecoilOffsetForShot() {
  const UWorld *World = GetWorld();
  const float CurrentTimeSeconds =
      World ? World->GetTimeSeconds() : LastShotTimeSeconds + SprayResetDelay + 1.0f;

  if (CurrentTimeSeconds - LastShotTimeSeconds > SprayResetDelay) {
    BurstShotIndex = 0;
  }

  FVector2D RecoilOffsetDeg = GetPatternOffsetForShot(BurstShotIndex);

  const float RandomMultiplier = bIsAiming ? AimingRecoilRandomMultiplier : 1.0f;
  RecoilOffsetDeg.X += FMath::FRandRange(-RandomRecoilYawDeg, RandomRecoilYawDeg) *
                       RandomMultiplier;
  RecoilOffsetDeg.Y +=
      FMath::FRandRange(-RandomRecoilPitchDeg, RandomRecoilPitchDeg) *
      RandomMultiplier;

  ++BurstShotIndex;
  LastShotTimeSeconds = CurrentTimeSeconds;

  return RecoilOffsetDeg;
}

FVector2D AWeaponBase::GetPatternOffsetForShot(int32 ShotIndex) const {
  const int32 NumPatternPoints = SprayPattern.Num();
  if (NumPatternPoints <= 0) {
    return FVector2D::ZeroVector;
  }

  if (bLoopSprayPattern) {
    const int32 WrappedIndex = ShotIndex % NumPatternPoints;
    return SprayPattern[WrappedIndex];
  }

  const int32 ClampedIndex = FMath::Clamp(ShotIndex, 0, NumPatternPoints - 1);
  return SprayPattern[ClampedIndex];
}

void AWeaponBase::ApplyCameraRecoil(const FVector2D &RecoilOffsetDeg) {
  if (!bApplyCameraRecoil || RecoilOffsetDeg.IsNearlyZero()) {
    return;
  }

  if (!ResolveLocalPlayerController()) {
    return;
  }

  const float AimingMultiplier = bIsAiming ? AimingCameraRecoilMultiplier : 1.0f;
  const float PitchKick =
      RecoilOffsetDeg.Y * CameraRecoilPitchMultiplier * AimingMultiplier;
  const float YawKick =
      RecoilOffsetDeg.X * CameraRecoilYawMultiplier * AimingMultiplier;

  CameraRecoilTargetOffsetDeg.X = FMath::Clamp(
      CameraRecoilTargetOffsetDeg.X + YawKick, -MaxAccumulatedCameraYawRecoil,
      MaxAccumulatedCameraYawRecoil);
  CameraRecoilTargetOffsetDeg.Y = FMath::Clamp(
      CameraRecoilTargetOffsetDeg.Y + PitchKick, -MaxAccumulatedCameraPitchRecoil,
      MaxAccumulatedCameraPitchRecoil);

  if (!IsActorTickEnabled()) {
    if (APlayerController *PlayerController = ResolveLocalPlayerController()) {
      LastControlRotationBeforeRecoil = PlayerController->GetControlRotation();
      bHasLastControlRotationBeforeRecoil = true;
    } else {
      bHasLastControlRotationBeforeRecoil = false;
    }
    SetActorTickEnabled(true);
  }
}

APlayerController *AWeaponBase::ResolveLocalPlayerController() {
  if (CachedLocalPlayerController.IsValid()) {
    return CachedLocalPlayerController.Get();
  }

  APlayerController *PlayerController =
      Cast<APlayerController>(GetOwningController());
  if (!PlayerController || !PlayerController->IsLocalController()) {
    CachedLocalPlayerController.Reset();
    return nullptr;
  }

  CachedLocalPlayerController = PlayerController;
  return PlayerController;
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

bool AWeaponBase::ShouldApplyAuthoritativeDamage() const {
  if (GetNetMode() == NM_Standalone) {
    return true;
  }

  if (OwningPawn.IsValid()) {
    return OwningPawn->HasAuthority();
  }

  if (const APawn *OwnerPawn = Cast<APawn>(GetOwner())) {
    return OwnerPawn->HasAuthority();
  }

  return HasAuthority();
}

bool AWeaponBase::ShouldApplyDamageToActor(const AActor *HitActor,
                                           FString &OutRejectReason) const {
  if (!HitActor) {
    OutRejectReason = TEXT("NoHitActor");
    return false;
  }

  const APawn *HitPawn = Cast<APawn>(HitActor);
  if (!HitPawn) {
    return true;
  }

  if (!HitPawn->GetPlayerState()) {
    return true;
  }

  if (bBypassFriendlyFireRules) {
    return true;
  }

  const UHealthComponent *TargetHealthComponent =
      HitPawn->FindComponentByClass<UHealthComponent>();
  if (!TargetHealthComponent) {
    OutRejectReason = TEXT("PlayerTargetHasNoHealthComponent");
    return false;
  }

  if (!TargetHealthComponent->AcceptsFriendlyFireDamage()) {
    OutRejectReason = TEXT("FriendlyFireDisabledOnTarget");
    return false;
  }

  return true;
}

FVector AWeaponBase::GetMuzzleLocation() const {
  if (WeaponMesh && WeaponMesh->DoesSocketExist(MuzzleSocketName)) {
    return WeaponMesh->GetSocketLocation(MuzzleSocketName);
  }

  return GetActorLocation();
}

bool AWeaponBase::MakeShotTrace(FHitResult &OutHit, FVector &OutTraceStart,
                                FVector &OutTraceEnd,
                                const FVector2D &RecoilOffsetDeg) const {
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
  if (!RecoilOffsetDeg.IsNearlyZero()) {
    const FRotator RecoilRotation(RecoilOffsetDeg.Y, RecoilOffsetDeg.X, 0.0f);
    ShotDirection = RecoilRotation.RotateVector(ShotDirection).GetSafeNormal();
  }

  const float EffectiveSpread = bIsAiming ? SpreadAngleDeg * AimingSpreadMultiplier
                                          : SpreadAngleDeg;
  if (EffectiveSpread > KINDA_SMALL_NUMBER) {
    const float ConeHalfAngle = FMath::DegreesToRadians(EffectiveSpread);
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

  FHitResult PawnHit;
  if (FindPawnTraceHit(World, OutTraceStart, OutTraceEnd, QueryParams,
                       PawnHit)) {
    const float ExistingHitDistanceSq =
        bHit ? FVector::DistSquared(OutTraceStart, OutHit.ImpactPoint)
             : TNumericLimits<float>::Max();
    const float PawnHitDistanceSq =
        FVector::DistSquared(OutTraceStart, PawnHit.ImpactPoint);
    if (!bHit || PawnHitDistanceSq <= ExistingHitDistanceSq + 1.0f) {
      OutHit = PawnHit;
      return true;
    }
  }

  if (bDrawDebugTrace) {
    const FColor TraceColor = bHit ? FColor::Green : FColor::Red;
    const FVector DebugEnd = bHit ? OutHit.ImpactPoint : OutTraceEnd;
    DrawDebugLine(World, OutTraceStart, DebugEnd, TraceColor, false,
                  DebugTraceDuration, 0, 1.5f);
  }

  return bHit;
}

bool AWeaponBase::FindPawnTraceHit(
    UWorld *World, const FVector &TraceStart, const FVector &TraceEnd,
    const FCollisionQueryParams &QueryParams, FHitResult &OutHit) const {
  if (!World) {
    return false;
  }

  FCollisionObjectQueryParams ObjectQueryParams;
  ObjectQueryParams.AddObjectTypesToQuery(ECC_Pawn);

  TArray<FHitResult> PawnHits;
  if (!World->LineTraceMultiByObjectType(PawnHits, TraceStart, TraceEnd,
                                         ObjectQueryParams, QueryParams)) {
    return false;
  }

  PawnHits.Sort([&TraceStart](const FHitResult &Left, const FHitResult &Right) {
    return FVector::DistSquared(TraceStart, Left.ImpactPoint) <
           FVector::DistSquared(TraceStart, Right.ImpactPoint);
  });

  for (const FHitResult &PawnHit : PawnHits) {
    AActor *HitActor = PawnHit.GetActor();
    if (!HitActor || HitActor == GetOwner() || HitActor == OwningPawn.Get()) {
      continue;
    }

    if (HitActor->FindComponentByClass<UHealthComponent>() ||
        Cast<APawn>(HitActor)) {
      OutHit = PawnHit;
      return true;
    }
  }

  return false;
}
