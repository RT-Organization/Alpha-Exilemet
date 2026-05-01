#include "AlphaStreamingSubsystem.h"
#include "Kismet/GameplayStatics.h"
#include "AlphaExilemetGameInstance.h"

void UAlphaStreamingSubsystem::StreamLevel(FName LevelToLoad, FName LevelToUnload)
{
	UWorld* World = GetWorld();
	if (!World) return;

	FLatentActionInfo LatentInfo;
	LatentInfo.CallbackTarget = this;
	LatentInfo.ExecutionFunction = FName("OnStreamLevelLoaded");
	LatentInfo.Linkage = 0;
	LatentInfo.UUID = 123; // Unique ID

	// Load the new level
	UGameplayStatics::LoadStreamLevel(World, LevelToLoad, true, true, LatentInfo);
	
	// Unload the old level
	FLatentActionInfo UnloadLatentInfo;
	UGameplayStatics::UnloadStreamLevel(World, LevelToUnload, UnloadLatentInfo, false);
}

void UAlphaStreamingSubsystem::HandleTutorialCompletion()
{
	// Get the Game Instance to update the save state
	UAlphaExilemetGameInstance* GI = Cast<UAlphaExilemetGameInstance>(GetGameInstance());
	if (GI && GI->LocalSaveRef)
	{
		GI->LocalSaveRef->CurrentLevelName = FName("Main");
		GI->SavePlayerData();
	}

	// Swap the levels
	StreamLevel(FName("Main"), FName("Tutorial"));
}

void UAlphaStreamingSubsystem::EnterPortal(FName PortalLevelName, FTransform PlayerEntryTransform)
{
	UAlphaExilemetGameInstance* GI = Cast<UAlphaExilemetGameInstance>(GetGameInstance());
	if (GI && GI->LocalSaveRef)
	{
		// Save where we were standing before entering the portal
		GI->LocalSaveRef->PrePortalTransform = PlayerEntryTransform;
		GI->LocalSaveRef->CurrentLevelName = PortalLevelName;
		GI->SavePlayerData();
	}

	StreamLevel(PortalLevelName, FName("Main"));
}

void UAlphaStreamingSubsystem::ExitPortal()
{
	UAlphaExilemetGameInstance* GI = Cast<UAlphaExilemetGameInstance>(GetGameInstance());
	FName CurrentPortal = FName("Main"); // Fallback

	if (GI && GI->LocalSaveRef)
	{
		CurrentPortal = GI->LocalSaveRef->CurrentLevelName;
		// Reset the save state back to main
		GI->LocalSaveRef->CurrentLevelName = FName("Main");
		GI->SavePlayerData();
	}

	StreamLevel(FName("Main"), CurrentPortal);
}

void UAlphaStreamingSubsystem::OnStreamLevelLoaded()
{
	// This fires when a streaming level finishes loading.
	// You can hook this up to fade the screen back from black later!
	UE_LOG(LogTemp, Warning, TEXT("Level Stream Complete!"));
}