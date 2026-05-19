#include "AlphaExilemetSaveGame.h"

UAlphaExilemetSaveGame::UAlphaExilemetSaveGame()
{
	SavedHealth   = 100.0f;
	SavedOxygen   = 100.0f;
	SavedCurrency = 0.0f;
	
	CurrentLevelName = FName("Tutorial");
	
	bHasValidTransform = false;
}
