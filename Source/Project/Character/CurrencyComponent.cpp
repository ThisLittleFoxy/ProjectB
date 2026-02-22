// Copyright Epic Games, Inc. All Rights Reserved.

#include "Character/CurrencyComponent.h"

UCurrencyComponent::UCurrencyComponent() { PrimaryComponentTick.bCanEverTick = false; }

int32 UCurrencyComponent::AddCurrency(int32 Amount) {
  if (Amount <= 0) {
    return 0;
  }

  const int32 PreviousCurrency = CurrentCurrency;
  CurrentCurrency = FMath::Max(0, CurrentCurrency + Amount);
  const int32 DeltaCurrency = CurrentCurrency - PreviousCurrency;
  if (DeltaCurrency != 0) {
    OnCurrencyChanged.Broadcast(this, CurrentCurrency, DeltaCurrency);
  }

  return DeltaCurrency;
}

bool UCurrencyComponent::SpendCurrency(int32 Amount) {
  if (Amount <= 0 || CurrentCurrency < Amount) {
    return false;
  }

  const int32 PreviousCurrency = CurrentCurrency;
  CurrentCurrency = FMath::Max(0, CurrentCurrency - Amount);
  const int32 DeltaCurrency = CurrentCurrency - PreviousCurrency;
  if (DeltaCurrency != 0) {
    OnCurrencyChanged.Broadcast(this, CurrentCurrency, DeltaCurrency);
  }

  return true;
}
