#pragma once

#include "CoreMinimal.h"
#include "GameFramework/CheatManager.h"
#include "ProjectCheatManager.generated.h"

/**
 * Project cheat manager for debug commands
 */
UCLASS()
class PROJECT_API UProjectCheatManager : public UCheatManager {
  GENERATED_BODY()

public:
  /** Show player info (location, health, etc.) */
  UFUNCTION(Exec, Category = "Debug")
  void PlayerInfo();

  /** Teleport player to specified coordinates */
  UFUNCTION(Exec, Category = "Debug")
  void TeleportTo(float X, float Y, float Z);

  /** Toggle god mode */
  UFUNCTION(Exec, Category = "Debug")
  void GodMode();
};
