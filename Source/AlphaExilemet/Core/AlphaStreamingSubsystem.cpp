#include "AlphaStreamingSubsystem.h"
#include "Kismet/GameplayStatics.h"

ULevelStreamingManager* UAlphaStreamingSubsystem::GetManager() const
{
	UGameInstance* GI = GetGameInstance();
	return GI ? GI->GetSubsystem<ULevelStreamingManager>() : nullptr;
}

void UAlphaStreamingSubsystem::ReturnToMainMenu()
{
	if (ULevelStreamingManager* M = GetManager()) M->ReturnToMainMenu();
}

void UAlphaStreamingSubsystem::HandleTutorialCompletion()
{
	if (ULevelStreamingManager* M = GetManager()) M->CompleteTutorialAndLoadMain();
}

void UAlphaStreamingSubsystem::CompletePortalChallenge()
{
	if (ULevelStreamingManager* M = GetManager()) M->CompletePortalChallenge();
}

void UAlphaStreamingSubsystem::Debug_ForceCompleteChallenge()
{
	if (ULevelStreamingManager* M = GetManager()) M->Debug_ForceExitPortal();
}
