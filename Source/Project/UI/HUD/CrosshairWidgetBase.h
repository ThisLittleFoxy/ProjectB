// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Blueprint/UserWidget.h"
#include "GameplayTagContainer.h"
#include "CrosshairWidgetBase.generated.h"

class APawn;
class UCombatComponent;
class UCurrencyComponent;
class UHealthComponent;
class UTexture2D;

/**
 * Base widget for player HUD.
 * Exposes direct bindings for ammo/health/crosshair state without EventGraph.
 */
UCLASS(BlueprintType, Blueprintable)
class PROJECT_API UCrosshairWidgetBase : public UUserWidget {
  GENERATED_BODY()

public:
  /** X: ammo in current magazine */
  UFUNCTION(BlueprintPure, Category = "HUD|Ammo")
  int32 GetAmmoMagazine() const;

  /** Reserve ammo only (without current magazine) */
  UFUNCTION(BlueprintPure, Category = "HUD|Ammo")
  int32 GetAmmoReserve() const;

  UFUNCTION(BlueprintPure, Category = "HUD|Health")
  float GetCurrentHealth() const;

  UFUNCTION(BlueprintPure, Category = "HUD|Health")
  float GetMaxHealth() const;

  UFUNCTION(BlueprintPure, Category = "HUD|Health")
  float GetHealthPercent() const;

  UFUNCTION(BlueprintPure, Category = "HUD|Health")
  bool IsAlive() const;

  UFUNCTION(BlueprintPure, Category = "HUD|Crosshair")
  bool IsScoping() const;

  UFUNCTION(BlueprintPure, Category = "HUD|Crosshair")
  bool ShouldShowCrosshair() const;

  /** True when ADS is active with scope-tag weapon (e.g. sniper) */
  UFUNCTION(BlueprintPure, Category = "HUD|Crosshair")
  bool ShouldShowSniperScopeOverlay() const;

  UFUNCTION(BlueprintPure, Category = "HUD|Money")
  int32 GetMoney() const;

  UFUNCTION(BlueprintPure, Category = "HUD|Money")
  int32 GetCurrency() const { return GetMoney(); }

  UFUNCTION(BlueprintPure, Category = "HUD|Weapon")
  UTexture2D *GetCurrentWeaponIcon() const;

  UFUNCTION(BlueprintPure, Category = "HUD|Weapon")
  FGameplayTag GetCurrentWeaponTypeTag() const;

private:
  void RefreshCachedComponents() const;

  UCombatComponent *ResolveCombatComponent() const;
  UCurrencyComponent *ResolveCurrencyComponent() const;
  UHealthComponent *ResolveHealthComponent() const;

  UPROPERTY(Transient)
  mutable TWeakObjectPtr<APawn> CachedOwningPawn;

  UPROPERTY(Transient)
  mutable TWeakObjectPtr<UCombatComponent> CachedCombatComponent;

  UPROPERTY(Transient)
  mutable TWeakObjectPtr<UCurrencyComponent> CachedCurrencyComponent;

  UPROPERTY(Transient)
  mutable TWeakObjectPtr<UHealthComponent> CachedHealthComponent;
};
