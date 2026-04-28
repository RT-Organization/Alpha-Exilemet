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
 * Owned by the GameInstance as a UObject.
 *
 * PURPOSE:
 *   The DataTables are read-only at runtime. This object owns the MUTABLE copy
 *   of every upgrade cost. Partial payments eat into these copies; when an
 *   upgrade is fully paid the manager seeds the NEXT level's cost automatically.
 *
 * CALL ORDER (from GameInstance Blueprint):
 *
 *   On fresh game or after loading:
 *     1. SetupPlayerData()          — loads character levels into PlayerRef
 *     2. SetupShipData()            — loads ship levels into BaseRef
 *     3. InitProgressionManager()   — seeds costs using current levels
 *     4. SetupProgressionData()     — overlays any partial payments from save
 *
 *   On save:
 *     SavePlayerData()  + SaveShipData()  + SaveProgressionData()
 *
 * WIDGET USAGE (in WBP_SystemUpgradeRow, WBP_ShipRepairRow, WBP_ToolStatBlock):
 *   - Replace reading CostPerLevel[CurrentLevel] from DataTable with:
 *       GetCurrentCost(RowName)
 *   - Replace CanAfford + DeductCost with:
 *       PayAndCheckComplete(RowName, Player)  → if true → apply upgrade → CompleteUpgrade(RowName, NewLevel)
 */
UCLASS(BlueprintType)
class ALPHAEXILEMET_API UUpgradeProgressionManager : public UObject
{
	GENERATED_BODY()

public:
	// -------------------------------------------------------------------------
	// INITIALIZATION
	// -------------------------------------------------------------------------

	/**
	 * Seeds RuntimeCosts from the three upgrade DataTables for the NEXT level
	 * each upgrade needs. Already-existing entries (partial payments) are NOT
	 * overwritten.
	 *
	 * @param CharacterUpgradeTable   DT_CharacterUpgrades  (FCharacterUpgradeRow)
	 * @param ToolUpgradeTable        DT_ToolUpgrades       (FToolUpgradeRow)
	 * @param ShipRepairTable         DT_ShipRepairs        (FShipRepairRow)
	 * @param CharacterLevels         Player->SystemUpgradeLevels
	 * @param ToolStatLevels          Merged ToolUpgradeLevels from all OwnedTools
	 * @param ShipLevels              BaseCamp->ShipRepairLevels
	 */
	UFUNCTION(BlueprintCallable, Category = "Progression|Init")
	void InitializeFromDataTables(
		UDataTable* CharacterUpgradeTable,
		UDataTable* ToolUpgradeTable,
		UDataTable* ShipRepairTable,
		const TMap<EPlayerStat, int32>& CharacterLevels,
		const TMap<FName, int32>&       ToolStatLevels,
		const TMap<EShipSystem, int32>& ShipLevels
	);

	// -------------------------------------------------------------------------
	// COST QUERIES
	// -------------------------------------------------------------------------

	/**
	 * Returns the current (possibly partially-paid) cost for an upgrade.
	 * @param UpgradeKey  The DataTable row name (same FName you get from
	 *                    RowData.GetRowName() in the widget).
	 */
	UFUNCTION(BlueprintPure, Category = "Progression|Cost")
	FUpgradeCost GetCurrentCost(FName UpgradeKey) const;

	/**
	 * True if the player can cover the FULL remaining cost right now.
	 * Use this to color-code buttons or show tooltips — NOT to gate payment.
	 * PayTowardsUpgrade always accepts whatever the player has.
	 */
	UFUNCTION(BlueprintPure, Category = "Progression|Cost")
	bool CanAffordFull(FName UpgradeKey, AAlphaExilemetCharacter* Player) const;

	/** True if this upgrade key exists in the manager (i.e. not maxed). */
	UFUNCTION(BlueprintPure, Category = "Progression|Cost")
	bool HasUpgradeAvailable(FName UpgradeKey) const;

	// -------------------------------------------------------------------------
	// TRANSACTION — PRIMARY API FOR WIDGETS
	// -------------------------------------------------------------------------

	/**
	 * Deducts as much of the remaining cost as the player currently has:
	 *   - Currency first (floor to int, deduct what's available up to cost)
	 *   - Then each required material (deduct available up to required amount)
	 *
	 * Returns TRUE when the upgrade is NOW FULLY PAID (cost reached zero).
	 * When it returns true the caller MUST:
	 *   1. Apply the upgrade effect  (UpgradeStat / UpgradeShipSystem / UpgradeStat on Tool)
	 *   2. Call AdvanceToNextLevelCost(UpgradeKey, LevelJustReached)
	 *
	 * Returns FALSE if only a partial payment was made — remaining cost is
	 * stored and shown next time the widget opens.
	 */
	UFUNCTION(BlueprintCallable, Category = "Progression|Transaction")
	bool PayTowardsUpgrade(FName UpgradeKey, AAlphaExilemetCharacter* Player);

	/**
	 * Seeds the cost for the NEXT level after an upgrade has been applied.
	 * If the upgrade is already at max level the key is removed from the map.
	 *
	 * @param UpgradeKey      Same row name used in PayTowardsUpgrade.
	 * @param LevelJustReached  The level the player/ship/tool just reached
	 *                          (e.g. if they went 1→2, pass 2).
	 */
	UFUNCTION(BlueprintCallable, Category = "Progression|Transaction")
	void AdvanceToNextLevelCost(FName UpgradeKey, int32 LevelJustReached);

	// -------------------------------------------------------------------------
	// SAVE / LOAD
	// -------------------------------------------------------------------------

	/** Call from GameInstance SaveProgressionData(). */
	UFUNCTION(BlueprintCallable, Category = "Progression|SaveLoad")
	void SaveToSaveObject(UAlphaExilemetSaveGame* SaveObject) const;

	/** Call from GameInstance SetupProgressionData() AFTER InitializeFromDataTables. */
	UFUNCTION(BlueprintCallable, Category = "Progression|SaveLoad")
	void LoadFromSaveObject(const UAlphaExilemetSaveGame* SaveObject);

	// -------------------------------------------------------------------------
	// DATA
	// -------------------------------------------------------------------------

	/**
	 * Live costs — starts from DataTable values for the appropriate level,
	 * decremented by partial payments.
	 * Key = DataTable row name (FName).
	 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Progression")
	TMap<FName, FUpgradeCost> RuntimeCosts;

private:
	// Cached DT pointers for AdvanceToNextLevelCost look-ups.
	UPROPERTY()
	UDataTable* CachedCharTable = nullptr;
	UPROPERTY()
	UDataTable* CachedToolTable = nullptr;
	UPROPERTY()
	UDataTable* CachedShipTable = nullptr;

	/**
	 * Looks up the cost for a specific level from whichever DT owns this key.
	 * Returns false if the key is not found OR if Level is out of bounds (maxed).
	 */
	bool TryGetCostForLevel(FName UpgradeKey, int32 Level, FUpgradeCost& OutCost) const;

	/**
	 * Deducts as much of ResourceID as the player has, up to Needed.
	 * Returns how much was actually deducted.
	 */
	static int32 DeductPartial(AAlphaExilemetCharacter* Player, FName ResourceID, int32 Needed);
};
