#include "SellTerminalWidget.h"
#include "AlphaExilemet/AlphaExilemetCharacter.h"
#include "AlphaExilemet/Tools/ToolBase.h"
#include "AlphaExilemet/Tools/PickaxeTool.h"
#include "AlphaExilemet/Tools/VacuumTool.h"
#include "AlphaExilemet/Tools/GasRodTool.h"
#include "AlphaExilemet/Data/AlphaExilemetTypes.h"

// ─────────────────────────────────────────────────────────────────────────────
// PRIVATE HELPERS
// ─────────────────────────────────────────────────────────────────────────────

int32 USellTerminalWidget::LookupStackValue(FName ResourceID, int32 Amount) const
{
	if (!ResourceDataTable || ResourceID.IsNone() || Amount <= 0) return 0;

	const FResourceRow* Row = ResourceDataTable->FindRow<FResourceRow>(
		ResourceID, TEXT("SellTerminalWidget::LookupStackValue")
	);
	return Row ? (Row->SellValue * Amount) : 0;
}

void USellTerminalWidget::CollectSalesFromTool(AToolBase* Tool, TArray<FPendingSale>& OutSales) const
{
	if (!Tool) return;

	auto Collect = [&](const TMap<FName, int32>& Inventory)
	{
		for (const auto& Pair : Inventory)
		{
			if (Pair.Value <= 0) continue;

			FPendingSale Sale;
			Sale.Tool        = Tool;
			Sale.ResourceID  = Pair.Key;
			Sale.Amount      = Pair.Value;
			Sale.CreditValue = LookupStackValue(Pair.Key, Pair.Value);
			OutSales.Add(Sale);
		}
	};

	if (APickaxeTool* Pickaxe = Cast<APickaxeTool>(Tool))
		Collect(Pickaxe->HarvestedOres);
	else if (AVacuumTool* Vacuum = Cast<AVacuumTool>(Tool))
		Collect(Vacuum->HarvestedSlime);
	else if (AGasRodTool* Rod = Cast<AGasRodTool>(Tool))
		Collect(Rod->HarvestedGas);
}

// ─────────────────────────────────────────────────────────────────────────────
// CART MANAGEMENT
// ─────────────────────────────────────────────────────────────────────────────

void USellTerminalWidget::ToggleSaleItem(AToolBase* Tool, FName ResourceID,
                                         int32 Amount, int32 CreditValue, bool bIsSelected)
{
	FPendingSale NewSale;
	NewSale.Tool        = Tool;
	NewSale.ResourceID  = ResourceID;
	NewSale.Amount      = Amount;
	NewSale.CreditValue = CreditValue;

	if (bIsSelected)
	{
		const int32 PrevNum = PendingSales.Num();
		PendingSales.AddUnique(NewSale);
		if (PendingSales.Num() > PrevNum)
		{
			SelectedCreditsTotal += CreditValue;
		}
	}
	else
	{
		const int32 PrevNum = PendingSales.Num();
		PendingSales.RemoveSingle(NewSale);
		if (PendingSales.Num() < PrevNum)
		{
			SelectedCreditsTotal -= CreditValue;
		}
	}
}

void USellTerminalWidget::ClearPendingSales()
{
	PendingSales.Empty();
	SelectedCreditsTotal = 0;
}

// ─────────────────────────────────────────────────────────────────────────────
// SELL
// ─────────────────────────────────────────────────────────────────────────────

void USellTerminalWidget::ProcessPendingSales(AAlphaExilemetCharacter* Player)
{
	if (!Player) return;

	int32 TotalBaseValue = 0;
	for (const FPendingSale& Sale : PendingSales)
	{
		if (Sale.Tool)
		{
			Sale.Tool->RemoveResource(Sale.ResourceID, Sale.Amount);
			TotalBaseValue += Sale.CreditValue;
		}
	}

	Player->ProcessSale(TotalBaseValue);
	ClearPendingSales();

	BP_OnCreditsChanged(Player->Currency);
	BP_OnSellCompleted();
}

void USellTerminalWidget::SellAll(AAlphaExilemetCharacter* Player)
{
	if (!Player) return;

	TArray<FPendingSale> AllSales;
	for (AToolBase* Tool : Player->OwnedTools)
	{
		CollectSalesFromTool(Tool, AllSales);
	}

	// FIX: IsEmpty() is not available in all UE TArray versions — use Num() == 0
	if (AllSales.Num() == 0) return;

	int32 TotalBase = 0;
	for (const FPendingSale& Sale : AllSales)
	{
		if (Sale.Tool)
		{
			Sale.Tool->RemoveResource(Sale.ResourceID, Sale.Amount);
			TotalBase += Sale.CreditValue;
		}
	}

	Player->ProcessSale(TotalBase);
	ClearPendingSales();

	BP_OnCreditsChanged(Player->Currency);
	BP_OnSellCompleted();
}

// ─────────────────────────────────────────────────────────────────────────────
// ECONOMY HELPERS
// ─────────────────────────────────────────────────────────────────────────────

float USellTerminalWidget::GetPlayerCreditAmount(AAlphaExilemetCharacter* Player) const
{
	return Player ? Player->Currency : 0.0f;
}

int32 USellTerminalWidget::CalculateTotalValueAllTools(AAlphaExilemetCharacter* Player) const
{
	if (!Player) return 0;

	int32 Total = 0;
	TArray<FPendingSale> Temp;
	for (AToolBase* Tool : Player->OwnedTools)
	{
		CollectSalesFromTool(Tool, Temp);
	}
	for (const FPendingSale& Sale : Temp)
	{
		Total += Sale.CreditValue;
	}
	return Total;
}
