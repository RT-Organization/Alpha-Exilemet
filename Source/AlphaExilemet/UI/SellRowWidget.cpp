#include "SellRowWidget.h"
#include "SellTerminalWidget.h"
#include "AlphaExilemet/Tools/ToolBase.h"
#include "AlphaExilemet/Tools/PickaxeTool.h"
#include "AlphaExilemet/Tools/VacuumTool.h"
#include "AlphaExilemet/Tools/GasRodTool.h"
#include "AlphaExilemet/Data/AlphaExilemetTypes.h"

// ─────────────────────────────────────────────────────────────────────────────
// SETUP
// ─────────────────────────────────────────────────────────────────────────────

void USellRowWidget::InitSellRow(AToolBase* InTool,
                                  int32 InCurrentCapacity,
                                  int32 InMaxDisplaySlots,
                                  UDataTable* InResourceDataTable)
{
	ToolReference     = InTool;
	CurrentCapacity   = InCurrentCapacity;
	MaxDisplaySlots   = FMath::Max(InMaxDisplaySlots, InCurrentCapacity); // safety
	ResourceDataTable = InResourceDataTable;

	BP_OnRowDataReady();
}

// ─────────────────────────────────────────────────────────────────────────────
// PRIVATE HELPERS
// ─────────────────────────────────────────────────────────────────────────────

bool USellRowWidget::GetRawInventoryFromTool(TMap<FName, int32>& OutMap) const
{
	if (!ToolReference) return false;

	if (const APickaxeTool* Pickaxe = Cast<APickaxeTool>(ToolReference))
	{
		OutMap = Pickaxe->HarvestedOres;
		return true;
	}
	if (const AVacuumTool* Vacuum = Cast<AVacuumTool>(ToolReference))
	{
		OutMap = Vacuum->HarvestedSlime;
		return true;
	}
	if (const AGasRodTool* Rod = Cast<AGasRodTool>(ToolReference))
	{
		OutMap = Rod->HarvestedGas;
		return true;
	}
	return false;
}

void USellRowWidget::HydrateSlotFromTable(FSellSlotData& OutSlotData) const
{
	if (!ResourceDataTable || OutSlotData.ResourceID.IsNone()) return;

	const FResourceRow* Row = ResourceDataTable->FindRow<FResourceRow>(
		OutSlotData.ResourceID, TEXT("USellRowWidget::HydrateSlotFromTable"));
	if (!Row) return;

	OutSlotData.CreditValue    = Row->SellValue * OutSlotData.Amount;
	OutSlotData.Icon           = Row->Icon;
	OutSlotData.PrimaryColor   = Row->PrimaryColor;
	OutSlotData.SecondaryColor = Row->SecondaryColor;
}

// ─────────────────────────────────────────────────────────────────────────────
// SLOT DATA
// Always returns MaxDisplaySlots entries:
//   index 0..CurrentCapacity-1  → usable slots (occupied or empty)
//   index CurrentCapacity..MaxDisplaySlots-1 → locked slots
// ─────────────────────────────────────────────────────────────────────────────

TArray<FSellSlotData> USellRowWidget::BuildSlotDataArray() const
{
	TArray<FSellSlotData> Result;
	Result.Reserve(MaxDisplaySlots);

	TMap<FName, int32> RawMap;
	GetRawInventoryFromTool(RawMap);

	TArray<FName> Keys;
	RawMap.GetKeys(Keys);

	for (int32 i = 0; i < MaxDisplaySlots; i++)
	{
		FSellSlotData SlotData;

		if (i >= CurrentCapacity)
		{
			// Locked — upgrade required to use this slot
			SlotData.bIsLocked   = true;
			SlotData.bIsOccupied = false;
		}
		else if (i < Keys.Num())
		{
			// Usable and has a resource
			const FName& Key     = Keys[i];
			SlotData.ResourceID  = Key;
			SlotData.Amount      = RawMap[Key];
			SlotData.bIsOccupied = (SlotData.Amount > 0);
			SlotData.bIsLocked   = false;
			HydrateSlotFromTable(SlotData);
		}
		else
		{
			// Usable but empty
			SlotData.bIsOccupied = false;
			SlotData.bIsLocked   = false;
		}

		Result.Add(SlotData);
	}

	return Result;
}

// ─────────────────────────────────────────────────────────────────────────────
// ECONOMY
// ─────────────────────────────────────────────────────────────────────────────

bool USellRowWidget::HasAnyItems() const
{
	TMap<FName, int32> RawMap;
	if (!GetRawInventoryFromTool(RawMap)) return false;

	for (const auto& Pair : RawMap)
	{
		if (Pair.Value > 0) return true;
	}
	return false;
}

int32 USellRowWidget::GetRowTotalCreditValue() const
{
	int32 Total = 0;
	for (const FSellSlotData& SlotData : BuildSlotDataArray())
	{
		if (SlotData.bIsOccupied) Total += SlotData.CreditValue;
	}
	return Total;
}

// ─────────────────────────────────────────────────────────────────────────────
// SELECTION
// ─────────────────────────────────────────────────────────────────────────────

void USellRowWidget::SetAllSlotsSelected(bool bSelect, USellTerminalWidget* Terminal)
{
	if (!Terminal || !ToolReference) return;

	TMap<FName, int32> RawMap;
	if (!GetRawInventoryFromTool(RawMap)) return;

	for (const auto& Pair : RawMap)
	{
		if (Pair.Value <= 0) continue;

		const int32 CreditVal = [&]() -> int32
		{
			if (!ResourceDataTable) return 0;
			const FResourceRow* Row = ResourceDataTable->FindRow<FResourceRow>(
				Pair.Key, TEXT("USellRowWidget::SetAllSlotsSelected"));
			return Row ? (Row->SellValue * Pair.Value) : 0;
		}();

		Terminal->ToggleSaleItem(ToolReference, Pair.Key, Pair.Value, CreditVal, bSelect);
	}

	BP_SetAllSlotsVisualState(bSelect);
}

void USellRowWidget::RemoveAllFromPendingSales(USellTerminalWidget* Terminal)
{
	if (!Terminal || !ToolReference) return;

	TMap<FName, int32> RawMap;
	if (!GetRawInventoryFromTool(RawMap)) return;

	for (const auto& Pair : RawMap)
	{
		if (Pair.Value <= 0) continue;

		const int32 CreditVal = [&]() -> int32
		{
			if (!ResourceDataTable) return 0;
			const FResourceRow* Row = ResourceDataTable->FindRow<FResourceRow>(Pair.Key, "");
			return Row ? (Row->SellValue * Pair.Value) : 0;
		}();

		Terminal->ToggleSaleItem(ToolReference, Pair.Key, Pair.Value, CreditVal, false);
	}
}
