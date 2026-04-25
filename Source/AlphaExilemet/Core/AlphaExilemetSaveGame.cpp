#include "AlphaExilemetSaveGame.h"

UAlphaExilemetSaveGame::UAlphaExilemetSaveGame()
{
	// Initialize defaults to prevent garbage data on a fresh save
	SavedHealth = 100.0f;
	SavedOxygen = 100.0f;
	SavedCurrency = 0.0f;
}