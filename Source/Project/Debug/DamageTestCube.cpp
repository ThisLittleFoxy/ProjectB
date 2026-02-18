// Copyright Epic Games, Inc. All Rights Reserved.

#include "Debug/DamageTestCube.h"
#include "Character/HealthComponent.h"
#include "Components/StaticMeshComponent.h"
#include "UObject/ConstructorHelpers.h"

ADamageTestCube::ADamageTestCube() {
  PrimaryActorTick.bCanEverTick = false;
  SetCanBeDamaged(true);

  CubeMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("CubeMesh"));
  SetRootComponent(CubeMesh);

  CubeMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
  CubeMesh->SetCollisionResponseToAllChannels(ECR_Block);
  CubeMesh->SetMobility(EComponentMobility::Movable);
  CubeMesh->SetGenerateOverlapEvents(false);
  CubeMesh->SetSimulatePhysics(false);

  static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMeshAsset(
      TEXT("/Engine/BasicShapes/Cube.Cube"));
  if (CubeMeshAsset.Succeeded()) {
    CubeMesh->SetStaticMesh(CubeMeshAsset.Object);
  }

  HealthComponent = CreateDefaultSubobject<UHealthComponent>(TEXT("HealthComponent"));
}
