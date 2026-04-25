#include "SellTerminalWidget.h"
#include "AlphaExilemet/AlphaExilemetCharacter.h"

void USellTerminalWidget::ToggleSaleItem(AToolBase* Tool, FName ResourceID, int32 Amount, int32 CreditValue, bool bIsSelected)
{
	FPendingSale NewSale;
	NewSale.Tool = Tool;
	NewSale.ResourceID = ResourceID;
	NewSale.Amount = Amount;
	NewSale.CreditValue = CreditValue;

	if (bIsSelected)
	{
		PendingSales.AddUnique(NewSale);
		SelectedCreditsTotal += CreditValue;
	}
	else
	{
		PendingSales.RemoveSingle(NewSale);
		SelectedCreditsTotal -= CreditValue;
	}
}

void USellTerminalWidget::ProcessPendingSales(AAlphaExilemetCharacter* Player)
{
	if (!Player) return;

	int32 TotalBaseValue = 0;

	// Loop through the saved cart, delete the items, tally the money
	for (const FPendingSale& Sale : PendingSales)
	{
		if (Sale.Tool)
		{
			Sale.Tool->RemoveResource(Sale.ResourceID, Sale.Amount);
			TotalBaseValue += Sale.CreditValue;
		}
	}

	// Pay the player and empty the cart
	Player->ProcessSale(TotalBaseValue);
	ClearPendingSales();
}

void USellTerminalWidget::ClearPendingSales()
{
	PendingSales.Empty();
	SelectedCreditsTotal = 0;
}