// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Components/ActorComponent.h"
#include "CurrencyComponent.generated.h"

class UCurrencyComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FCurrencyChangedSignature,
                                               UCurrencyComponent *,
                                               CurrencyComponent, int32,
                                               CurrentCurrency, int32,
                                               DeltaCurrency);

/**
 * Simple wallet component for actors (player, AI, etc.).
 */
UCLASS(ClassGroup = (Character), meta = (BlueprintSpawnableComponent))
class PROJECT_API UCurrencyComponent : public UActorComponent {
  GENERATED_BODY()

public:
  UCurrencyComponent();

  UPROPERTY(BlueprintAssignable, Category = "Currency|Events")
  FCurrencyChangedSignature OnCurrencyChanged;

  UFUNCTION(BlueprintCallable, Category = "Currency")
  int32 AddCurrency(int32 Amount);

  UFUNCTION(BlueprintCallable, Category = "Currency")
  bool SpendCurrency(int32 Amount);

  UFUNCTION(BlueprintPure, Category = "Currency")
  int32 GetCurrency() const { return CurrentCurrency; }

protected:
  UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Currency",
            meta = (ClampMin = "0"))
  int32 CurrentCurrency = 0;
};
