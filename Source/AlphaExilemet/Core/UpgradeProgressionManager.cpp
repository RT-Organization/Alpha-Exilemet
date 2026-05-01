#include "UpgradeProgressionManager.h"
#include "AlphaExilemet/AlphaExilemetCharacter.h"
#include "AlphaExilemet/Core/AlphaExilemetSaveGame.h"

// =========================================================================
// INITIALIZATION
// =========================================================================

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

	// CHARACTER UPGRADES
	if (CachedCharTable)
	{
		for (FName RowName : CachedCharTable->GetRowNames())
		{
			if (RuntimeCosts.Contains(RowName)) continue;

			FCharacterUpgradeRow* Row = CachedCharTable->FindRow<FCharacterUpgradeRow>(
				RowName, TEXT("ProgressionManager::Init"));
			if (!Row) continue;

			int32 CurrentLevel = CharacterLevels.FindRef(Row->StatID);
			if (Row->CostPerLevel.IsValidIndex(CurrentLevel))
				RuntimeCosts.Add(RowName, Row->CostPerLevel[CurrentLevel]);
		}
	}

	// TOOL UPGRADES
	if (CachedToolTable)
	{
		for (FName RowName : CachedToolTable->GetRowNames())
		{
			if (RuntimeCosts.Contains(RowName)) continue;

			FToolUpgradeRow* Row = CachedToolTable->FindRow<FToolUpgradeRow>(
				RowName, TEXT("ProgressionManager::Init"));
			if (!Row) continue;

			int32 CurrentLevel = ToolStatLevels.FindRef(Row->StatID);
			if (Row->CostPerLevel.IsValidIndex(CurrentLevel))
				RuntimeCosts.Add(RowName, Row->CostPerLevel[CurrentLevel]);
		}
	}

	// SHIP UPGRADES
	if (CachedShipTable)
	{
		for (FName RowName : CachedShipTable->GetRowNames())
		{
			if (RuntimeCosts.Contains(RowName)) continue;

			FShipRepairRow* Row = CachedShipTable->FindRow<FShipRepairRow>(
				RowName, TEXT("ProgressionManager::Init"));
			if (!Row) continue;

			int32 CurrentLevel = ShipLevels.FindRef(Row->SystemID);
			if (Row->CostPerLevel.IsValidIndex(CurrentLevel))
				RuntimeCosts.Add(RowName, Row->CostPerLevel[CurrentLevel]);
		}
	}
}

// =========================================================================
// COST QUERIES
// =========================================================================

FUpgradeCost UUpgradeProgressionManager::GetCurrentCost(FName UpgradeKey) const
{
	if (const FUpgradeCost* Cost = RuntimeCosts.Find(UpgradeKey))
		return *Cost;
	return FUpgradeCost();
}

bool UUpgradeProgressionManager::CanPayAnything(FName UpgradeKey, AAlphaExilemetCharacter* Player) const
{
	if (!Player) return false;

	const FUpgradeCost* Cost = RuntimeCosts.Find(UpgradeKey);
	if (!Cost) return false; // Upgrade doesn't exist / Maxed out

	// 1. FREE UPGRADE CHECK: Is the cost completely 0?
	// If 0 currency and no materials required, the player can "afford" it!
	if (Cost->CurrencyCost <= 0 && Cost->RequiredMaterials.Num() == 0)
	{
		return true;
	}

	// 2. Can we pay the currency in full?
	if (Cost->CurrencyCost > 0 && Player->Currency >= static_cast<float>(Cost->CurrencyCost)) 
	{
		return true;
	}

	// 3. Do we have at least 1 of ANY required material?
	for (const auto& Pair : Cost->RequiredMaterials)
	{
		if (Pair.Value > 0 && Player->GetTotalResourceAmount(Pair.Key) > 0)
		{
			return true;
		}
	}

	// We aren't free, we don't have enough currency, and we have 0 materials to give.
	return false;
}

bool UUpgradeProgressionManager::HasUpgradeAvailable(FName UpgradeKey) const
{
	return RuntimeCosts.Contains(UpgradeKey);
}

// =========================================================================
// TRANSACTION — ALL OR NOTHING
// =========================================================================

bool UUpgradeProgressionManager::PayTowardsUpgrade(FName UpgradeKey, AAlphaExilemetCharacter* Player)
{
	if (!Player) return false;

	FUpgradeCost* Cost = RuntimeCosts.Find(UpgradeKey);
	if (!Cost) return false;

	// ── DEDUCT MATERIALS (Partial Payment) ──────────────────────────────────
	for (auto It = Cost->RequiredMaterials.CreateIterator(); It; ++It)
	{
		int32 RequiredAmount = It->Value;
		if (RequiredAmount > 0)
		{
			int32 PlayerHas = Player->GetTotalResourceAmount(It->Key);
			if (PlayerHas > 0)
			{
				// Take as much as the player has, up to the required amount
				int32 AmountToTake = FMath::Min(PlayerHas, RequiredAmount);
				
				Player->DeductResourceFromTools(It->Key, AmountToTake);
				It->Value -= AmountToTake; // Lower the remaining cost
			}
		}
	}

	// ── DEDUCT CURRENCY (All or Nothing) ────────────────────────────────────
	if (Cost->CurrencyCost > 0 && Player->Currency >= static_cast<float>(Cost->CurrencyCost))
	{
		Player->Currency -= static_cast<float>(Cost->CurrencyCost);
		Cost->CurrencyCost = 0;
		
		// Tell the entire game UI that the player's wallet just changed!
		Player->OnCurrencyUpdated.Broadcast(Player->Currency);
	}

	// ── CLEANUP ─────────────────────────────────────────────────────────────
	// Remove materials from the map if their cost has reached 0
	for (auto It = Cost->RequiredMaterials.CreateIterator(); It; ++It)
	{
		if (It->Value <= 0) It.RemoveCurrent();
	}

	// ── CHECK IF FULLY PAID ─────────────────────────────────────────────────
	// If currency is 0 and the materials map is empty, the upgrade is complete!
	if (Cost->CurrencyCost <= 0 && Cost->RequiredMaterials.Num() == 0)
	{
		return true; 
	}

	// We made a partial payment, but it's not fully paid yet. 
	// Returning false tells the Blueprint NOT to level up yet, just update the UI.
	return false;
}

void UUpgradeProgressionManager::AdvanceToNextLevelCost(FName UpgradeKey, int32 LevelJustReached)
{
	FUpgradeCost NextCost;
	if (TryGetCostForLevel(UpgradeKey, LevelJustReached, NextCost))
	{
		RuntimeCosts.Add(UpgradeKey, NextCost);
	}
	else
	{
		// Maxed out — remove entirely
		RuntimeCosts.Remove(UpgradeKey);
	}
}

// =========================================================================
// SAVE / LOAD
// =========================================================================

void UUpgradeProgressionManager::SaveToSaveObject(UAlphaExilemetSaveGame* SaveObject) const
{
	if (!SaveObject) return;
	// We only save RuntimeCosts that differ from the DataTable defaults.
	// Since this is an all-or-nothing system, RuntimeCosts only contains
	// the current level's full cost (never partial amounts).
	// We still save it so the correct level cost is restored on load.
	SaveObject->SavedRemainingCosts = RuntimeCosts;
}

void UUpgradeProgressionManager::LoadFromSaveObject(const UAlphaExilemetSaveGame* SaveObject)
{
	if (!SaveObject) return;
	// Overlay saved costs on top of initialized defaults.
	// This ensures the correct level cost is shown after loading.
	for (const auto& Pair : SaveObject->SavedRemainingCosts)
	{
		RuntimeCosts.Add(Pair.Key, Pair.Value);
	}
}

// =========================================================================
// PRIVATE
// =========================================================================

bool UUpgradeProgressionManager::TryGetCostForLevel(FName UpgradeKey, int32 Level, FUpgradeCost& OutCost) const
{
	if (CachedCharTable)
	{
		if (FCharacterUpgradeRow* Row = CachedCharTable->FindRow<FCharacterUpgradeRow>(UpgradeKey, TEXT("")))
		{
			if (Row->CostPerLevel.IsValidIndex(Level)) { OutCost = Row->CostPerLevel[Level]; return true; }
			return false;
		}
	}
	if (CachedToolTable)
	{
		if (FToolUpgradeRow* Row = CachedToolTable->FindRow<FToolUpgradeRow>(UpgradeKey, TEXT("")))
		{
			if (Row->CostPerLevel.IsValidIndex(Level)) { OutCost = Row->CostPerLevel[Level]; return true; }
			return false;
		}
	}
	if (CachedShipTable)
	{
		if (FShipRepairRow* Row = CachedShipTable->FindRow<FShipRepairRow>(UpgradeKey, TEXT("")))
		{
			if (Row->CostPerLevel.IsValidIndex(Level)) { OutCost = Row->CostPerLevel[Level]; return true; }
			return false;
		}
	}
	return false;
}