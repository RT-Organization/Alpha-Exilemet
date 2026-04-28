#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "AlphaExilemet/Data/AlphaExilemetTypes.h"
#include "BaseTransactionWidget.generated.h"

class AAlphaExilemetCharacter;
class UUpgradeProgressionManager;

UCLASS()
class ALPHAEXILEMET_API UBaseTransactionWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	// =========================================================================
	// LEGACY API — Keep for SellTerminal. Do NOT use in upgrade widgets.
	// =========================================================================
	UFUNCTION(BlueprintCallable, Category = "AlphaExilemet|Transaction")
	bool CanAfford(FUpgradeCost CostInfo, AAlphaExilemetCharacter* Player);

	UFUNCTION(BlueprintCallable, Category = "AlphaExilemet|Transaction")
	void DeductCost(FUpgradeCost CostInfo, AAlphaExilemetCharacter* Player);

	// =========================================================================
	// NEW UPGRADE API
	// Use these in WBP_SystemUpgradeRow, WBP_ShipRepairRow, WBP_ToolStatBlock.
	//
	// UpgradeKey = the DataTable Row Name (FName) for this upgrade entry.
	// Get it in BP via: right-click RowData variable → "Get Row Name".
	// =========================================================================

	/** Returns the live remaining cost. Use this for display instead of CostPerLevel[N]. */
	UFUNCTION(BlueprintPure, Category = "AlphaExilemet|Transaction|Upgrades")
	FUpgradeCost GetRemainingCost(FName UpgradeKey) const;

	/** True if the player can cover 100% of the remaining cost right now. Use for button colour. */
	UFUNCTION(BlueprintPure, Category = "AlphaExilemet|Transaction|Upgrades")
	bool CanAffordFullNow(FName UpgradeKey, AAlphaExilemetCharacter* Player) const;

	/** True if this upgrade has not been maxed out. */
	UFUNCTION(BlueprintPure, Category = "AlphaExilemet|Transaction|Upgrades")
	bool IsUpgradeAvailable(FName UpgradeKey) const;

	/**
	 * THE MAIN PURCHASE FUNCTION — call on button click.
	 * Deducts as much as the player has (partial payment is valid).
	 * Returns TRUE  = fully paid. Then: apply upgrade + call NotifyUpgradeComplete.
	 * Returns FALSE = partial payment only. Just refresh the cost display.
	 */
	UFUNCTION(BlueprintCallable, Category = "AlphaExilemet|Transaction|Upgrades")
	bool PayAndCheckComplete(FName UpgradeKey, AAlphaExilemetCharacter* Player);

	/**
	 * Call AFTER applying a fully-paid upgrade to seed the next level's cost.
	 * LevelJustReached = the level the player just moved TO (old level + 1).
	 */
	UFUNCTION(BlueprintCallable, Category = "AlphaExilemet|Transaction|Upgrades")
	void NotifyUpgradeComplete(FName UpgradeKey, int32 LevelJustReached);

private:
	UUpgradeProgressionManager* GetProgressionManager() const;
};
