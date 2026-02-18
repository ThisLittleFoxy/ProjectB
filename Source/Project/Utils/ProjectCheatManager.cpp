// ProjectCheatManager.cpp

#include "ProjectCheatManager.h"
#include "Character/HealthComponent.h"
#include "Debug/DamageTestCube.h"
#include "Engine/World.h"
#include "GameFramework/DamageType.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/Pawn.h"
#include "Kismet/GameplayStatics.h"

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
  UHealthComponent* HealthComp = Pawn->FindComponentByClass<UHealthComponent>();

  FString HealthLine = TEXT("Health: n/a");
  if (HealthComp) {
    HealthLine = FString::Printf(TEXT("Health: %.1f / %.1f"),
                                 HealthComp->GetCurrentHealth(),
                                 HealthComp->GetMaxHealth());
  }

  FString Info = FString::Printf(
      TEXT("=== PLAYER INFO ===\nLocation: (%.1f, %.1f, %.1f)\nRotation: (%.1f, %.1f, %.1f)\n%s"),
      Location.X, Location.Y, Location.Z,
      Rotation.Pitch, Rotation.Yaw, Rotation.Roll,
      *HealthLine);

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

void UProjectCheatManager::SpawnDamageCube() {
  APawn* Pawn = GetControlledPawn();
  if (!Pawn) {
    GEngine->AddOnScreenDebugMessage(-1, 3.0f, FColor::Red, TEXT("No pawn!"));
    return;
  }

  UWorld* World = Pawn->GetWorld();
  if (!World) {
    return;
  }

  const FVector SpawnLocation =
      Pawn->GetActorLocation() + Pawn->GetActorForwardVector() * 500.0f +
      FVector(0.0f, 0.0f, 120.0f);
  const FRotator SpawnRotation = FRotator::ZeroRotator;

  FActorSpawnParameters SpawnParams;
  SpawnParams.Owner = nullptr;
  SpawnParams.Instigator = nullptr;
  SpawnParams.SpawnCollisionHandlingOverride =
      ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

  ADamageTestCube* SpawnedCube = World->SpawnActor<ADamageTestCube>(
      ADamageTestCube::StaticClass(), SpawnLocation, SpawnRotation, SpawnParams);

  if (!SpawnedCube) {
    SpawnParams.SpawnCollisionHandlingOverride =
        ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
    SpawnedCube = World->SpawnActor<ADamageTestCube>(
        ADamageTestCube::StaticClass(), SpawnLocation, SpawnRotation, SpawnParams);
  }

  if (!SpawnedCube) {
    UE_LOG(LogTemp, Warning, TEXT("SpawnDamageCube: spawn failed."));
    GEngine->AddOnScreenDebugMessage(-1, 3.0f, FColor::Red,
                                     TEXT("Spawn failed"));
    return;
  }

  const FString Msg = FString::Printf(TEXT("Spawned damage cube: %s"),
                                      *SpawnedCube->GetName());
  UE_LOG(LogTemp, Log, TEXT("%s"), *Msg);
  GEngine->AddOnScreenDebugMessage(-1, 3.0f, FColor::Green, Msg);
}

void UProjectCheatManager::SpawnPlayerCopy() { SpawnDamageCube(); }

void UProjectCheatManager::DamageCrosshair(float DamageAmount) {
  APlayerController* PC = GetOuterAPlayerController();
  APawn* Pawn = GetControlledPawn();
  if (!PC || !Pawn) {
    GEngine->AddOnScreenDebugMessage(-1, 3.0f, FColor::Red, TEXT("No pawn!"));
    return;
  }

  const float FinalDamage = FMath::Max(0.0f, DamageAmount);
  if (FinalDamage <= KINDA_SMALL_NUMBER) {
    GEngine->AddOnScreenDebugMessage(-1, 2.0f, FColor::Yellow,
                                     TEXT("Damage must be > 0"));
    return;
  }

  UWorld* World = Pawn->GetWorld();
  if (!World) {
    return;
  }

  FVector ViewLocation = FVector::ZeroVector;
  FRotator ViewRotation = FRotator::ZeroRotator;
  PC->GetPlayerViewPoint(ViewLocation, ViewRotation);

  const FVector TraceEnd = ViewLocation + ViewRotation.Vector() * 15000.0f;

  FCollisionQueryParams TraceParams(SCENE_QUERY_STAT(DamageCrosshairTrace), true);
  TraceParams.AddIgnoredActor(Pawn);

  FHitResult HitResult;
  const bool bHit = World->LineTraceSingleByChannel(
      HitResult, ViewLocation, TraceEnd, ECC_Visibility, TraceParams);

  if (!bHit || !HitResult.GetActor()) {
    GEngine->AddOnScreenDebugMessage(-1, 2.0f, FColor::Yellow,
                                     TEXT("No actor under crosshair"));
    return;
  }

  AActor* HitActor = HitResult.GetActor();
  UGameplayStatics::ApplyPointDamage(HitActor, FinalDamage, ViewRotation.Vector(),
                                     HitResult, PC, Pawn,
                                     UDamageType::StaticClass());

  const FString Msg = FString::Printf(TEXT("Hit %s for %.1f damage"),
                                      *HitActor->GetName(), FinalDamage);
  UE_LOG(LogTemp, Log, TEXT("%s"), *Msg);
  GEngine->AddOnScreenDebugMessage(-1, 2.0f, FColor::Green, Msg);
}

void UProjectCheatManager::KillCrosshair() { DamageCrosshair(100000.0f); }

APawn* UProjectCheatManager::GetControlledPawn() const {
  const APlayerController* PC = GetOuterAPlayerController();
  return PC ? PC->GetPawn() : nullptr;
}
