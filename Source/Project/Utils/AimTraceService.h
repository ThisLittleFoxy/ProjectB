#pragma once

#include "CoreMinimal.h"

/**
 * ������ ��� ����������� ����� �� ������ ������
 * ������������ ��� �������������� � ���������, ������������� � �.�.
 */
class PROJECT_API FAimTraceService{
  public :
      // ��������� ��� �� ������ ������
      static bool MakeScreenCenterRay(const APlayerController *PC,
                                      float Distance, FVector &OutStart,
                                      FVector &OutEnd){if (!PC) return false;

int32 SX = 0, SY = 0;
PC->GetViewportSize(SX, SY);
const FVector2D Center(SX * 0.5f, SY * 0.5f);

FVector Origin, Dir;
if (PC->DeprojectScreenPositionToWorld(Center.X, Center.Y, Origin, Dir)) {
  OutStart = Origin;
  OutEnd = Origin + Dir * Distance;
  return true;
}

return false;
}

// ��������� ��� �� ����� ������ ����������� (��� ��������� ���������)
static bool MakeViewPointRay(const AController *Controller, float Distance,
                             FVector &OutStart, FVector &OutEnd) {
  if (!Controller)
    return false;

  FVector Location;
  FRotator Rotation;
  Controller->GetPlayerViewPoint(Location, Rotation);

  OutStart = Location;
  OutEnd = Location + Rotation.Vector() * Distance;
  return true;
}

// ��������������� �������� ������ �� ������ ������
static bool TraceFromScreenCenter(UWorld *World, const APlayerController *PC,
                                  float Distance, ECollisionChannel Channel,
                                  const TArray<AActor *> &ActorsToIgnore,
                                  FHitResult &OutHit,
                                  bool bTraceComplex = false) {
  if (!World || !PC)
    return false;

  FVector Start, End;
  if (!MakeScreenCenterRay(PC, Distance, Start, End))
    return false;

  FCollisionQueryParams Params(SCENE_QUERY_STAT(ScreenCenterTrace),
                               bTraceComplex);
  for (const AActor *A : ActorsToIgnore) {
    if (IsValid(A))
      Params.AddIgnoredActor(A);
  }

  return World->LineTraceSingleByChannel(OutHit, Start, End, Channel, Params);
}

// ������ �� ViewPoint (������������)
static bool TraceFromViewPoint(UWorld *World, const AController *Controller,
                               float Distance, ECollisionChannel Channel,
                               const TArray<AActor *> &ActorsToIgnore,
                               FHitResult &OutHit, bool bTraceComplex = false) {
  if (!World || !Controller)
    return false;

  FVector Start, End;
  if (!MakeViewPointRay(Controller, Distance, Start, End))
    return false;

  FCollisionQueryParams Params(SCENE_QUERY_STAT(ViewPointTrace), bTraceComplex);
  for (const AActor *A : ActorsToIgnore) {
    if (IsValid(A))
      Params.AddIgnoredActor(A);
  }

  return World->LineTraceSingleByChannel(OutHit, Start, End, Channel, Params);
  }
};