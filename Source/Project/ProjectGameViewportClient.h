#pragma once

#include "CoreMinimal.h"
#include "Engine/GameViewportClient.h"
#include "ProjectGameViewportClient.generated.h"

UCLASS()
class PROJECT_API UProjectGameViewportClient : public UGameViewportClient {
  GENERATED_BODY()

public:
  virtual bool WindowCloseRequested() override;
  virtual void CloseRequested(FViewport *InViewport) override;

private:
  bool ShouldRequestProcessExit() const;
  void RequestProcessExit();

  bool bHasRequestedProcessExit = false;
};
