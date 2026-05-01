#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "AlphaExilemet/Data/AlphaExilemetTypes.h"
#include "BaseTransactionWidget.generated.h"

class AAlphaExilemetCharacter;
class UUpgradeProgressionManager;

/**
 * UBaseTransactionWidget
 *
 * Base class for ALL upgrade row widgets and sell widgets.
 *
 * ─────────────────────────────────────────────────────────────────────────────
 * HOW THE KEY SYSTEM WORKS — READ THIS ONCE
 * ─────────────────────────────────────────────────────────────────────────────
 *
 * The ProgressionManager stores every upgrade cost indexed by the DataTable
 * ROW NAME (an FName), e.g. "Health", "AtmosphericScrubber", "Pickaxe_Strength".
 *
 * The TERMINALS already loop over DataTable row names using GetDataTableRowNames.
 * They pass that row name (FName) into the row widget's Initialize function.
 * The row widget stores it as UpgradeKey and uses it for every subsequent call.
 *
 * Result: zero enum conversions, one set of functions, identical logic in all
 * three row widgets (WBP_SystemUpgradeRow, WBP_ShipRepairRow, WBP_ToolStatBlock).
 *
 * ─────────────────────────────────────────────────────────────────────────────
 * UPGRADE WIDGET CALL PATTERN (same for all three widgets)
 * ─────────────────────────────────────────────────────────────────────────────
 *
 *   [Init / CacheVariables]
 *     UpgradeKey (FName) is received from the terminal and stored as a variable.
 *
 *   [UpdateCostDisplay]
 *     GetRemainingCost(UpgradeKey) → Break FUpgradeCost → display currency + materials
 *
 *   [Button IsEnabled / colour]
 *     CanAffordFullNow(UpgradeKey, PlayerRef) → SetIsEnabled
 *
 *   [On Button Clicked]
 *     PayAndCheckComplete(UpgradeKey, PlayerRef)
 *       TRUE  → apply upgrade (UpgradeStat / UpgradeShipSystem / UpgradeStat on tool)
 *             → NotifyUpgradeComplete(UpgradeKey, NewLevel)
 *             → RefreshUI
 *       FALSE → RefreshUI (partial payment — cost reduced)
 *
 * ─────────────────────────────────────────────────────────────────────────────
 * SELL TERMINAL
 * ─────────────────────────────────────────────────────────────────────────────
 *   Still uses the legacy CanAfford / DeductCost below. Do not change it.
 */
UCLASS()
class ALPHAEXILEMET_API UBaseTransactionWidget : public UUserWidget
{
	GENERATED_BODY()

public:

	// =========================================================================
	// LEGACY API — Sell terminal only. Do NOT use in upgrade widgets.
	// =========================================================================

	UFUNCTION(BlueprintCallable, Category = "AlphaExilemet|Transaction")
	bool CanAfford(FUpgradeCost CostInfo, AAlphaExilemetCharacter* Player);

	UFUNCTION(BlueprintCallable, Category = "AlphaExilemet|Transaction")
	void DeductCost(FUpgradeCost CostInfo, AAlphaExilemetCharacter* Player);

	// =========================================================================
	// UPGRADE API — All three row widgets use these identical functions.
	// UpgradeKey = the DataTable Row Name passed in from the terminal.
	// =========================================================================

	/**
	 * Returns the current (possibly partially-paid) cost for this upgrade.
	 * Use this in UpdateCostDisplay instead of reading CostPerLevel from DataTable.
	 * Returns an empty FUpgradeCost (zeros) if the upgrade is maxed or key not found.
	 */
	UFUNCTION(BlueprintPure, Category = "AlphaExilemet|Transaction|Upgrades")
	FUpgradeCost GetRemainingCost(FName UpgradeKey) const;

	/**
	 * True if the player can cover 100% of the remaining cost right now.
	 * Use this to set button enabled state and colour.
	 * Note: PayAndCheckComplete always accepts partial amounts regardless.
	 */
	UFUNCTION(BlueprintPure, Category = "AlphaExilemet|Transaction|Upgrades")
	bool CanPayAnything(FName UpgradeKey, AAlphaExilemetCharacter* Player) const;

	/**
	 * True if this upgrade key still exists in the manager (upgrade not maxed).
	 * Use this to hide or disable the entire row when max level is reached.
	 */
	UFUNCTION(BlueprintPure, Category = "AlphaExilemet|Transaction|Upgrades")
	bool IsUpgradeAvailable(FName UpgradeKey) const;

	/**
	 * THE MAIN PURCHASE FUNCTION. Call this on upgrade button click.
	 *
	 * Deducts as much of the remaining cost as the player currently has.
	 * Works for ALL resource types: currency, ore (Pickaxe), slime (Vacuum),
	 * gas spheres (GasRod). No special casing needed per widget.
	 *
	 * Returns TRUE  = upgrade fully paid.
	 *   → Apply the upgrade effect (UpgradeStat / UpgradeShipSystem / etc.)
	 *   → Call NotifyUpgradeComplete(UpgradeKey, NewLevel)
	 *   → Refresh the UI
	 *
	 * Returns FALSE = partial payment only. Just refresh the UI to show
	 *   the reduced remaining cost. No upgrade effect applied.
	 */
	UFUNCTION(BlueprintCallable, Category = "AlphaExilemet|Transaction|Upgrades")
	bool PayAndCheckComplete(FName UpgradeKey, AAlphaExilemetCharacter* Player);

	/**
	 * Call AFTER a fully-paid upgrade has been applied.
	 * Seeds the next level's cost in the ProgressionManager.
	 * If the upgrade was already at max the key is removed automatically.
	 *
	 * LevelJustReached = the level the player just moved TO (old level + 1).
	 */
	UFUNCTION(BlueprintCallable, Category = "AlphaExilemet|Transaction|Upgrades")
	void NotifyUpgradeComplete(FName UpgradeKey, int32 LevelJustReached);

private:
	UUpgradeProgressionManager* GetProgressionManager() const;
};
