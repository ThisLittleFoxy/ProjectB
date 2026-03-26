#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "ProjectProfileSaveGame.generated.h"

UCLASS()
class PROJECT_API UProjectProfileSaveGame : public USaveGame {
  GENERATED_BODY()

public:
  static constexpr int32 CurrentSchemaVersion = 1;

  UPROPERTY(VisibleAnywhere, BlueprintReadOnly, SaveGame, Category = "Save")
  int32 SaveSchemaVersion = CurrentSchemaVersion;

  UPROPERTY(VisibleAnywhere, BlueprintReadOnly, SaveGame, Category = "Save")
  FDateTime SavedAtUtc;
};
