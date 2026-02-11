#include "Combat/HealthComponent.h"

#include "GameFramework/Actor.h"
#include "Net/UnrealNetwork.h"

UHealthComponent::UHealthComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
    SetIsReplicatedByDefault(true);
}

void UHealthComponent::BeginPlay()
{
    Super::BeginPlay();

    Health = MaxHealth;

    if (GetOwner() && GetOwner()->HasAuthority())
    {
        GetOwner()->OnTakeAnyDamage.AddDynamic(this, &UHealthComponent::HandleTakeAnyDamage);
    }
}

void UHealthComponent::HandleTakeAnyDamage(AActor* DamagedActor, float Damage, const UDamageType* DamageType,
    AController* InstigatedBy, AActor* DamageCauser)
{
    if (Damage <= 0.0f || !IsAlive())
    {
        return;
    }

    const float PreviousHealth = Health;
    Health = FMath::Clamp(Health - Damage, 0.0f, MaxHealth);

    OnHealthChanged.Broadcast(Health, Health - PreviousHealth);

    if (Health <= 0.0f)
    {
        OnDeath.Broadcast();
    }
}

void UHealthComponent::OnRep_Health(float PreviousHealth)
{
    OnHealthChanged.Broadcast(Health, Health - PreviousHealth);

    if (PreviousHealth > 0.0f && Health <= 0.0f)
    {
        OnDeath.Broadcast();
    }
}

void UHealthComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    DOREPLIFETIME(UHealthComponent, Health);
}
