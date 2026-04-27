#include "VacuumSellRowWidget.h"
#include "AlphaExilemet/Tools/VacuumTool.h"
#include "AlphaExilemet/Data/AlphaExilemetTypes.h"

// ─────────────────────────────────────────────────────────────────────────────
// PRIVATE HELPERS
// ─────────────────────────────────────────────────────────────────────────────

AVacuumTool* UVacuumSellRowWidget::GetVacuumTool() const
{
	return Cast<AVacuumTool>(ToolReference);
}

void UVacuumSellRowWidget::HydrateSegmentFromTable(FVacuumSegmentData& OutSeg) const
{
	if (!ResourceDataTable || OutSeg.ResourceID.IsNone()) return;

	const FResourceRow* Row = ResourceDataTable->FindRow<FResourceRow>(
		OutSeg.ResourceID, TEXT("UVacuumSellRowWidget::HydrateSegmentFromTable"));
	if (!Row) return;

	OutSeg.CreditValue    = Row->SellValue * OutSeg.Amount;
	OutSeg.PrimaryColor   = Row->PrimaryColor;
	OutSeg.SecondaryColor = Row->SecondaryColor;
}

// ─────────────────────────────────────────────────────────────────────────────
// SEGMENT DATA
// ─────────────────────────────────────────────────────────────────────────────

TArray<FVacuumSegmentData> UVacuumSellRowWidget::BuildSegmentDataArray() const
{
	TArray<FVacuumSegmentData> Result;

	AVacuumTool* Vacuum = GetVacuumTool();
	if (!Vacuum) return Result;

	const float TankMax = FMath::Max(1.0f, static_cast<float>(CurrentCapacity));
	float TotalFillRatio = 0.0f; // Track how much of the bar is taken

	for (const auto& Pair : Vacuum->HarvestedSlime)
	{
		if (Pair.Value <= 0) continue;

		FVacuumSegmentData Seg;
		Seg.ResourceID = Pair.Key;
		Seg.Amount     = Pair.Value;
		Seg.FillRatio  = FMath::Clamp(static_cast<float>(Pair.Value) / TankMax, 0.0f, 1.0f);
		Seg.bIsEmpty   = false;
		HydrateSegmentFromTable(Seg);

		TotalFillRatio += Seg.FillRatio;
		Result.Add(Seg);
	}

	// If the tank isn't 100% full, add a "ghost" segment to fill the empty space
	if (TotalFillRatio < 1.0f)
	{
		FVacuumSegmentData EmptySeg;
		EmptySeg.FillRatio = 1.0f - TotalFillRatio; // e.g., 1.0 - 0.15 = 0.85
		EmptySeg.bIsEmpty  = true;
		Result.Add(EmptySeg);
	}

	return Result;
}

// ─────────────────────────────────────────────────────────────────────────────
// TANK INFO
// ─────────────────────────────────────────────────────────────────────────────

int32 UVacuumSellRowWidget::GetTotalStoredSlime() const
{
	AVacuumTool* Vacuum = GetVacuumTool();
	if (!Vacuum) return 0;

	int32 Total = 0;
	for (const auto& Pair : Vacuum->HarvestedSlime)
	{
		Total += Pair.Value;
	}
	return Total;
}

float UVacuumSellRowWidget::GetTankFillRatio() const
{
	const float Max = FMath::Max(1.0f, static_cast<float>(CurrentCapacity));
	return FMath::Clamp(static_cast<float>(GetTotalStoredSlime()) / Max, 0.0f, 1.0f);
}

bool UVacuumSellRowWidget::VacuumHasAnySlime() const
{
	AVacuumTool* Vacuum = GetVacuumTool();
	if (!Vacuum) return false;

	for (const auto& Pair : Vacuum->HarvestedSlime)
	{
		if (Pair.Value > 0) return true;
	}
	return false;
}

void UVacuumSellRowWidget::InitVacuumSellRow(AVacuumTool* InVacuumTool, int32 InTankCapacity, UDataTable* InResourceDataTable)
{
	// Set the inherited variables from USellRowWidget
	ToolReference = InVacuumTool;
	CurrentCapacity = InTankCapacity; 
	ResourceDataTable = InResourceDataTable;

	// MaxDisplaySlots is irrelevant for the vacuum, so we can ignore it or safely set it to 0
	MaxDisplaySlots = 0; 

	// Fire the exact same BP event so the UI knows it's time to build the segments
	BP_OnRowDataReady();
}