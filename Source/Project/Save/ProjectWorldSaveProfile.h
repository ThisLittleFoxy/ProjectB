#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "Save/ProjectSaveTypes.h"
#include "ProjectWorldSaveProfile.generated.h"

UCLASS(Config = Game, DefaultConfig, meta = (DisplayName = "World Save Profile"))
class PROJECT_API UProjectWorldSaveProfile : public UDeveloperSettings {
  GENERATED_BODY()

public:
  virtual FName GetCategoryName() const override;

  UPROPERTY(EditAnywhere, Config, BlueprintReadOnly, Category = "World Save")
  TArray<FProjectWorldSaveRule> WorldRules;
};
