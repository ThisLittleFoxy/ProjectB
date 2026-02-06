// ProjectCheatManager.cpp

#include "ProjectCheatManager.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/Pawn.h"

void UProjectCheatManager::PlayerInfo() {
  APlayerController* PC = GetOuterAPlayerController();
  if (!PC) {
    return;
  }

  APawn* Pawn = PC->GetPawn();
  if (!Pawn) {
    GEngine->AddOnScreenDebugMessage(-1, 3.0f, FColor::Red, TEXT("No pawn!"));
    return;
  }

  FVector Location = Pawn->GetActorLocation();
  FRotator Rotation = Pawn->GetActorRotation();

  FString Info = FString::Printf(
      TEXT("=== PLAYER INFO ===\nLocation: (%.1f, %.1f, %.1f)\nRotation: (%.1f, %.1f, %.1f)"),
      Location.X, Location.Y, Location.Z,
      Rotation.Pitch, Rotation.Yaw, Rotation.Roll);

  UE_LOG(LogTemp, Log, TEXT("%s"), *Info);
  GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Green, Info);
}

void UProjectCheatManager::TeleportTo(float X, float Y, float Z) {
  APlayerController* PC = GetOuterAPlayerController();
  if (!PC) {
    return;
  }

  APawn* Pawn = PC->GetPawn();
  if (!Pawn) {
    GEngine->AddOnScreenDebugMessage(-1, 3.0f, FColor::Red, TEXT("No pawn!"));
    return;
  }

  FVector NewLocation(X, Y, Z);
  Pawn->SetActorLocation(NewLocation);

  FString Msg = FString::Printf(TEXT("Teleported to (%.1f, %.1f, %.1f)"), X, Y, Z);
  UE_LOG(LogTemp, Log, TEXT("%s"), *Msg);
  GEngine->AddOnScreenDebugMessage(-1, 3.0f, FColor::Green, Msg);
}

void UProjectCheatManager::GodMode() {
  APlayerController* PC = GetOuterAPlayerController();
  if (!PC) {
    return;
  }

  APawn* Pawn = PC->GetPawn();
  if (!Pawn) {
    GEngine->AddOnScreenDebugMessage(-1, 3.0f, FColor::Red, TEXT("No pawn!"));
    return;
  }

  // Toggle can be damaged
  bool bCurrentlyInvulnerable = !Pawn->CanBeDamaged();
  Pawn->SetCanBeDamaged(!bCurrentlyInvulnerable);

  FString Msg = FString::Printf(TEXT("God Mode: %s"),
      Pawn->CanBeDamaged() ? TEXT("OFF") : TEXT("ON"));
  UE_LOG(LogTemp, Log, TEXT("%s"), *Msg);
  GEngine->AddOnScreenDebugMessage(-1, 3.0f,
      Pawn->CanBeDamaged() ? FColor::Yellow : FColor::Green, Msg);
}
