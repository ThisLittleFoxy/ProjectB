#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "Save/ProjectSaveTypes.h"
#include "ProjectSaveGame.generated.h"

UCLASS()
class PROJECT_API UProjectSaveGame : public USaveGame {
  GENERATED_BODY()

public:
  static constexpr int32 CurrentSchemaVersion = 1;

  UPROPERTY(VisibleAnywhere, BlueprintReadOnly, SaveGame, Category = "Save")
  int32 SaveSchemaVersion = CurrentSchemaVersion;

  UPROPERTY(VisibleAnywhere, BlueprintReadOnly, SaveGame, Category = "Save")
  EProjectSaveSlotKind SlotKind = EProjectSaveSlotKind::Quick;

  UPROPERTY(VisibleAnywhere, BlueprintReadOnly, SaveGame, Category = "Save")
  FDateTime SavedAtUtc;

  UPROPERTY(VisibleAnywhere, BlueprintReadOnly, SaveGame, Category = "Save")
  FPlayerSaveData PlayerData;

  UPROPERTY(VisibleAnywhere, BlueprintReadOnly, SaveGame, Category = "Save")
  TArray<FWorldActorSaveData> WorldActorRecords;
};
