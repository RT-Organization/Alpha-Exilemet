#include "BaseTransactionWidget.h"
#include "AlphaExilemetCharacter.h"

bool UBaseTransactionWidget::CanAfford(FUpgradeCost CostInfo, AAlphaExilemetCharacter* Player)
{
	// Always check if the player pointer is valid first to prevent crashes!
	if (!Player) return false;

	// 1. Check Currency
	if (Player->Currency < CostInfo.CurrencyCost)
	{
		return false; // Not enough money!
	}

	// 2. Check Materials (Waiting on your coworker's Inventory system!)
	/* TODO: Once your coworker finishes the Inventory, you will loop through 
	   CostInfo.RequiredMaterials and check if the player has enough. 
	   It will look something like this:

	   for (const auto& MaterialPair : CostInfo.RequiredMaterials)
	   {
			FName MaterialName = MaterialPair.Key;
			int32 AmountNeeded = MaterialPair.Value;

			if (!Player->InventoryComponent->HasEnoughMaterial(MaterialName, AmountNeeded))
			{
				return false; // Not enough of this specific material!
			}
	   }
	*/

	// If we pass all checks, the player can afford it!
	return true;
}

void UBaseTransactionWidget::DeductCost(FUpgradeCost CostInfo, AAlphaExilemetCharacter* Player)
{
	if (!Player) return;

	// Deduct the money
	Player->Currency -= CostInfo.CurrencyCost;

	// TODO: Call your coworker's function to deduct the materials from the inventory
	// Example: Player->InventoryComponent->RemoveMaterials(CostInfo.RequiredMaterials);
}