#pragma once

#include "CoreMinimal.h"
#include "GameFramework/CheatManager.h"
#include "ProjectCheatManager.generated.h"

class APawn;

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

  /** Spawn a damage test cube in front of the player */
  UFUNCTION(Exec, Category = "Debug")
  void SpawnDamageCube();

  /** Backward-compatible alias for SpawnDamageCube */
  UFUNCTION(Exec, Category = "Debug")
  void SpawnPlayerCopy();

  /** Apply damage to actor under crosshair */
  UFUNCTION(Exec, Category = "Debug")
  void DamageCrosshair(float DamageAmount = 25.0f);

  /** Instantly kill actor under crosshair */
  UFUNCTION(Exec, Category = "Debug")
  void KillCrosshair();

  /** Instantly kill the controlled pawn for arena death flow testing */
  UFUNCTION(Exec, Category = "Debug")
  void ArenaKillSelf();

private:
  APawn *GetControlledPawn() const;
};
