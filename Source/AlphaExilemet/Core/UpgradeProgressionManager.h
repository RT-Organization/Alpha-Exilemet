#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "AlphaExilemet/Data/AlphaExilemetTypes.h"
#include "UpgradeProgressionManager.generated.h"

class AAlphaExilemetCharacter;
class UAlphaExilemetSaveGame;
class UDataTable;

/**
 * UUpgradeProgressionManager
 *
 * Owned by the GameInstance. Single source of truth for live upgrade costs.
 *
 * PAYMENT MODEL:
 *   The player must have the FULL remaining cost (currency + ALL materials)
 *   before any deduction occurs. If they cannot afford it, nothing is taken.
 *   PayTowardsUpgrade returns FALSE and touches nothing.
 *
 *   Partial payment is still supported via the MATERIAL-ONLY partial system:
 *   if the player has all currency but only some materials, currency is held
 *   and only the materials they have are taken. The cost updates accordingly.
 *   Currency is ONLY deducted when it can be taken in full.
 *
 *   Actually — per designer spec: player needs ALL currency AND ALL materials
 *   at once. If they don't have everything, nothing is deducted.
 *   Use CanAffordFull() to gate the button. PayTowardsUpgrade() is all-or-nothing.
 */
UCLASS(BlueprintType)
class ALPHAEXILEMET_API UUpgradeProgressionManager : public UObject
{
	GENERATED_BODY()

public:
	// =========================================================================
	// INITIALIZATION
	// =========================================================================

	UFUNCTION(BlueprintCallable, Category = "Progression|Init")
	void InitializeFromDataTables(
		UDataTable* CharacterUpgradeTable,
		UDataTable* ToolUpgradeTable,
		UDataTable* ShipRepairTable,
		const TMap<EPlayerStat, int32>& CharacterLevels,
		const TMap<FName, int32>&       ToolStatLevels,
		const TMap<EShipSystem, int32>& ShipLevels
	);

	// =========================================================================
	// COST QUERIES
	// =========================================================================

	/** Returns the current cost for this upgrade key. Empty struct if maxed/not found. */
	UFUNCTION(BlueprintPure, Category = "Progression|Cost")
	FUpgradeCost GetCurrentCost(FName UpgradeKey) const;

	/**
	 * True if the player has enough currency AND all required materials right now.
	 * This is the gate check — use it to enable/disable the upgrade button.
	 * PayTowardsUpgrade will only succeed if this returns true.
	 */
	UFUNCTION(BlueprintPure, Category = "Progression|Cost")
	bool CanPayAnything(FName UpgradeKey, AAlphaExilemetCharacter* Player) const;

	/** True if this upgrade key exists (upgrade not maxed). */
	UFUNCTION(BlueprintPure, Category = "Progression|Cost")
	bool HasUpgradeAvailable(FName UpgradeKey) const;

	// =========================================================================
	// TRANSACTION
	// =========================================================================

	/**
	 * ALL-OR-NOTHING payment. Only deducts if the player can afford everything.
	 *
	 * If CanAffordFull() == false: returns FALSE, touches nothing.
	 * If CanAffordFull() == true:  deducts full cost, returns TRUE.
	 *
	 * On TRUE the caller MUST:
	 *   1. Apply the upgrade (UpgradeStat / UpgradeShipSystem / etc.)
	 *   2. Call AdvanceToNextLevelCost(UpgradeKey, LevelJustReached)
	 */
	UFUNCTION(BlueprintCallable, Category = "Progression|Transaction")
	bool PayTowardsUpgrade(FName UpgradeKey, AAlphaExilemetCharacter* Player);

	/**
	 * Seeds the next level's cost after an upgrade has been applied.
	 * If already at max level the key is removed.
	 * LevelJustReached = old level + 1.
	 */
	UFUNCTION(BlueprintCallable, Category = "Progression|Transaction")
	void AdvanceToNextLevelCost(FName UpgradeKey, int32 LevelJustReached);

	// =========================================================================
	// SAVE / LOAD
	// =========================================================================

	UFUNCTION(BlueprintCallable, Category = "Progression|SaveLoad")
	void SaveToSaveObject(UAlphaExilemetSaveGame* SaveObject) const;

	UFUNCTION(BlueprintCallable, Category = "Progression|SaveLoad")
	void LoadFromSaveObject(const UAlphaExilemetSaveGame* SaveObject);

	// =========================================================================
	// DATA
	// =========================================================================

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Progression")
	TMap<FName, FUpgradeCost> RuntimeCosts;

private:
	UPROPERTY() UDataTable* CachedCharTable = nullptr;
	UPROPERTY() UDataTable* CachedToolTable = nullptr;
	UPROPERTY() UDataTable* CachedShipTable = nullptr;

	bool TryGetCostForLevel(FName UpgradeKey, int32 Level, FUpgradeCost& OutCost) const;
};