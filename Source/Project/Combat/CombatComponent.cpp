// Copyright Epic Games, Inc. All Rights Reserved.

#include "Combat/CombatComponent.h"
#include "Combat/WeaponBase.h"
#include "Camera/CameraComponent.h"
#include "Components/SceneComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "Interaction/InteractionComponent.h"
#include "Project.h"
#include "UObject/UnrealType.h"

namespace {
bool CopyPropertyByName(UObject *SourceObject, UObject *TargetObject,
                        const FName PropertyName) {
  if (!SourceObject || !TargetObject) {
    return false;
  }

  FProperty *SourceProperty =
      SourceObject->GetClass()->FindPropertyByName(PropertyName);
  FProperty *TargetProperty =
      TargetObject->GetClass()->FindPropertyByName(PropertyName);
  if (!SourceProperty || !TargetProperty || !SourceProperty->SameType(TargetProperty)) {
    return false;
  }

  const void *SourceValue =
      SourceProperty->ContainerPtrToValuePtr<void>(SourceObject);
  void *TargetValue = TargetProperty->ContainerPtrToValuePtr<void>(TargetObject);
  TargetProperty->CopyCompleteValue(TargetValue, SourceValue);
  return true;
}

void CopyFirstPersonRenderSettings(USkeletalMeshComponent *SourceMesh,
                                   USkeletalMeshComponent *TargetMesh) {
  if (!SourceMesh || !TargetMesh) {
    return;
  }

  bool bCopiedAnyFirstPersonProperty = false;
  bCopiedAnyFirstPersonProperty |=
      CopyPropertyByName(SourceMesh, TargetMesh, TEXT("FirstPersonPrimitiveType"));
  bCopiedAnyFirstPersonProperty |= CopyPropertyByName(
      SourceMesh, TargetMesh, TEXT("bEnableFirstPersonFieldOfView"));
  bCopiedAnyFirstPersonProperty |=
      CopyPropertyByName(SourceMesh, TargetMesh, TEXT("FirstPersonFieldOfView"));
  bCopiedAnyFirstPersonProperty |=
      CopyPropertyByName(SourceMesh, TargetMesh, TEXT("bEnableFirstPersonScale"));
  bCopiedAnyFirstPersonProperty |=
      CopyPropertyByName(SourceMesh, TargetMesh, TEXT("FirstPersonScale"));

  // Keep visibility behavior consistent with first-person arms mesh.
  CopyPropertyByName(SourceMesh, TargetMesh, TEXT("bOnlyOwnerSee"));
  CopyPropertyByName(SourceMesh, TargetMesh, TEXT("bOwnerNoSee"));

  if (!bCopiedAnyFirstPersonProperty) {
    UE_LOG(LogProject, Verbose,
           TEXT("CombatComponent: first-person render properties were not found, "
                "skipping first-person visual sync for weapon mesh"));
  }

  TargetMesh->MarkRenderStateDirty();
}
} // namespace

UCombatComponent::UCombatComponent() {
  PrimaryComponentTick.bCanEverTick = false;
}

void UCombatComponent::BeginPlay() {
  Super::BeginPlay();

  CacheOwnerReferences();

  if (bSpawnStarterWeaponOnBeginPlay && StarterWeaponClass && !CurrentWeapon) {
    if (!EquipWeapon(StarterWeaponClass)) {
      UE_LOG(LogProject, Warning,
             TEXT("CombatComponent: failed to spawn starter weapon on '%s'"),
             *GetNameSafe(GetOwner()));
    }
  }
}

bool UCombatComponent::EquipWeapon(TSubclassOf<AWeaponBase> WeaponClass) {
  if (!WeaponClass) {
    UE_LOG(LogProject, Warning,
           TEXT("CombatComponent: EquipWeapon called with null class"));
    return false;
  }

  if (!OwningCharacter) {
    CacheOwnerReferences();
    if (!OwningCharacter) {
      UE_LOG(LogProject, Error,
             TEXT("CombatComponent: owner is not a Character"));
      return false;
    }
  }

  UWorld *World = GetWorld();
  if (!World) {
    return false;
  }

  FActorSpawnParameters SpawnParams;
  SpawnParams.Owner = OwningCharacter;
  SpawnParams.Instigator = OwningCharacter;
  SpawnParams.SpawnCollisionHandlingOverride =
      ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

  AWeaponBase *SpawnedWeapon = World->SpawnActor<AWeaponBase>(
      WeaponClass, OwningCharacter->GetActorTransform(), SpawnParams);

  return EquipSpawnedWeapon(SpawnedWeapon);
}

bool UCombatComponent::EquipSpawnedWeapon(AWeaponBase *NewWeapon) {
  if (!IsValid(NewWeapon)) {
    return false;
  }

  if (!OwningCharacter) {
    CacheOwnerReferences();
  }
  if (!OwningCharacter) {
    return false;
  }

  if (CurrentWeapon == NewWeapon) {
    return true;
  }

  UnequipCurrentWeapon(true);
  CurrentWeapon = NewWeapon;
  CurrentWeapon->SetOwningPawn(OwningCharacter);

  CachedAttachMesh = ResolveAttachMesh();

  if (CachedAttachMesh) {
    const bool bHasSocket = CachedAttachMesh->DoesSocketExist(WeaponAttachSocketName);
    UE_LOG(
        LogProject, Log,
        TEXT("CombatComponent: attaching weapon '%s' to mesh '%s' socket '%s' (exists=%s)"),
        *GetNameSafe(CurrentWeapon), *GetNameSafe(CachedAttachMesh),
        *WeaponAttachSocketName.ToString(), bHasSocket ? TEXT("true") : TEXT("false"));

    CurrentWeapon->AttachToComponent(
        CachedAttachMesh, FAttachmentTransformRules::SnapToTargetNotIncludingScale,
        WeaponAttachSocketName);

    if (USceneComponent *WeaponRoot = CurrentWeapon->GetRootComponent()) {
      WeaponRoot->SetUsingAbsoluteLocation(false);
      WeaponRoot->SetUsingAbsoluteRotation(false);
      WeaponRoot->SetUsingAbsoluteScale(false);
    }

    if (USkeletalMeshComponent *WeaponMesh = CurrentWeapon->GetWeaponMesh()) {
      CopyFirstPersonRenderSettings(CachedAttachMesh, WeaponMesh);
    }
  } else if (USceneComponent *Root = OwningCharacter->GetRootComponent()) {
    CurrentWeapon->AttachToComponent(
        Root, FAttachmentTransformRules::SnapToTargetNotIncludingScale);
  }

  return true;
}

void UCombatComponent::UnequipCurrentWeapon(bool bDestroyWeapon) {
  if (!CurrentWeapon) {
    return;
  }

  StopScope();
  CurrentWeapon->StopFire();
  CurrentWeapon->SetAiming(false);

  if (bDestroyWeapon) {
    CurrentWeapon->Destroy();
  } else {
    CurrentWeapon->DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
  }

  CurrentWeapon = nullptr;
}

void UCombatComponent::StartFire() {
  if (bFireInteractsWithWidgets) {
    if (!CachedInteractionComponent) {
      CachedInteractionComponent = ResolveInteractionComponent();
    }

    if (CachedInteractionComponent &&
        CachedInteractionComponent->IsHoveringWidget()) {
      CachedInteractionComponent->PressWidgetInteraction();
      return;
    }
  }

  if (CurrentWeapon) {
    CurrentWeapon->StartFire();
  }
}

void UCombatComponent::StopFire() {
  if (CachedInteractionComponent) {
    CachedInteractionComponent->ReleaseWidgetInteraction();
  }

  if (CurrentWeapon) {
    CurrentWeapon->StopFire();
  }
}

bool UCombatComponent::Reload() {
  return CurrentWeapon ? CurrentWeapon->Reload() : false;
}

int32 UCombatComponent::GetAmmoInMagazine() const {
  return CurrentWeapon ? CurrentWeapon->GetAmmoInMagazine() : 0;
}

int32 UCombatComponent::GetAmmoInReserve() const {
  return CurrentWeapon ? CurrentWeapon->GetReserveAmmo() : 0;
}

int32 UCombatComponent::GetAmmoTotalAvailable() const {
  if (!CurrentWeapon) {
    return 0;
  }

  return CurrentWeapon->GetAmmoInMagazine() + CurrentWeapon->GetReserveAmmo();
}

void UCombatComponent::StartScope() {
  bIsScoping = true;

  if (CurrentWeapon) {
    CurrentWeapon->SetAiming(true);
  }

  if (!bEnableScopeFov) {
    return;
  }

  if (!CachedCameraComponent) {
    CachedCameraComponent = ResolveCameraComponent();
  }
  if (!CachedCameraComponent) {
    return;
  }

  if (DefaultFieldOfView <= KINDA_SMALL_NUMBER) {
    DefaultFieldOfView = CachedCameraComponent->FieldOfView;
  }

  CachedCameraComponent->SetFieldOfView(ScopedFieldOfView);
}

void UCombatComponent::StopScope() {
  bIsScoping = false;

  if (CurrentWeapon) {
    CurrentWeapon->SetAiming(false);
  }

  if (!bEnableScopeFov) {
    return;
  }

  if (!CachedCameraComponent) {
    CachedCameraComponent = ResolveCameraComponent();
  }
  if (!CachedCameraComponent) {
    return;
  }

  if (DefaultFieldOfView > KINDA_SMALL_NUMBER) {
    CachedCameraComponent->SetFieldOfView(DefaultFieldOfView);
  }
}

void UCombatComponent::CacheOwnerReferences() {
  OwningCharacter = Cast<ACharacter>(GetOwner());
  CachedCameraComponent = ResolveCameraComponent();
  CachedInteractionComponent = ResolveInteractionComponent();
  CachedAttachMesh = ResolveAttachMesh();

  if (CachedCameraComponent && DefaultFieldOfView <= KINDA_SMALL_NUMBER) {
    DefaultFieldOfView = CachedCameraComponent->FieldOfView;
  }
}

UCameraComponent *UCombatComponent::ResolveCameraComponent() const {
  return OwningCharacter ? OwningCharacter->FindComponentByClass<UCameraComponent>()
                         : nullptr;
}

UInteractionComponent *UCombatComponent::ResolveInteractionComponent() const {
  return OwningCharacter ? OwningCharacter->FindComponentByClass<UInteractionComponent>()
                         : nullptr;
}

USkeletalMeshComponent *UCombatComponent::ResolveAttachMesh() const {
  if (!OwningCharacter) {
    return nullptr;
  }

  TArray<USkeletalMeshComponent *> MeshComponents;
  OwningCharacter->GetComponents<USkeletalMeshComponent>(MeshComponents);

  if (bPreferFirstPersonMesh) {
    // 1) Best-effort: pick nearest skeletal mesh in camera parent chain.
    // This matches hierarchy like Mesh -> FirstPersonMesh -> FirstPersonCamera.
    if (const UCameraComponent *CameraComp =
            OwningCharacter->FindComponentByClass<UCameraComponent>()) {
      USceneComponent *CurrentParent = CameraComp->GetAttachParent();
      while (CurrentParent) {
        if (USkeletalMeshComponent *ParentMesh =
                Cast<USkeletalMeshComponent>(CurrentParent)) {
          return ParentMesh;
        }
        CurrentParent = CurrentParent->GetAttachParent();
      }

      // 2) Alternative FPS setup: mesh can be attached as child to camera.
      for (USkeletalMeshComponent *MeshComp : MeshComponents) {
        if (IsValid(MeshComp) && MeshComp->IsAttachedTo(CameraComp)) {
          return MeshComp;
        }
      }
    }

    // 3) Fallback: pick by configured name (exact or contains)
    const FString PreferredName = PreferredFirstPersonMeshName.ToString();
    for (USkeletalMeshComponent *MeshComp : MeshComponents) {
      if (!IsValid(MeshComp)) {
        continue;
      }

      if (MeshComp->GetFName() == PreferredFirstPersonMeshName ||
          MeshComp->GetName().Equals(PreferredFirstPersonMeshName.ToString(),
                                     ESearchCase::IgnoreCase) ||
          (!PreferredName.IsEmpty() &&
           MeshComp->GetName().Contains(PreferredName, ESearchCase::IgnoreCase))) {
        return MeshComp;
      }
    }
  }

  if (USkeletalMeshComponent *CharacterMesh = OwningCharacter->GetMesh()) {
    return CharacterMesh;
  }

  for (USkeletalMeshComponent *MeshComp : MeshComponents) {
    if (IsValid(MeshComp)) {
      return MeshComp;
    }
  }

  return nullptr;
}
