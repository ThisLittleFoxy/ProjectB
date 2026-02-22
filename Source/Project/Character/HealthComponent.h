// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Components/ActorComponent.h"
#include "Math/Color.h"
#include "HealthComponent.generated.h"

class AActor;
class AController;
class UDamageType;
class UHealthComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_FourParams(FHealthChangedSignature,
                                              UHealthComponent *,
                                              HealthComponent, float,
                                              CurrentHealth, float, MaxHealth,
                                              float, DeltaHealth);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOutOfHealthSignature,
                                            UHealthComponent *,
                                            HealthComponent);

/**
 * Reusable health container:
 * - reacts to owner OnTakeAnyDamage
 * - supports heal/damage API for Blueprint and C++
 * - broadcasts health/death state changes for HUD and gameplay logic
 */
UCLASS(ClassGroup = (Character), meta = (BlueprintSpawnableComponent))
class PROJECT_API UHealthComponent : public UActorComponent {
  GENERATED_BODY()

public:
  UHealthComponent();

  virtual void BeginPlay() override;

  UPROPERTY(BlueprintAssignable, Category = "Health|Events")
  FHealthChangedSignature OnHealthChanged;

  UPROPERTY(BlueprintAssignable, Category = "Health|Events")
  FOutOfHealthSignature OnOutOfHealth;

  /** Apply incoming damage and return actually applied value */
  UFUNCTION(BlueprintCallable, Category = "Health")
  float ApplyDamage(float DamageAmount);

  /** Apply healing and return actually restored value */
  UFUNCTION(BlueprintCallable, Category = "Health")
  float ApplyHealing(float HealAmount);

  UFUNCTION(BlueprintCallable, Category = "Health")
  void RestoreFullHealth();

  UFUNCTION(BlueprintPure, Category = "Health")
  float GetCurrentHealth() const { return CurrentHealth; }

  UFUNCTION(BlueprintPure, Category = "Health")
  float GetMaxHealth() const { return MaxHealth; }

  UFUNCTION(BlueprintPure, Category = "Health")
  float GetHealthPercent() const;

  UFUNCTION(BlueprintPure, Category = "Health")
  bool IsAlive() const { return CurrentHealth > 0.0f; }

protected:
  UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Health",
            meta = (ClampMin = "1.0"))
  float MaxHealth = 100.0f;

  UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Health",
            meta = (ClampMin = "0.0"))
  float CurrentHealth = 100.0f;

  /**
   * If true, component starts with CurrentHealth = MaxHealth at BeginPlay.
   * If false, keeps the authored CurrentHealth value (clamped).
   */
  UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Health")
  bool bInitializeFromMaxHealthOnBeginPlay = true;

  /** Hide actor and disable collision immediately when HP reaches 0 */
  UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Health|Death")
  bool bHideActorOnDeath = true;

  /** Destroy owner automatically when HP reaches 0 */
  UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Health|Death")
  bool bDestroyOwnerOnDeath = true;

  /**
   * Optional delay before actor destroy after death.
   * Useful if you want a short death effect before actor removal.
   */
  UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Health|Death",
            meta = (ClampMin = "0.0", EditCondition = "bDestroyOwnerOnDeath"))
  float DestroyDelayOnDeath = 0.0f;

  /** If true, instigator receives currency reward when this actor dies */
  UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Health|Rewards")
  bool bGrantCurrencyOnDeath = false;

  UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Health|Rewards",
            meta = (ClampMin = "0", EditCondition = "bGrantCurrencyOnDeath"))
  int32 CurrencyRewardOnDeath = 10;

  /** Show incoming damage on screen (debug) */
  UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Health|Debug")
  bool bShowDamageOnScreen = false;

  UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Health|Debug",
            meta = (ClampMin = "0.0", EditCondition = "bShowDamageOnScreen"))
  float DamageScreenMessageDuration = 1.5f;

  UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Health|Debug",
            meta = (EditCondition = "bShowDamageOnScreen"))
  FColor DamageScreenMessageColor = FColor::Red;

private:
  UFUNCTION()
  void HandleOwnerTakeAnyDamage(AActor *DamagedActor, float Damage,
                                const UDamageType *DamageType,
                                AController *InstigatedBy,
                                AActor *DamageCauser);

  void HandleDeath();
  void GrantDeathCurrencyReward(AController *InstigatedBy, AActor *DamageCauser);

  float SetHealth(float NewHealth);

  UPROPERTY(Transient)
  bool bOutOfHealthNotified = false;
};
