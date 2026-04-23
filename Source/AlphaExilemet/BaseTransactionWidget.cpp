#include "BaseTransactionWidget.h"
#include "AlphaExilemetCharacter.h"

bool UBaseTransactionWidget::CanAfford(FUpgradeCost CostInfo, AAlphaExilemetCharacter* Player)
{
	if (!Player) return false;

	// 1. Check currency first (fast bail-out)
	if (Player->Currency < CostInfo.CurrencyCost)
	{
		return false;
	}

	// 2. Check every required material across all owned tools.
	//    GetTotalResourceAmount loops through OwnedTools and sums GetResourceAmount,
	//    which is overridden by Pickaxe (HarvestedOres), Vacuum (HarvestedSlime),
	//    and GasRod (HarvestedGas) — so the right tool is always checked automatically.
	for (const auto& MaterialPair : CostInfo.RequiredMaterials)
	{
		FName MaterialName   = MaterialPair.Key;
		int32 AmountNeeded   = MaterialPair.Value;

		int32 TotalHeld = Player->GetTotalResourceAmount(MaterialName);
		if (TotalHeld < AmountNeeded)
		{
			return false;
		}
	}

	return true;
}

void UBaseTransactionWidget::DeductCost(FUpgradeCost CostInfo, AAlphaExilemetCharacter* Player)
{
	if (!Player) return;

	// Deduct currency
	Player->Currency -= CostInfo.CurrencyCost;

	// Deduct materials — DeductResourceFromTools spreads the removal across tools
	// in OwnedTools order (e.g. takes ore from Pickaxe, slime from Vacuum, etc.)
	for (const auto& MaterialPair : CostInfo.RequiredMaterials)
	{
		Player->DeductResourceFromTools(MaterialPair.Key, MaterialPair.Value);
	}
}
