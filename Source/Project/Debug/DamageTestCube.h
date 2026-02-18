// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "DamageTestCube.generated.h"

class UHealthComponent;
class UStaticMeshComponent;

/**
 * Simple debug target for damage tests.
 * Uses HealthComponent and disappears on death.
 */
UCLASS()
class PROJECT_API ADamageTestCube : public AActor {
  GENERATED_BODY()

public:
  ADamageTestCube();

  UFUNCTION(BlueprintPure, Category = "Debug")
  UHealthComponent *GetHealthComponent() const { return HealthComponent; }

protected:
  UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Debug")
  TObjectPtr<UStaticMeshComponent> CubeMesh;

  UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Debug")
  TObjectPtr<UHealthComponent> HealthComponent;
};
