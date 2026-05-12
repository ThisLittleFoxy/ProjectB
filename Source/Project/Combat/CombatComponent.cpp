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
#include "Net/UnrealNetwork.h"
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
  CopyPropertyByName(SourceMesh, TargetMesh, TEXT("bOnlyOwnerSee"));
  CopyPropertyByName(SourceMesh, TargetMesh, TEXT("bOwnerNoSee"));

  if (!bCopiedAnyFirstPersonProperty) {
    UE_LOG(LogProject, Verbose,
           TEXT("CombatComponent: first-person render properties were not found, "
                "skipping first-person visual sync for weapon mesh"));
  }

  TargetMesh->MarkRenderStateDirty();
}

bool IsValidLoadoutSlotIndex(int32 SlotIndex) {
  return SlotIndex >= 0 && SlotIndex < ProjectWeaponLoadout::SlotCount;
}
} // namespace

UCombatComponent::UCombatComponent() {
  PrimaryComponentTick.bCanEverTick = false;
  SetIsReplicatedByDefault(true);
  ScopeOverlayWeaponTypeTag =
      FGameplayTag::RequestGameplayTag(TEXT("Weapon.Type.Sniper"), false);
  EnsureLoadoutArraySize();
}

void UCombatComponent::GetLifetimeReplicatedProps(
    TArray<FLifetimeProperty> &OutLifetimeProps) const {
  Super::GetLifetimeReplicatedProps(OutLifetimeProps);

  DOREPLIFETIME(UCombatComponent, ReplicatedLoadoutWeaponClasses);
  DOREPLIFETIME(UCombatComponent, ReplicatedCurrentWeaponSlotIndex);
}

void UCombatComponent::BeginPlay() {
  Super::BeginPlay();

  if (!ScopeOverlayWeaponTypeTag.IsValid()) {
    ScopeOverlayWeaponTypeTag =
        FGameplayTag::RequestGameplayTag(TEXT("Weapon.Type.Sniper"), false);
  }

  CacheOwnerReferences();
  EnsureLoadoutArraySize();

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
      UE_LOG(LogProject, Error, TEXT("CombatComponent: owner is not a Character"));
      return false;
    }
  }

  StopScope();
  DestroyAllLoadoutWeapons();
  EnsureLoadoutArraySize();

  TArray<TSubclassOf<AWeaponBase>> EffectiveLoadoutClasses;
  EffectiveLoadoutClasses.SetNumZeroed(ProjectWeaponLoadout::SlotCount);
  for (int32 SlotIndex = 0;
       SlotIndex < LoadoutWeaponClasses.Num() &&
       SlotIndex < ProjectWeaponLoadout::SlotCount;
       ++SlotIndex) {
    EffectiveLoadoutClasses[SlotIndex] = LoadoutWeaponClasses[SlotIndex];
  }

  if (!EffectiveLoadoutClasses[0] && StarterWeaponClass) {
    EffectiveLoadoutClasses[0] = StarterWeaponClass;
  }

  bool bSpawnedAnyWeapon = false;
  for (int32 SlotIndex = 0; SlotIndex < EffectiveLoadoutClasses.Num(); ++SlotIndex) {
    if (!EffectiveLoadoutClasses[SlotIndex]) {
      continue;
    }

    bSpawnedAnyWeapon |=
        SetWeaponForSlot(static_cast<EWeaponLoadoutSlot>(SlotIndex),
                         EffectiveLoadoutClasses[SlotIndex]);
  }

  if (!bSpawnedAnyWeapon) {
    CurrentWeapon = nullptr;
    CurrentWeaponSlotIndex = INDEX_NONE;
    UpdateReplicatedWeaponPresenceFromLocalState();
    return false;
  }

  const int32 PreferredSlotIndex =
      FMath::Clamp(InitialEquippedSlotIndex, 0, ProjectWeaponLoadout::SlotCount - 1);
  if (IsValidLoadoutSlotIndex(PreferredSlotIndex) &&
      IsValid(SpawnedLoadoutWeapons[PreferredSlotIndex])) {
    return EquipWeaponSlot(PreferredSlotIndex);
  }

  const int32 FallbackSlotIndex = FindNextOccupiedSlotIndex(0, 1);
  return IsValidLoadoutSlotIndex(FallbackSlotIndex)
             ? EquipWeaponSlot(FallbackSlotIndex)
             : false;
}

void UCombatComponent::ClearLoadout() {
  if (bIsScoping) {
    StopScope();
  }

  AWeaponBase *PreviousWeapon = CurrentWeapon;
  DestroyAllLoadoutWeapons();
  BroadcastCurrentWeaponChanged(PreviousWeapon, nullptr, INDEX_NONE);
  UpdateReplicatedWeaponPresenceFromLocalState();
}

bool UCombatComponent::EquipWeaponSlot(int32 SlotIndex) {
  EnsureLoadoutArraySize();
  if (!IsValidLoadoutSlotIndex(SlotIndex)) {
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
    UpdateReplicatedWeaponPresenceFromLocalState();
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
  UpdateCurrentWeaponAimState();

  BroadcastCurrentWeaponChanged(PreviousWeapon, CurrentWeapon,
                                CurrentWeaponSlotIndex);
  UpdateReplicatedWeaponPresenceFromLocalState();
  return true;
}

bool UCombatComponent::EquipNextWeapon() {
  if (CountOccupiedSlots() <= 1) {
    return false;
  }

  const int32 StartSlotIndex = CurrentWeaponSlotIndex == INDEX_NONE ? 0
                                                                    : CurrentWeaponSlotIndex;
  const int32 NextSlotIndex = FindNextOccupiedSlotIndex(StartSlotIndex, 1);
  return IsValidLoadoutSlotIndex(NextSlotIndex) ? EquipWeaponSlot(NextSlotIndex)
                                                : false;
}

bool UCombatComponent::EquipPreviousWeapon() {
  if (CountOccupiedSlots() <= 1) {
    return false;
  }

  const int32 StartSlotIndex =
      CurrentWeaponSlotIndex == INDEX_NONE ? ProjectWeaponLoadout::SlotCount - 1
                                           : CurrentWeaponSlotIndex;
  const int32 PreviousSlotIndex =
      FindNextOccupiedSlotIndex(StartSlotIndex, -1);
  return IsValidLoadoutSlotIndex(PreviousSlotIndex)
             ? EquipWeaponSlot(PreviousSlotIndex)
             : false;
}

bool UCombatComponent::SetWeaponForSlot(EWeaponLoadoutSlot Slot,
                                        TSubclassOf<AWeaponBase> WeaponClass) {
  if (!WeaponClass) {
    return false;
  }

  if (!OwningCharacter) {
    CacheOwnerReferences();
    if (!OwningCharacter) {
      UE_LOG(LogProject, Error, TEXT("CombatComponent: owner is not a Character"));
      return false;
    }
  }

  EnsureLoadoutArraySize();

  const int32 SlotIndex = ProjectWeaponLoadout::ToIndex(static_cast<uint8>(Slot));
  if (!IsValidLoadoutSlotIndex(SlotIndex)) {
    return false;
  }

  const bool bWasCurrentSlot = CurrentWeaponSlotIndex == SlotIndex;
  if (AWeaponBase *ExistingWeapon = SpawnedLoadoutWeapons[SlotIndex]) {
    if (ExistingWeapon == CurrentWeapon) {
      SetWeaponActiveState(ExistingWeapon, false);
      CurrentWeapon = nullptr;
      CurrentWeaponSlotIndex = INDEX_NONE;
    }
    ExistingWeapon->Destroy();
    SpawnedLoadoutWeapons[SlotIndex] = nullptr;
  }

  AWeaponBase *SpawnedWeapon = SpawnWeaponForSlot(SlotIndex, WeaponClass);
  if (!IsValid(SpawnedWeapon)) {
    UpdateReplicatedWeaponPresenceFromLocalState();
    return false;
  }

  if (bWasCurrentSlot || CurrentWeaponSlotIndex == INDEX_NONE) {
    return EquipWeaponSlot(SlotIndex);
  }

  UpdateReplicatedWeaponPresenceFromLocalState();
  return true;
}

void UCombatComponent::ClearWeaponSlot(EWeaponLoadoutSlot Slot) {
  EnsureLoadoutArraySize();

  const int32 SlotIndex = ProjectWeaponLoadout::ToIndex(static_cast<uint8>(Slot));
  if (!IsValidLoadoutSlotIndex(SlotIndex)) {
    return;
  }

  AWeaponBase *WeaponToRemove = SpawnedLoadoutWeapons[SlotIndex];
  if (!IsValid(WeaponToRemove)) {
    return;
  }

  const bool bWasCurrentSlot = CurrentWeapon == WeaponToRemove;
  AWeaponBase *PreviousWeapon = bWasCurrentSlot ? CurrentWeapon : nullptr;

  if (bWasCurrentSlot) {
    StopScope();
    CurrentWeapon->StopFire();
    CurrentWeapon->SetAiming(false);
    SetWeaponActiveState(CurrentWeapon, false);
    CurrentWeapon = nullptr;
    CurrentWeaponSlotIndex = INDEX_NONE;
  }

  WeaponToRemove->Destroy();
  SpawnedLoadoutWeapons[SlotIndex] = nullptr;

  if (bWasCurrentSlot) {
    const int32 FallbackSlotIndex = FindNextOccupiedSlotIndex(SlotIndex, 1);
    if (IsValidLoadoutSlotIndex(FallbackSlotIndex)) {
      EquipWeaponSlot(FallbackSlotIndex);
    } else {
      BroadcastCurrentWeaponChanged(PreviousWeapon, nullptr, INDEX_NONE);
      UpdateReplicatedWeaponPresenceFromLocalState();
    }
  } else {
    UpdateReplicatedWeaponPresenceFromLocalState();
  }
}

bool UCombatComponent::SetActiveLoadoutSlot(EWeaponLoadoutSlot Slot) {
  return EquipWeaponSlot(ProjectWeaponLoadout::ToIndex(static_cast<uint8>(Slot)));
}

bool UCombatComponent::EquipActiveSlot() {
  return IsValidLoadoutSlotIndex(CurrentWeaponSlotIndex)
             ? EquipWeaponSlot(CurrentWeaponSlotIndex)
             : false;
}

AWeaponBase *UCombatComponent::GetWeaponInSlot(EWeaponLoadoutSlot Slot) const {
  const int32 SlotIndex = ProjectWeaponLoadout::ToIndex(static_cast<uint8>(Slot));
  return SpawnedLoadoutWeapons.IsValidIndex(SlotIndex)
             ? SpawnedLoadoutWeapons[SlotIndex]
             : nullptr;
}

bool UCombatComponent::IsLoadoutSlotOccupied(EWeaponLoadoutSlot Slot) const {
  return IsValid(GetWeaponInSlot(Slot));
}

bool UCombatComponent::EquipWeapon(TSubclassOf<AWeaponBase> WeaponClass) {
  if (!WeaponClass) {
    UE_LOG(LogProject, Warning,
           TEXT("CombatComponent: EquipWeapon called with null class"));
    return false;
  }

  const int32 TargetSlotIndex = IsValidLoadoutSlotIndex(CurrentWeaponSlotIndex)
                                    ? CurrentWeaponSlotIndex
                                    : 0;
  return SetWeaponForSlot(static_cast<EWeaponLoadoutSlot>(TargetSlotIndex),
                          WeaponClass);
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

  EnsureLoadoutArraySize();

  const int32 TargetSlotIndex = IsValidLoadoutSlotIndex(CurrentWeaponSlotIndex)
                                    ? CurrentWeaponSlotIndex
                                    : 0;
  const bool bWasCurrentSlot = CurrentWeaponSlotIndex == TargetSlotIndex;

  if (AWeaponBase *ExistingWeapon = SpawnedLoadoutWeapons[TargetSlotIndex]) {
    if (ExistingWeapon == CurrentWeapon) {
      SetWeaponActiveState(ExistingWeapon, false);
      CurrentWeapon = nullptr;
      CurrentWeaponSlotIndex = INDEX_NONE;
    }
    if (ExistingWeapon != NewWeapon) {
      ExistingWeapon->Destroy();
    }
  }

  NewWeapon->SetOwningPawn(OwningCharacter);
  AttachWeaponToOwner(NewWeapon);
  SetWeaponActiveState(NewWeapon, false);
  SpawnedLoadoutWeapons[TargetSlotIndex] = NewWeapon;

  if (bWasCurrentSlot || CurrentWeaponSlotIndex == INDEX_NONE) {
    return EquipWeaponSlot(TargetSlotIndex);
  }

  UpdateReplicatedWeaponPresenceFromLocalState();
  return true;
}

void UCombatComponent::UnequipCurrentWeapon(bool bDestroyWeapon) {
  if (!CurrentWeapon) {
    return;
  }

  StopScope();

  AWeaponBase *PreviousWeapon = CurrentWeapon;
  PreviousWeapon->StopFire();
  PreviousWeapon->SetAiming(false);

  const int32 ExistingIndex = CurrentWeaponSlotIndex;
  if (bDestroyWeapon) {
    PreviousWeapon->Destroy();
  } else {
    PreviousWeapon->DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
    SetWeaponActiveState(PreviousWeapon, false);
  }

  if (IsValidLoadoutSlotIndex(ExistingIndex)) {
    SpawnedLoadoutWeapons[ExistingIndex] = nullptr;
  }

  CurrentWeapon = nullptr;
  CurrentWeaponSlotIndex = INDEX_NONE;

  const int32 FallbackSlotIndex = FindNextOccupiedSlotIndex(
      ExistingIndex == INDEX_NONE ? 0 : ExistingIndex, 1);
  if (IsValidLoadoutSlotIndex(FallbackSlotIndex)) {
    EquipWeaponSlot(FallbackSlotIndex);
  } else {
    BroadcastCurrentWeaponChanged(PreviousWeapon, nullptr, INDEX_NONE);
    UpdateReplicatedWeaponPresenceFromLocalState();
  }
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
  if (bIsScoping) {
    return;
  }

  bIsScoping = true;

  if (CurrentWeapon) {
    CurrentWeapon->SetAiming(true);
  }
}

void UCombatComponent::StopScope() {
  if (!bIsScoping) {
    return;
  }

  bIsScoping = false;

  if (CurrentWeapon) {
    CurrentWeapon->SetAiming(false);
  }
}

bool UCombatComponent::IsCurrentWeaponScopeType() const {
  return false;
}

bool UCombatComponent::IsScopeOverlayActive() const {
  return false;
}

bool UCombatComponent::IsUsingPhysicalScope() const {
  return false;
}

int32 UCombatComponent::GetLoadoutCount() const { return CountOccupiedSlots(); }

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
  EnsureLoadoutArraySize();

  for (AWeaponBase *Weapon : SpawnedLoadoutWeapons) {
    if (IsValid(Weapon)) {
      Weapon->Destroy();
    }
  }

  for (int32 SlotIndex = 0; SlotIndex < SpawnedLoadoutWeapons.Num(); ++SlotIndex) {
    SpawnedLoadoutWeapons[SlotIndex] = nullptr;
  }

  CurrentWeapon = nullptr;
  CurrentWeaponSlotIndex = INDEX_NONE;
}

void UCombatComponent::UpdateReplicatedWeaponPresenceFromLocalState() {
  const AActor *OwnerActor = GetOwner();
  if (!OwnerActor || !OwnerActor->HasAuthority()) {
    return;
  }

  EnsureLoadoutArraySize();
  ReplicatedLoadoutWeaponClasses.SetNumZeroed(ProjectWeaponLoadout::SlotCount);

  for (int32 SlotIndex = 0; SlotIndex < SpawnedLoadoutWeapons.Num(); ++SlotIndex) {
    const AWeaponBase *Weapon = SpawnedLoadoutWeapons[SlotIndex];
    ReplicatedLoadoutWeaponClasses[SlotIndex] =
        IsValid(Weapon) ? Weapon->GetClass() : nullptr;
  }

  ReplicatedCurrentWeaponSlotIndex = CurrentWeaponSlotIndex;

  if (AActor *MutableOwnerActor = GetOwner()) {
    MutableOwnerActor->ForceNetUpdate();
  }
}

void UCombatComponent::OnRep_ReplicatedWeaponPresence() {
  ApplyReplicatedWeaponPresence();
}

void UCombatComponent::ApplyReplicatedWeaponPresence() {
  const AActor *OwnerActor = GetOwner();
  if (OwnerActor && OwnerActor->HasAuthority()) {
    return;
  }

  if (!OwningCharacter) {
    CacheOwnerReferences();
  }
  if (!OwningCharacter) {
    return;
  }

  EnsureLoadoutArraySize();

  TArray<TSubclassOf<AWeaponBase>> DesiredWeaponClasses =
      ReplicatedLoadoutWeaponClasses;
  DesiredWeaponClasses.SetNumZeroed(ProjectWeaponLoadout::SlotCount);

  AWeaponBase *PreviousWeapon = CurrentWeapon;
  for (int32 SlotIndex = 0; SlotIndex < ProjectWeaponLoadout::SlotCount; ++SlotIndex) {
    TSubclassOf<AWeaponBase> DesiredWeaponClass = DesiredWeaponClasses[SlotIndex];
    AWeaponBase *ExistingWeapon = SpawnedLoadoutWeapons[SlotIndex];
    const bool bClassMatches =
        IsValid(ExistingWeapon) && ExistingWeapon->GetClass() == DesiredWeaponClass;

    if (!DesiredWeaponClass) {
      if (IsValid(ExistingWeapon)) {
        if (ExistingWeapon == CurrentWeapon) {
          CurrentWeapon = nullptr;
          CurrentWeaponSlotIndex = INDEX_NONE;
        }
        ExistingWeapon->Destroy();
        SpawnedLoadoutWeapons[SlotIndex] = nullptr;
      }
      continue;
    }

    if (bClassMatches) {
      AttachWeaponToOwner(ExistingWeapon);
      continue;
    }

    if (IsValid(ExistingWeapon)) {
      if (ExistingWeapon == CurrentWeapon) {
        CurrentWeapon = nullptr;
        CurrentWeaponSlotIndex = INDEX_NONE;
      }
      ExistingWeapon->Destroy();
      SpawnedLoadoutWeapons[SlotIndex] = nullptr;
    }

    SpawnWeaponForSlot(SlotIndex, DesiredWeaponClass);
  }

  if (IsValidLoadoutSlotIndex(ReplicatedCurrentWeaponSlotIndex) &&
      IsValid(SpawnedLoadoutWeapons[ReplicatedCurrentWeaponSlotIndex])) {
    EquipWeaponSlot(ReplicatedCurrentWeaponSlotIndex);
    return;
  }

  for (AWeaponBase *Weapon : SpawnedLoadoutWeapons) {
    SetWeaponActiveState(Weapon, false);
  }
  CurrentWeapon = nullptr;
  CurrentWeaponSlotIndex = INDEX_NONE;

  if (PreviousWeapon) {
    BroadcastCurrentWeaponChanged(PreviousWeapon, nullptr, INDEX_NONE);
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

void UCombatComponent::EnsureLoadoutArraySize() {
  if (SpawnedLoadoutWeapons.Num() == ProjectWeaponLoadout::SlotCount) {
    return;
  }

  SpawnedLoadoutWeapons.SetNumZeroed(ProjectWeaponLoadout::SlotCount);
}

int32 UCombatComponent::CountOccupiedSlots() const {
  int32 OccupiedSlotCount = 0;
  for (AWeaponBase *Weapon : SpawnedLoadoutWeapons) {
    if (IsValid(Weapon)) {
      ++OccupiedSlotCount;
    }
  }

  return OccupiedSlotCount;
}

int32 UCombatComponent::FindNextOccupiedSlotIndex(int32 StartSlotIndex,
                                                  int32 Direction) const {
  if (CountOccupiedSlots() <= 0) {
    return INDEX_NONE;
  }

  if (!IsValidLoadoutSlotIndex(StartSlotIndex)) {
    StartSlotIndex = Direction >= 0 ? 0 : ProjectWeaponLoadout::SlotCount - 1;
  }

  for (int32 StepIndex = 1; StepIndex <= ProjectWeaponLoadout::SlotCount;
       ++StepIndex) {
    const int32 CandidateSlotIndex =
        (StartSlotIndex + (Direction * StepIndex) + ProjectWeaponLoadout::SlotCount) %
        ProjectWeaponLoadout::SlotCount;
    if (SpawnedLoadoutWeapons.IsValidIndex(CandidateSlotIndex) &&
        IsValid(SpawnedLoadoutWeapons[CandidateSlotIndex])) {
      return CandidateSlotIndex;
    }
  }

  return INDEX_NONE;
}

AWeaponBase *UCombatComponent::SpawnWeaponForSlot(
    int32 SlotIndex, TSubclassOf<AWeaponBase> WeaponClass) {
  if (!IsValidLoadoutSlotIndex(SlotIndex) || !WeaponClass) {
    return nullptr;
  }

  if (!OwningCharacter) {
    CacheOwnerReferences();
  }
  if (!OwningCharacter) {
    return nullptr;
  }

  UWorld *World = GetWorld();
  if (!World) {
    return nullptr;
  }

  FActorSpawnParameters SpawnParams;
  SpawnParams.Owner = OwningCharacter;
  SpawnParams.Instigator = OwningCharacter;
  SpawnParams.SpawnCollisionHandlingOverride =
      ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

  AWeaponBase *SpawnedWeapon = World->SpawnActor<AWeaponBase>(
      WeaponClass, OwningCharacter->GetActorTransform(), SpawnParams);
  if (!IsValid(SpawnedWeapon)) {
    UE_LOG(LogProject, Warning,
           TEXT("CombatComponent: failed to spawn loadout weapon '%s' for slot %d on '%s'"),
           *GetNameSafe(WeaponClass.Get()), SlotIndex, *GetNameSafe(GetOwner()));
    return nullptr;
  }

  SpawnedWeapon->SetOwningPawn(OwningCharacter);
  AttachWeaponToOwner(SpawnedWeapon);
  SetWeaponActiveState(SpawnedWeapon, false);
  SpawnedLoadoutWeapons[SlotIndex] = SpawnedWeapon;
  return SpawnedWeapon;
}

void UCombatComponent::UpdateCurrentWeaponAimState() {
  if (!CurrentWeapon) {
    return;
  }

  CurrentWeapon->SetAiming(bIsScoping);
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

  if (bPreferFirstPersonMesh && OwningCharacter->IsLocallyControlled()) {
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

      for (USkeletalMeshComponent *MeshComp : MeshComponents) {
        if (IsValid(MeshComp) && MeshComp->IsAttachedTo(CameraComp)) {
          return MeshComp;
        }
      }
    }

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
