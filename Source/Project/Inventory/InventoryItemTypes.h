// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Combat/WeaponLoadoutTypes.h"
#include "CoreMinimal.h"
#include "InventoryItemTypes.generated.h"

class AWeaponBase;
class UTexture2D;

UENUM(BlueprintType)
enum class EInventoryItemKind : uint8 {
  None UMETA(DisplayName = "None"),
  Weapon UMETA(DisplayName = "Weapon"),
  Generic UMETA(DisplayName = "Generic")
};

UENUM(BlueprintType)
enum class EInventoryItemContainer : uint8 {
  StorageGrid UMETA(DisplayName = "Storage Grid"),
  LoadoutSlot UMETA(DisplayName = "Loadout Slot")
};

USTRUCT(BlueprintType)
struct PROJECT_API FInventoryGridPlacement {
  GENERATED_BODY()

  UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Inventory")
  FIntPoint TopLeft = FIntPoint::ZeroValue;

  UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Inventory")
  bool bRotated = false;
};

USTRUCT(BlueprintType)
struct PROJECT_API FInventoryItemInstance {
  GENERATED_BODY()

  UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Inventory")
  FGuid ItemId;

  UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Inventory")
  EInventoryItemKind ItemKind = EInventoryItemKind::None;

  UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Inventory")
  EInventoryItemContainer Container = EInventoryItemContainer::StorageGrid;

  UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Inventory")
  TSubclassOf<AWeaponBase> WeaponClass;

  UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Inventory")
  FText DisplayName;

  UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Inventory")
  TObjectPtr<UTexture2D> Icon = nullptr;

  UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Inventory",
            meta = (ClampMin = "0.0"))
  float Weight = 0.0f;

  UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Inventory")
  FIntPoint Footprint = FIntPoint(1, 1);

  UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Inventory")
  bool bCanRotate = false;

  UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Inventory")
  FInventoryGridPlacement GridPlacement;

  UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Inventory")
  EWeaponLoadoutSlot LoadoutSlot = EWeaponLoadoutSlot::Slot1Primary;
};

USTRUCT(BlueprintType)
struct PROJECT_API FInventoryItemViewData {
  GENERATED_BODY()

  UPROPERTY(BlueprintReadOnly, Category = "Inventory")
  FGuid ItemId;

  UPROPERTY(BlueprintReadOnly, Category = "Inventory")
  EInventoryItemKind ItemKind = EInventoryItemKind::None;

  UPROPERTY(BlueprintReadOnly, Category = "Inventory")
  EInventoryItemContainer Container = EInventoryItemContainer::StorageGrid;

  UPROPERTY(BlueprintReadOnly, Category = "Inventory")
  TSubclassOf<AWeaponBase> WeaponClass;

  UPROPERTY(BlueprintReadOnly, Category = "Inventory")
  FText DisplayName;

  UPROPERTY(BlueprintReadOnly, Category = "Inventory")
  TObjectPtr<UTexture2D> Icon = nullptr;

  UPROPERTY(BlueprintReadOnly, Category = "Inventory")
  float Weight = 0.0f;

  UPROPERTY(BlueprintReadOnly, Category = "Inventory")
  FIntPoint BaseFootprint = FIntPoint(1, 1);

  UPROPERTY(BlueprintReadOnly, Category = "Inventory")
  FIntPoint OccupiedFootprint = FIntPoint(1, 1);

  UPROPERTY(BlueprintReadOnly, Category = "Inventory")
  bool bCanRotate = false;

  UPROPERTY(BlueprintReadOnly, Category = "Inventory")
  bool bIsRotated = false;

  UPROPERTY(BlueprintReadOnly, Category = "Inventory")
  FIntPoint GridPosition = FIntPoint(-1, -1);

  UPROPERTY(BlueprintReadOnly, Category = "Inventory")
  EWeaponLoadoutSlot LoadoutSlot = EWeaponLoadoutSlot::Slot1Primary;
};
