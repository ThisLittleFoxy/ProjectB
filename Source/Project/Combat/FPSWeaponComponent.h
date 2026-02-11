#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "FPSWeaponComponent.generated.h"

class AWeaponBase;
class UHealthComponent;
class UInputMappingContext;
class APlayerController;

UCLASS(ClassGroup=(Combat), BlueprintType, Blueprintable, meta=(BlueprintSpawnableComponent))
class PROJECT_API UFPSWeaponComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UFPSWeaponComponent();

    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

    UFUNCTION(BlueprintCallable, Category="Weapon")
    void EquipDefaultWeapon();

    UFUNCTION(BlueprintCallable, Category="Weapon")
    void StartFire();

    UFUNCTION(BlueprintCallable, Category="Weapon")
    void StopFire();

    UFUNCTION(BlueprintCallable, Category="Input")
    void AddDefaultInputMappingForController(APlayerController* PlayerController, int32 Priority = 0);

    UFUNCTION(BlueprintPure, Category="Weapon")
    AWeaponBase* GetCurrentWeapon() const { return CurrentWeapon; }

protected:
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Weapon")
    TSubclassOf<AWeaponBase> DefaultWeaponClass;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Weapon")
    FName WeaponAttachSocket = TEXT("WeaponSocket");

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Input")
    TObjectPtr<UInputMappingContext> DefaultInputMapping;

    UPROPERTY(ReplicatedUsing=OnRep_CurrentWeapon, VisibleInstanceOnly, BlueprintReadOnly, Category="Weapon")
    TObjectPtr<AWeaponBase> CurrentWeapon;

    UFUNCTION(Server, Reliable)
    void ServerEquipDefaultWeapon();

    UFUNCTION()
    void OnRep_CurrentWeapon();

    UFUNCTION()
    void HandleOwnerDeath();

    void AttachWeaponToOwnerMesh(AWeaponBase* WeaponToAttach) const;
};
