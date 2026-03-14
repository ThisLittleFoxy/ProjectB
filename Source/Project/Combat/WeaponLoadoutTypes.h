// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/Texture2D.h"
#include "Templates/SubclassOf.h"
#include "WeaponLoadoutTypes.generated.h"

class AWeaponBase;

namespace ProjectWeaponLoadout {
constexpr int32 SlotCount = 3;

FORCEINLINE int32 ToIndex(const uint8 SlotValue) { return static_cast<int32>(SlotValue); }
} // namespace ProjectWeaponLoadout

UENUM(BlueprintType)
enum class EWeaponEquipGroup : uint8 {
  Primary UMETA(DisplayName = "Primary"),
  Auxiliary UMETA(DisplayName = "Auxiliary")
};

UENUM(BlueprintType)
enum class EWeaponLoadoutSlot : uint8 {
  Slot1Primary UMETA(DisplayName = "Slot 1"),
  Slot2Secondary UMETA(DisplayName = "Slot 2"),
  Slot3Auxiliary UMETA(DisplayName = "Slot 3")
};

USTRUCT(BlueprintType)
struct PROJECT_API FWeaponShopOffer {
  GENERATED_BODY()

  UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Shop")
  TSubclassOf<AWeaponBase> WeaponClass;

  UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Shop")
  bool bOverridePrice = false;

  UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Shop",
            meta = (ClampMin = "0", EditCondition = "bOverridePrice"))
  int32 Price = 0;

  UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Shop")
  int32 SortOrder = 0;

  UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Shop")
  FText OverrideDisplayName;

  UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Shop")
  TObjectPtr<UTexture2D> OverrideIcon = nullptr;
};
