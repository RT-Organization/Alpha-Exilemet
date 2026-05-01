#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "AlphaStreamingSubsystem.generated.h"

UCLASS()
class ALPHAEXILEMET_API UAlphaStreamingSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	/** Streams in a new level and unloads the current one */
	UFUNCTION(BlueprintCallable, Category = "AlphaExilemet|Streaming")
	void StreamLevel(FName LevelToLoad, FName LevelToUnload);

	/** Handles the specific cinematic transition from Tutorial to Main */
	UFUNCTION(BlueprintCallable, Category = "AlphaExilemet|Streaming")
	void HandleTutorialCompletion();

	/** Saves player location, unloads Main, streams Portal */
	UFUNCTION(BlueprintCallable, Category = "AlphaExilemet|Streaming")
	void EnterPortal(FName PortalLevelName, FTransform PlayerEntryTransform);

	/** Unloads Portal, streams Main, restores player location */
	UFUNCTION(BlueprintCallable, Category = "AlphaExilemet|Streaming")
	void ExitPortal();

private:
	UFUNCTION()
	void OnStreamLevelLoaded();
};