#include "UpgradeProgressionManager.h"
#include "AlphaExilemet/AlphaExilemetCharacter.h"
#include "AlphaExilemet/Core/AlphaExilemetSaveGame.h"

// -------------------------------------------------------------------------
// INITIALIZATION
// -------------------------------------------------------------------------

void UUpgradeProgressionManager::InitializeFromDataTables(
	UDataTable* CharacterUpgradeTable,
	UDataTable* ToolUpgradeTable,
	UDataTable* ShipRepairTable,
	const TMap<EPlayerStat, int32>& CharacterLevels,
	const TMap<FName, int32>&       ToolStatLevels,
	const TMap<EShipSystem, int32>& ShipLevels)
{
	CachedCharTable = CharacterUpgradeTable;
	CachedToolTable = ToolUpgradeTable;
	CachedShipTable = ShipRepairTable;

	// --- CHARACTER UPGRADES ---
	// DT Row: FCharacterUpgradeRow → one row per stat (Health, Oxygen, Agility)
	// CostPerLevel[N] = cost to reach level N+1.
	// If player is already at level 2, seed CostPerLevel[2] (cost to reach level 3).
	if (CachedCharTable)
	{
		TArray<FName> RowNames = CachedCharTable->GetRowNames();
		for (FName RowName : RowNames)
		{
			// Don't overwrite entries that already have partial payments loaded
			if (RuntimeCosts.Contains(RowName)) continue;

			FCharacterUpgradeRow* Row = CachedCharTable->FindRow<FCharacterUpgradeRow>(RowName, TEXT("UUpgradeProgressionManager::Init"));
			if (!Row) continue;

			int32 CurrentLevel = CharacterLevels.FindRef(Row->StatID);
			if (Row->CostPerLevel.IsValidIndex(CurrentLevel))
			{
				RuntimeCosts.Add(RowName, Row->CostPerLevel[CurrentLevel]);
			}
			// If not valid index → already maxed, do not add to map
		}
	}

	// --- TOOL UPGRADES ---
	// DT Row: FToolUpgradeRow → one row per tool stat ("Pickaxe_Strength", etc.)
	// The row's StatID matches the key in ToolUpgradeLevels.
	if (CachedToolTable)
	{
		TArray<FName> RowNames = CachedToolTable->GetRowNames();
		for (FName RowName : RowNames)
		{
			if (RuntimeCosts.Contains(RowName)) continue;

			FToolUpgradeRow* Row = CachedToolTable->FindRow<FToolUpgradeRow>(RowName, TEXT("UUpgradeProgressionManager::Init"));
			if (!Row) continue;

			int32 CurrentLevel = ToolStatLevels.FindRef(Row->StatID);
			if (Row->CostPerLevel.IsValidIndex(CurrentLevel))
			{
				RuntimeCosts.Add(RowName, Row->CostPerLevel[CurrentLevel]);
			}
		}
	}

	// --- SHIP REPAIR UPGRADES ---
	// DT Row: FShipRepairRow → one row per ship system.
	if (CachedShipTable)
	{
		TArray<FName> RowNames = CachedShipTable->GetRowNames();
		for (FName RowName : RowNames)
		{
			if (RuntimeCosts.Contains(RowName)) continue;

			FShipRepairRow* Row = CachedShipTable->FindRow<FShipRepairRow>(RowName, TEXT("UUpgradeProgressionManager::Init"));
			if (!Row) continue;

			int32 CurrentLevel = ShipLevels.FindRef(Row->SystemID);
			if (Row->CostPerLevel.IsValidIndex(CurrentLevel))
			{
				RuntimeCosts.Add(RowName, Row->CostPerLevel[CurrentLevel]);
			}
		}
	}
}

// -------------------------------------------------------------------------
// COST QUERIES
// -------------------------------------------------------------------------

FUpgradeCost UUpgradeProgressionManager::GetCurrentCost(FName UpgradeKey) const
{
	if (const FUpgradeCost* Cost = RuntimeCosts.Find(UpgradeKey))
	{
		return *Cost;
	}
	return FUpgradeCost(); // Empty = maxed or not found
}

bool UUpgradeProgressionManager::CanAffordFull(FName UpgradeKey, AAlphaExilemetCharacter* Player) const
{
	if (!Player) return false;

	const FUpgradeCost* Cost = RuntimeCosts.Find(UpgradeKey);
	if (!Cost) return false;

	// Check currency (Player->Currency is float; cost is int32)
	if (FMath::FloorToInt(Player->Currency) < Cost->CurrencyCost) return false;

	// Check every material across all tools
	for (const auto& Pair : Cost->RequiredMaterials)
	{
		if (Player->GetTotalResourceAmount(Pair.Key) < Pair.Value) return false;
	}

	return true;
}

bool UUpgradeProgressionManager::HasUpgradeAvailable(FName UpgradeKey) const
{
	return RuntimeCosts.Contains(UpgradeKey);
}

// -------------------------------------------------------------------------
// TRANSACTION
// -------------------------------------------------------------------------

bool UUpgradeProgressionManager::PayTowardsUpgrade(FName UpgradeKey, AAlphaExilemetCharacter* Player)
{
	if (!Player) return false;

	FUpgradeCost* Cost = RuntimeCosts.Find(UpgradeKey);
	if (!Cost) return false;

	// --- 1. Deduct currency ---
	// Player->Currency is float; treat it as floor-to-int for fairness.
	int32 CurrencyAvailable = FMath::FloorToInt(Player->Currency);
	int32 CurrencyToPay     = FMath::Min(CurrencyAvailable, Cost->CurrencyCost);
	Player->Currency        -= static_cast<float>(CurrencyToPay);
	Cost->CurrencyCost      -= CurrencyToPay;

	// --- 2. Deduct materials (partial deduction is intentional) ---
	// Works for ALL resource types:
	//   - Solid  → PickaxeTool::HarvestedOres  (GetTotalResourceAmount / DeductResourceFromTools)
	//   - Slime  → VacuumTool::HarvestedSlime  (stored as integers via AbsorbSlime's FloorToInt)
	//   - Gas    → GasRodTool::HarvestedGas    (stored as sphere count via TryAddGas)
	// The character already has GetTotalResourceAmount / DeductResourceFromTools which
	// loop across all owned tools — so no special-casing per resource type is needed here.
	for (auto& Pair : Cost->RequiredMaterials)
	{
		int32 Deducted = DeductPartial(Player, Pair.Key, Pair.Value);
		Pair.Value    -= Deducted;
	}

	// Remove materials that are now fully paid (keep the map clean for display)
	for (auto It = Cost->RequiredMaterials.CreateIterator(); It; ++It)
	{
		if (It->Value <= 0) It.RemoveCurrent();
	}

	// --- 3. Check completion ---
	bool bFullyPaid = (Cost->CurrencyCost <= 0 && Cost->RequiredMaterials.IsEmpty());

	// NOTE: We do NOT remove the entry here on full payment.
	// The caller must call AdvanceToNextLevelCost() which will replace / remove it.
	// Leaving it at zero cost is intentional — the widget reads it to show "PAID".

	return bFullyPaid;
}

void UUpgradeProgressionManager::AdvanceToNextLevelCost(FName UpgradeKey, int32 LevelJustReached)
{
	// LevelJustReached = the level the player just moved TO.
	// The cost for the NEXT upgrade = CostPerLevel[LevelJustReached]
	// (array is 0-indexed: [0]=cost to reach L1, [1]=cost to reach L2, …)
	FUpgradeCost NextCost;
	if (TryGetCostForLevel(UpgradeKey, LevelJustReached, NextCost))
	{
		RuntimeCosts.Add(UpgradeKey, NextCost);
	}
	else
	{
		// No more levels — upgrade is maxed, remove from map entirely.
		RuntimeCosts.Remove(UpgradeKey);
	}
}

// -------------------------------------------------------------------------
// SAVE / LOAD
// -------------------------------------------------------------------------

void UUpgradeProgressionManager::SaveToSaveObject(UAlphaExilemetSaveGame* SaveObject) const
{
	if (!SaveObject) return;
	SaveObject->SavedRemainingCosts = RuntimeCosts;
}

void UUpgradeProgressionManager::LoadFromSaveObject(const UAlphaExilemetSaveGame* SaveObject)
{
	if (!SaveObject) return;

	// Overlay saved partial payments on top of the already-initialized defaults.
	// Only keys that exist in the save file are overwritten; fresh upgrades
	// (not yet touched by the player) keep their DataTable-seeded defaults.
	for (const auto& Pair : SaveObject->SavedRemainingCosts)
	{
		RuntimeCosts.Add(Pair.Key, Pair.Value);
	}
}

// -------------------------------------------------------------------------
// PRIVATE HELPERS
// -------------------------------------------------------------------------

bool UUpgradeProgressionManager::TryGetCostForLevel(FName UpgradeKey, int32 Level, FUpgradeCost& OutCost) const
{
	// Try character upgrades
	if (CachedCharTable)
	{
		if (FCharacterUpgradeRow* Row = CachedCharTable->FindRow<FCharacterUpgradeRow>(UpgradeKey, TEXT("")))
		{
			if (Row->CostPerLevel.IsValidIndex(Level))
			{
				OutCost = Row->CostPerLevel[Level];
				return true;
			}
			return false; // Key found but level is out of bounds → maxed
		}
	}

	// Try tool upgrades
	if (CachedToolTable)
	{
		if (FToolUpgradeRow* Row = CachedToolTable->FindRow<FToolUpgradeRow>(UpgradeKey, TEXT("")))
		{
			if (Row->CostPerLevel.IsValidIndex(Level))
			{
				OutCost = Row->CostPerLevel[Level];
				return true;
			}
			return false;
		}
	}

	// Try ship repairs
	if (CachedShipTable)
	{
		if (FShipRepairRow* Row = CachedShipTable->FindRow<FShipRepairRow>(UpgradeKey, TEXT("")))
		{
			if (Row->CostPerLevel.IsValidIndex(Level))
			{
				OutCost = Row->CostPerLevel[Level];
				return true;
			}
			return false;
		}
	}

	return false; // Key not found in any table
}

int32 UUpgradeProgressionManager::DeductPartial(AAlphaExilemetCharacter* Player, FName ResourceID, int32 Needed)
{
	if (!Player || Needed <= 0) return 0;

	int32 Available = Player->GetTotalResourceAmount(ResourceID);
	int32 ToDeduct  = FMath::Min(Available, Needed);

	if (ToDeduct > 0)
	{
		Player->DeductResourceFromTools(ResourceID, ToDeduct);
	}

	return ToDeduct;
}
