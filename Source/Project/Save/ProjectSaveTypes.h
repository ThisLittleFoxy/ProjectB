#pragma once

#include "CoreMinimal.h"
#include "Combat/WeaponBase.h"
#include "GameFramework/Actor.h"
#include "Inventory/InventoryItemTypes.h"
#include "Save/WeaponAmmoSaveData.h"
#include "UObject/SoftObjectPtr.h"
#include "ProjectSaveTypes.generated.h"

class AActor;

UENUM(BlueprintType)
enum class EProjectSaveSlotKind : uint8 {
  Quick UMETA(DisplayName = "Quick"),
  Manual UMETA(DisplayName = "Manual")
};

UENUM(BlueprintType)
enum class EProjectWorldSavePolicy : uint8 {
  None UMETA(DisplayName = "None"),
  GameplayCritical UMETA(DisplayName = "Gameplay Critical"),
  PersistentEnemy UMETA(DisplayName = "Persistent Enemy"),
  CustomComponentOnly UMETA(DisplayName = "Custom Component Only")
};

USTRUCT(BlueprintType)
struct PROJECT_API FProjectWorldSaveRule {
  GENERATED_BODY()

  // First matching rule wins. If no rule matches, the actor is not persisted.
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Config, Category = "Save")
  TSoftClassPtr<AActor> ActorClass;

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Config, Category = "Save")
  EProjectWorldSavePolicy SavePolicy = EProjectWorldSavePolicy::None;

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Config, Category = "Save")
  bool bIncludeDerivedClasses = true;

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Config, Category = "Save")
  bool bSaveHealthState = false;

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Config, Category = "Save")
  bool bSaveDestroyedState = false;

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Config, Category = "Save")
  bool bSaveTransform = false;

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Config, Category = "Save")
  bool bSaveCustomData = false;
};

USTRUCT(BlueprintType)
struct PROJECT_API FSaveableActorCustomData {
  GENERATED_BODY()

  UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category = "Save")
  TMap<FName, bool> BoolFlags;

  UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category = "Save")
  TMap<FName, int32> IntFlags;

  UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category = "Save")
  TMap<FName, float> FloatFlags;

  bool IsEmpty() const {
    return BoolFlags.IsEmpty() && IntFlags.IsEmpty() && FloatFlags.IsEmpty();
  }

  bool IsEquivalentTo(const FSaveableActorCustomData &Other) const {
    auto AreBoolMapsEqual = [](const TMap<FName, bool> &Left,
                               const TMap<FName, bool> &Right) {
      if (Left.Num() != Right.Num()) {
        return false;
      }

      for (const TPair<FName, bool> &Pair : Left) {
        const bool *OtherValue = Right.Find(Pair.Key);
        if (!OtherValue || *OtherValue != Pair.Value) {
          return false;
        }
      }

      return true;
    };

    auto AreIntMapsEqual = [](const TMap<FName, int32> &Left,
                              const TMap<FName, int32> &Right) {
      if (Left.Num() != Right.Num()) {
        return false;
      }

      for (const TPair<FName, int32> &Pair : Left) {
        const int32 *OtherValue = Right.Find(Pair.Key);
        if (!OtherValue || *OtherValue != Pair.Value) {
          return false;
        }
      }

      return true;
    };

    auto AreFloatMapsEqual = [](const TMap<FName, float> &Left,
                                const TMap<FName, float> &Right) {
      if (Left.Num() != Right.Num()) {
        return false;
      }

      for (const TPair<FName, float> &Pair : Left) {
        const float *OtherValue = Right.Find(Pair.Key);
        if (!OtherValue ||
            !FMath::IsNearlyEqual(*OtherValue, Pair.Value, KINDA_SMALL_NUMBER)) {
          return false;
        }
      }

      return true;
    };

    return AreBoolMapsEqual(BoolFlags, Other.BoolFlags) &&
           AreIntMapsEqual(IntFlags, Other.IntFlags) &&
           AreFloatMapsEqual(FloatFlags, Other.FloatFlags);
  }
};

USTRUCT(BlueprintType)
struct PROJECT_API FRunMetaSaveData {
  GENERATED_BODY()

  UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category = "Save")
  FString BuildVersion;

  UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category = "Save")
  int64 TotalPlayTimeSeconds = 0;
};

USTRUCT(BlueprintType)
struct PROJECT_API FArmoryItemSaveData {
  GENERATED_BODY()

  UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category = "Save")
  FGuid ItemId;

  UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category = "Save")
  TSoftClassPtr<AWeaponBase> WeaponClass;

  UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category = "Save")
  EInventoryItemContainer Container = EInventoryItemContainer::StorageGrid;

  UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category = "Save")
  FInventoryGridPlacement GridPlacement;

  UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category = "Save")
  EWeaponLoadoutSlot LoadoutSlot = EWeaponLoadoutSlot::Slot1Primary;

  UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category = "Save")
  FWeaponAmmoSaveData AmmoData;
};

USTRUCT(BlueprintType)
struct PROJECT_API FPlayerSaveData {
  GENERATED_BODY()

  UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category = "Save")
  FString SavedMapName;

  UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category = "Save")
  FTransform PawnTransform = FTransform::Identity;

  UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category = "Save")
  FRotator ControlRotation = FRotator::ZeroRotator;

  UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category = "Save")
  bool bHasHealthState = false;

  UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category = "Save")
  float CurrentHealth = 0.0f;

  UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category = "Save")
  int32 Currency = 0;

  UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category = "Save",
            meta = (ClampMin = "1"))
  int32 StorageGridWidth = 1;

  UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category = "Save",
            meta = (ClampMin = "1"))
  int32 StorageGridHeight = 1;

  UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category = "Save")
  EWeaponLoadoutSlot ActiveSlot = EWeaponLoadoutSlot::Slot1Primary;

  UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category = "Save")
  TArray<FArmoryItemSaveData> ArmoryItems;
};

USTRUCT(BlueprintType)
struct PROJECT_API FWorldActorSaveData {
  GENERATED_BODY()

  UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category = "Save")
  FGuid PersistentId;

  UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category = "Save")
  bool bDestroyedOrDead = false;

  UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category = "Save")
  bool bHasHealthState = false;

  UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category = "Save")
  float CurrentHealth = 0.0f;

  UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category = "Save")
  bool bHasTransformState = false;

  UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category = "Save")
  FTransform ActorTransform = FTransform::Identity;

  UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category = "Save")
  FSaveableActorCustomData CustomData;
};
