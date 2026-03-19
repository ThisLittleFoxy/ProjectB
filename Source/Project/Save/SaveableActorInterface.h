#pragma once

#include "CoreMinimal.h"
#include "Save/ProjectSaveTypes.h"
#include "UObject/Interface.h"
#include "SaveableActorInterface.generated.h"

UINTERFACE(BlueprintType)
class PROJECT_API USaveableActorInterface : public UInterface {
  GENERATED_BODY()
};

class PROJECT_API ISaveableActorInterface {
  GENERATED_BODY()

public:
  UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Save")
  void GatherSaveCustomData(FSaveableActorCustomData &OutCustomData) const;

  UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Save")
  void ApplySaveCustomData(const FSaveableActorCustomData &InCustomData);
};
