#include "AlphaExilemetSaveGame.h"

UAlphaExilemetSaveGame::UAlphaExilemetSaveGame()
{
	// Initialize defaults to prevent garbage data on a fresh save
	SavedHealth   = 100.0f;
	SavedOxygen   = 100.0f;
	SavedCurrency = 0.0f;
	
	CurrentLevelName = FName("Tutorial");

	// TASK A1: Default false so SetupPlayerData() never teleports a fresh save to world origin.
	// This is set to true only by SavePlayerData() after a real position has been recorded.
	bHasValidTransform = false;
}
