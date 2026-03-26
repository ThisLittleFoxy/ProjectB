#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "Save/ProjectSaveTypes.h"
#include "ProjectSaveIndex.generated.h"

USTRUCT(BlueprintType)
struct PROJECT_API FProjectSaveSlotMetadata {
  GENERATED_BODY()

  UPROPERTY(VisibleAnywhere, BlueprintReadOnly, SaveGame, Category = "Save")
  FString SlotName;

  UPROPERTY(VisibleAnywhere, BlueprintReadOnly, SaveGame, Category = "Save")
  EProjectSaveSlotKind SlotKind = EProjectSaveSlotKind::Quick;

  UPROPERTY(VisibleAnywhere, BlueprintReadOnly, SaveGame, Category = "Save")
  FDateTime SavedAtUtc;

  UPROPERTY(VisibleAnywhere, BlueprintReadOnly, SaveGame, Category = "Save")
  FString MapName;

  UPROPERTY(VisibleAnywhere, BlueprintReadOnly, SaveGame, Category = "Save")
  FRunMetaSaveData RunMeta;
};

UCLASS()
class PROJECT_API UProjectSaveIndex : public USaveGame {
  GENERATED_BODY()

public:
  UPROPERTY(VisibleAnywhere, BlueprintReadOnly, SaveGame, Category = "Save")
  TArray<FProjectSaveSlotMetadata> SaveSlots;
};
