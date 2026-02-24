// Copyright Epic Games, Inc. All Rights Reserved.

#include "Combat/CombatComponent.h"
#include "Combat/WeaponBase.h"
#include "Camera/CameraComponent.h"
#include "Components/SceneComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "GameplayTagsManager.h"
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
  ScopeOverlayWeaponTypeTag =
      FGameplayTag::RequestGameplayTag(TEXT("Weapon.Type.Sniper"), false);
}

void UCombatComponent::BeginPlay() {
  Super::BeginPlay();

  if (!ScopeOverlayWeaponTypeTag.IsValid()) {
    ScopeOverlayWeaponTypeTag =
        FGameplayTag::RequestGameplayTag(TEXT("Weapon.Type.Sniper"), false);
  }

  CacheOwnerReferences();

  if (bSpawnStarterWeaponOnBeginPlay && !CurrentWeapon) {
    if (!InitializeLoadout()) {
      UE_LOG(LogProject, Warning,
             TEXT("CombatComponent: failed to initialize loadout on '%s'"),
             *GetNameSafe(GetOwner()));
    }
  }
}

bool UCombatComponent::InitializeLoadout() {
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

  StopScope();
  DestroyAllLoadoutWeapons();

  TArray<TSubclassOf<AWeaponBase>> EffectiveLoadoutClasses;
  for (const TSubclassOf<AWeaponBase> WeaponClass : LoadoutWeaponClasses) {
    if (WeaponClass) {
      EffectiveLoadoutClasses.Add(WeaponClass);
    }
  }

  if (EffectiveLoadoutClasses.Num() == 0 && StarterWeaponClass) {
    EffectiveLoadoutClasses.Add(StarterWeaponClass);
  }

  if (EffectiveLoadoutClasses.Num() == 0) {
    UE_LOG(LogProject, Warning,
           TEXT("CombatComponent: no loadout classes configured on '%s'"),
           *GetNameSafe(GetOwner()));
    CurrentWeapon = nullptr;
    CurrentWeaponSlotIndex = INDEX_NONE;
    return false;
  }

  FActorSpawnParameters SpawnParams;
  SpawnParams.Owner = OwningCharacter;
  SpawnParams.Instigator = OwningCharacter;
  SpawnParams.SpawnCollisionHandlingOverride =
      ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

  SpawnedLoadoutWeapons.Reserve(EffectiveLoadoutClasses.Num());

  for (const TSubclassOf<AWeaponBase> WeaponClass : EffectiveLoadoutClasses) {
    AWeaponBase *SpawnedWeapon = World->SpawnActor<AWeaponBase>(
        WeaponClass, OwningCharacter->GetActorTransform(), SpawnParams);
    if (!IsValid(SpawnedWeapon)) {
      UE_LOG(LogProject, Warning,
             TEXT("CombatComponent: failed to spawn loadout weapon '%s' on '%s'"),
             *GetNameSafe(WeaponClass.Get()), *GetNameSafe(GetOwner()));
      continue;
    }

    SpawnedWeapon->SetOwningPawn(OwningCharacter);
    AttachWeaponToOwner(SpawnedWeapon);
    SetWeaponActiveState(SpawnedWeapon, false);
    SpawnedLoadoutWeapons.Add(SpawnedWeapon);
  }

  if (SpawnedLoadoutWeapons.Num() == 0) {
    UE_LOG(LogProject, Warning,
           TEXT("CombatComponent: loadout spawn produced no valid weapons on '%s'"),
           *GetNameSafe(GetOwner()));
    return false;
  }

  const int32 EquippedSlot =
      FMath::Clamp(InitialEquippedSlotIndex, 0, SpawnedLoadoutWeapons.Num() - 1);
  return EquipWeaponSlot(EquippedSlot);
}

bool UCombatComponent::EquipWeaponSlot(int32 SlotIndex) {
  if (!SpawnedLoadoutWeapons.IsValidIndex(SlotIndex)) {
    return false;
  }

  AWeaponBase *NewWeapon = SpawnedLoadoutWeapons[SlotIndex];
  if (!IsValid(NewWeapon)) {
    return false;
  }

  if (!OwningCharacter) {
    CacheOwnerReferences();
    if (!OwningCharacter) {
      return false;
    }
  }

  if (CurrentWeapon == NewWeapon && CurrentWeaponSlotIndex == SlotIndex) {
    SetWeaponActiveState(CurrentWeapon, true);
    return true;
  }

  if (bIsScoping) {
    StopScope();
  }

  AWeaponBase *PreviousWeapon = CurrentWeapon;
  if (IsValid(PreviousWeapon) && PreviousWeapon != NewWeapon) {
    SetWeaponActiveState(PreviousWeapon, false);
  }

  CurrentWeapon = NewWeapon;
  CurrentWeaponSlotIndex = SlotIndex;
  CurrentWeapon->SetOwningPawn(OwningCharacter);
  AttachWeaponToOwner(CurrentWeapon);
  SetWeaponActiveState(CurrentWeapon, true);

  BroadcastCurrentWeaponChanged(PreviousWeapon, CurrentWeapon,
                                CurrentWeaponSlotIndex);
  return true;
}

bool UCombatComponent::EquipNextWeapon() {
  const int32 WeaponCount = SpawnedLoadoutWeapons.Num();
  if (WeaponCount <= 1) {
    return false;
  }

  const int32 BaseIndex = CurrentWeaponSlotIndex == INDEX_NONE ? 0
                                                                : CurrentWeaponSlotIndex;
  const int32 NextIndex = (BaseIndex + 1) % WeaponCount;
  return EquipWeaponSlot(NextIndex);
}

bool UCombatComponent::EquipPreviousWeapon() {
  const int32 WeaponCount = SpawnedLoadoutWeapons.Num();
  if (WeaponCount <= 1) {
    return false;
  }

  const int32 BaseIndex = CurrentWeaponSlotIndex == INDEX_NONE ? 0
                                                                : CurrentWeaponSlotIndex;
  const int32 PreviousIndex = (BaseIndex - 1 + WeaponCount) % WeaponCount;
  return EquipWeaponSlot(PreviousIndex);
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

  if (CurrentWeapon == NewWeapon && CurrentWeaponSlotIndex != INDEX_NONE) {
    return true;
  }

  if (bIsScoping) {
    StopScope();
  }

  for (AWeaponBase *ExistingWeapon : SpawnedLoadoutWeapons) {
    if (IsValid(ExistingWeapon) && ExistingWeapon != NewWeapon) {
      ExistingWeapon->Destroy();
    }
  }
  SpawnedLoadoutWeapons.Reset();

  NewWeapon->SetOwningPawn(OwningCharacter);
  AttachWeaponToOwner(NewWeapon);
  SetWeaponActiveState(NewWeapon, false);
  SpawnedLoadoutWeapons.Add(NewWeapon);

  CurrentWeapon = nullptr;
  CurrentWeaponSlotIndex = INDEX_NONE;
  return EquipWeaponSlot(0);
}

void UCombatComponent::UnequipCurrentWeapon(bool bDestroyWeapon) {
  if (!CurrentWeapon) {
    return;
  }

  StopScope();

  AWeaponBase *PreviousWeapon = CurrentWeapon;
  PreviousWeapon->StopFire();
  PreviousWeapon->SetAiming(false);

  const int32 ExistingIndex = SpawnedLoadoutWeapons.IndexOfByKey(PreviousWeapon);
  if (ExistingIndex != INDEX_NONE) {
    SpawnedLoadoutWeapons.RemoveAt(ExistingIndex);
  }

  if (bDestroyWeapon) {
    PreviousWeapon->Destroy();
  } else {
    PreviousWeapon->DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
    SetWeaponActiveState(PreviousWeapon, false);
  }

  CurrentWeapon = nullptr;
  CurrentWeaponSlotIndex = INDEX_NONE;
  BroadcastCurrentWeaponChanged(PreviousWeapon, nullptr, INDEX_NONE);
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

bool UCombatComponent::IsCurrentWeaponScopeType() const {
  if (!CurrentWeapon || !ScopeOverlayWeaponTypeTag.IsValid()) {
    return false;
  }

  return CurrentWeapon->GetWeaponTypeTag().MatchesTagExact(
      ScopeOverlayWeaponTypeTag);
}

bool UCombatComponent::IsScopeOverlayActive() const {
  return bIsScoping && IsCurrentWeaponScopeType();
}

FGameplayTag UCombatComponent::GetCurrentWeaponTypeTag() const {
  return CurrentWeapon ? CurrentWeapon->GetWeaponTypeTag() : FGameplayTag();
}

UTexture2D *UCombatComponent::GetCurrentWeaponIcon() const {
  return CurrentWeapon ? CurrentWeapon->GetWeaponIcon() : nullptr;
}

void UCombatComponent::AttachWeaponToOwner(AWeaponBase *Weapon) {
  if (!IsValid(Weapon) || !OwningCharacter) {
    return;
  }

  CachedAttachMesh = ResolveAttachMesh();

  if (CachedAttachMesh) {
    const FName WeaponSocketOverride = Weapon->GetAttachSocketNameOverride();
    FName AttachSocketName = WeaponAttachSocketName;
    if (!WeaponSocketOverride.IsNone()) {
      AttachSocketName = WeaponSocketOverride;
    }

    bool bHasSocket = CachedAttachMesh->DoesSocketExist(AttachSocketName);
    if (!bHasSocket && AttachSocketName != WeaponAttachSocketName &&
        CachedAttachMesh->DoesSocketExist(WeaponAttachSocketName)) {
      UE_LOG(LogProject, Warning,
             TEXT("CombatComponent: socket override '%s' not found on mesh '%s', "
                  "falling back to default socket '%s'"),
             *AttachSocketName.ToString(), *GetNameSafe(CachedAttachMesh),
             *WeaponAttachSocketName.ToString());
      AttachSocketName = WeaponAttachSocketName;
      bHasSocket = true;
    }

    UE_LOG(LogProject, Log,
           TEXT("CombatComponent: attaching weapon '%s' to mesh '%s' socket '%s' (exists=%s)"),
           *GetNameSafe(Weapon), *GetNameSafe(CachedAttachMesh),
           *AttachSocketName.ToString(), bHasSocket ? TEXT("true") : TEXT("false"));

    Weapon->AttachToComponent(
        CachedAttachMesh, FAttachmentTransformRules::SnapToTargetNotIncludingScale,
        AttachSocketName);

    if (USceneComponent *WeaponRoot = Weapon->GetRootComponent()) {
      WeaponRoot->SetUsingAbsoluteLocation(false);
      WeaponRoot->SetUsingAbsoluteRotation(false);
      WeaponRoot->SetUsingAbsoluteScale(false);
    }

    if (USkeletalMeshComponent *WeaponMesh = Weapon->GetWeaponMesh()) {
      CopyFirstPersonRenderSettings(CachedAttachMesh, WeaponMesh);
    }
  } else if (USceneComponent *Root = OwningCharacter->GetRootComponent()) {
    Weapon->AttachToComponent(Root,
                              FAttachmentTransformRules::SnapToTargetNotIncludingScale);
  }
}

void UCombatComponent::SetWeaponActiveState(AWeaponBase *Weapon,
                                            bool bShouldBeActive) {
  if (!IsValid(Weapon)) {
    return;
  }

  if (!bShouldBeActive) {
    Weapon->StopFire();
    Weapon->SetAiming(false);
    Weapon->SetActorTickEnabled(false);
  }

  Weapon->SetActorHiddenInGame(!bShouldBeActive);
  Weapon->SetActorEnableCollision(false);

  if (USkeletalMeshComponent *WeaponMesh = Weapon->GetWeaponMesh()) {
    WeaponMesh->SetVisibility(bShouldBeActive, true);
  }
}

void UCombatComponent::BroadcastCurrentWeaponChanged(AWeaponBase *PreviousWeapon,
                                                     AWeaponBase *NewWeapon,
                                                     int32 NewSlotIndex) {
  const FGameplayTag NewWeaponTypeTag =
      IsValid(NewWeapon) ? NewWeapon->GetWeaponTypeTag() : FGameplayTag();
  OnCurrentWeaponChanged.Broadcast(PreviousWeapon, NewWeapon, NewSlotIndex,
                                   NewWeaponTypeTag);
}

void UCombatComponent::DestroyAllLoadoutWeapons() {
  for (AWeaponBase *Weapon : SpawnedLoadoutWeapons) {
    if (IsValid(Weapon)) {
      Weapon->Destroy();
    }
  }

  SpawnedLoadoutWeapons.Reset();
  CurrentWeapon = nullptr;
  CurrentWeaponSlotIndex = INDEX_NONE;
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
