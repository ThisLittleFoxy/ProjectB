#pragma once

#include "CoreMinimal.h"
#include "WeaponAmmoSaveData.generated.h"

USTRUCT(BlueprintType)
struct PROJECT_API FWeaponAmmoSaveData {
  GENERATED_BODY()

  UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category = "Save")
  int32 AmmoInMagazine = 0;

  UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category = "Save")
  int32 ReserveAmmo = 0;
};
