#include "ProjectGameViewportClient.h"

#include "GenericPlatform/GenericPlatformMisc.h"
#include "Project.h"

bool UProjectGameViewportClient::WindowCloseRequested() {
  const bool bAllowClose = Super::WindowCloseRequested();
  if (bAllowClose) {
    RequestProcessExit();
  }

  return bAllowClose;
}

void UProjectGameViewportClient::CloseRequested(FViewport *InViewport) {
  RequestProcessExit();
  Super::CloseRequested(InViewport);
}

bool UProjectGameViewportClient::ShouldRequestProcessExit() const {
#if WITH_EDITOR
  if (GIsEditor) {
    return false;
  }
#endif

  return true;
}

void UProjectGameViewportClient::RequestProcessExit() {
  if (bHasRequestedProcessExit || !ShouldRequestProcessExit()) {
    return;
  }

  bHasRequestedProcessExit = true;

  UE_LOG(LogProject, Log,
         TEXT("ProjectGameViewportClient: requesting process exit after window close"));
  FPlatformMisc::RequestExit(false, TEXT("UProjectGameViewportClient::RequestProcessExit"));
}
