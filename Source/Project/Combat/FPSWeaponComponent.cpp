#include "Combat/FPSWeaponComponent.h"

#include "Combat/HealthComponent.h"
#include "Combat/WeaponBase.h"
#include "Components/SkeletalMeshComponent.h"
#include "EnhancedInputSubsystems.h"
#include "GameFramework/Character.h"
#include "GameFramework/PlayerController.h"
#include "InputMappingContext.h"
#include "Net/UnrealNetwork.h"

UFPSWeaponComponent::UFPSWeaponComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
    SetIsReplicatedByDefault(true);
}

void UFPSWeaponComponent::BeginPlay()
{
    Super::BeginPlay();

    if (UHealthComponent* HealthComp = GetOwner()->FindComponentByClass<UHealthComponent>())
    {
        HealthComp->OnDeath.AddDynamic(this, &UFPSWeaponComponent::HandleOwnerDeath);
    }
}

void UFPSWeaponComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    StopFire();
    Super::EndPlay(EndPlayReason);
}

void UFPSWeaponComponent::EquipDefaultWeapon()
{
    ServerEquipDefaultWeapon();
}

void UFPSWeaponComponent::ServerEquipDefaultWeapon_Implementation()
{
    ACharacter* OwnerCharacter = Cast<ACharacter>(GetOwner());
    if (!OwnerCharacter || !DefaultWeaponClass || CurrentWeapon)
    {
        return;
    }

    FActorSpawnParameters Params;
    Params.Owner = OwnerCharacter;
    Params.Instigator = OwnerCharacter;
    Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

    AWeaponBase* NewWeapon = GetWorld()->SpawnActor<AWeaponBase>(DefaultWeaponClass, Params);
    if (!NewWeapon)
    {
        return;
    }

    CurrentWeapon = NewWeapon;
    CurrentWeapon->SetOwner(OwnerCharacter);
    AttachWeaponToOwnerMesh(CurrentWeapon);
}

void UFPSWeaponComponent::StartFire()
{
    if (CurrentWeapon)
    {
        CurrentWeapon->StartFire();
    }
}

void UFPSWeaponComponent::StopFire()
{
    if (CurrentWeapon)
    {
        CurrentWeapon->StopFire();
    }
}

void UFPSWeaponComponent::AddDefaultInputMappingForController(APlayerController* PlayerController, int32 Priority)
{
    if (!PlayerController || !DefaultInputMapping)
    {
        return;
    }

    if (ULocalPlayer* LP = PlayerController->GetLocalPlayer())
    {
        if (UEnhancedInputLocalPlayerSubsystem* Subsystem = LP->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>())
        {
            Subsystem->AddMappingContext(DefaultInputMapping, Priority);
        }
    }
}

void UFPSWeaponComponent::OnRep_CurrentWeapon()
{
    AttachWeaponToOwnerMesh(CurrentWeapon);
}

void UFPSWeaponComponent::AttachWeaponToOwnerMesh(AWeaponBase* WeaponToAttach) const
{
    ACharacter* OwnerCharacter = Cast<ACharacter>(GetOwner());
    if (!OwnerCharacter || !WeaponToAttach || !OwnerCharacter->GetMesh())
    {
        return;
    }

    WeaponToAttach->AttachToComponent(OwnerCharacter->GetMesh(), FAttachmentTransformRules::SnapToTargetNotIncludingScale, WeaponAttachSocket);
}

void UFPSWeaponComponent::HandleOwnerDeath()
{
    StopFire();
}

void UFPSWeaponComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    DOREPLIFETIME(UFPSWeaponComponent, CurrentWeapon);
}
