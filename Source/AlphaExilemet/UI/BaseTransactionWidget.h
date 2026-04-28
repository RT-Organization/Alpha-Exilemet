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
	// LEGACY API  (kept for backwards compatibility)
	// These check / deduct a static FUpgradeCost directly.
	// Use the PARTIAL PAYMENT API below for new upgrade widgets.
	// =========================================================================

	/** Checks if the player has enough currency AND all required materials. */
	UFUNCTION(BlueprintCallable, Category = "AlphaExilemet|Transaction")
	bool CanAfford(FUpgradeCost CostInfo, AAlphaExilemetCharacter* Player);

	/** Subtracts the full currency and materials cost from the player. */
	UFUNCTION(BlueprintCallable, Category = "AlphaExilemet|Transaction")
	void DeductCost(FUpgradeCost CostInfo, AAlphaExilemetCharacter* Player);

	// =========================================================================
	// PARTIAL PAYMENT API  (new — use this in all upgrade widgets)
	//
	// USAGE PATTERN IN BLUEPRINT (replace your old CanAfford + DeductCost flow):
	//
	//   [Upgrade Button OnClicked]
	//     → GetCurrentCostForUpgrade(RowName)      ← for display only
	//     → PayAndCheckComplete(RowName, Player)
	//         TRUE  → Apply upgrade (UpgradeStat / UpgradeShipSystem / etc.)
	//               → CompleteUpgrade(RowName, NewLevel)
	//               → RefreshUI
	//         FALSE → Show "partial payment" feedback, RefreshUI cost display
	//
	//   [Widget Construct / Refresh]
	//     → GetCurrentCostForUpgrade(RowName)      ← always read runtime cost, NOT from DT
	//     → CanAffordFullUpgrade(RowName, Player)   ← for button color / enable state
	// =========================================================================

	/**
	 * Returns the ProgressionManager from the owning GameInstance.
	 * Returns null if the GameInstance is wrong type or manager not initialized.
	 */
	UFUNCTION(BlueprintPure, Category = "AlphaExilemet|Transaction|Progression")
	UUpgradeProgressionManager* GetProgressionManager() const;

	/**
	 * Returns the current (possibly partially-paid) cost for this upgrade.
	 * Use this for cost display in your widgets — NOT DataTable CostPerLevel directly.
	 */
	UFUNCTION(BlueprintPure, Category = "AlphaExilemet|Transaction|Progression")
	FUpgradeCost GetCurrentCostForUpgrade(FName UpgradeKey) const;

	/**
	 * True if the player can cover the full remaining cost right now.
	 * Use this to enable/disable or color the Upgrade button.
	 * (PayAndCheckComplete works regardless — it accepts partial amounts.)
	 */
	UFUNCTION(BlueprintPure, Category = "AlphaExilemet|Transaction|Progression")
	bool CanAffordFullUpgrade(FName UpgradeKey, AAlphaExilemetCharacter* Player) const;

	/**
	 * True if this upgrade key still has a cost (i.e. upgrade is not maxed).
	 */
	UFUNCTION(BlueprintPure, Category = "AlphaExilemet|Transaction|Progression")
	bool IsUpgradeAvailable(FName UpgradeKey) const;

	/**
	 * Core purchase function. Deducts as much of the remaining cost as the
	 * player currently has — for every resource type (currency, ore, slime, gas).
	 *
	 * Returns TRUE  → upgrade fully paid. Caller must:
	 *                  1. Apply the upgrade (UpgradeStat / UpgradeShipSystem / etc.)
	 *                  2. Call CompleteUpgrade(UpgradeKey, LevelJustReached)
	 *
	 * Returns FALSE → partial payment made, cost reduced. Save will persist
	 *                  the remainder automatically.
	 */
	UFUNCTION(BlueprintCallable, Category = "AlphaExilemet|Transaction|Progression")
	bool PayAndCheckComplete(FName UpgradeKey, AAlphaExilemetCharacter* Player);

	/**
	 * Call this IMMEDIATELY after applying a fully-paid upgrade to seed the
	 * next level's cost into the ProgressionManager.
	 *
	 * @param UpgradeKey        Same FName used in PayAndCheckComplete.
	 * @param LevelJustReached  The level the player/ship/tool just reached.
	 *                          (If health went 1→2, pass 2.)
	 */
	UFUNCTION(BlueprintCallable, Category = "AlphaExilemet|Transaction|Progression")
	void CompleteUpgrade(FName UpgradeKey, int32 LevelJustReached);
};
