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
 * Base class for all upgrade / sell widgets.
 *
 * HOW THE KEY SYSTEM WORKS
 * ─────────────────────────
 * The UpgradeProgressionManager stores costs keyed by DataTable ROW NAME (FName).
 * In your DataTable the row names are:  "Health", "Oxygen", "Agility"  (character)
 *                                       "SHIP_Scrubber", ...            (ship)
 *                                       "Pickaxe_Strength", ...         (tools)
 *
 * Widgets do NOT pass FName directly.  Instead they pass the typed enum/id that
 * is already on their RowData struct, and this class converts it to the right key:
 *
 *   WBP_SystemUpgradeRow  → Row Data Stat ID  (EPlayerStat enum)
 *                           → use the PlayerStat overloads below
 *
 *   WBP_ShipRepairRow     → Row Data System ID (EShipSystem enum)
 *                           → use the ShipSystem overloads below
 *
 *   WBP_ToolStatBlock     → Cached Stat ID     (FName — already the row name)
 *                           → use the FName overloads directly
 *
 * The conversion is: UEnum::GetValueAsName() which returns e.g. "Health" for
 * EPlayerStat::Health and "AtmosphericScrubber" for EShipSystem::AtmosphericScrubber.
 * Make sure your DataTable row names match EXACTLY (case-sensitive).
 */
UCLASS()
class ALPHAEXILEMET_API UBaseTransactionWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	// =========================================================================
	// LEGACY API — keep for SellTerminal. NOT for upgrade widgets.
	// =========================================================================
	UFUNCTION(BlueprintCallable, Category = "AlphaExilemet|Transaction")
	bool CanAfford(FUpgradeCost CostInfo, AAlphaExilemetCharacter* Player);

	UFUNCTION(BlueprintCallable, Category = "AlphaExilemet|Transaction")
	void DeductCost(FUpgradeCost CostInfo, AAlphaExilemetCharacter* Player);

	// =========================================================================
	// UPGRADE API — EPlayerStat overloads (WBP_SystemUpgradeRow)
	//
	// Pass "Row Data Stat ID" (the EPlayerStat pin from Break Character Upgrade Row)
	// directly into these nodes. No FName conversion needed in Blueprint.
	// =========================================================================

	/** Cost display. Pass Row Data Stat ID. Replaces CostPerLevel[CurrentLevel]. */
	UFUNCTION(BlueprintPure, Category = "AlphaExilemet|Transaction|Character")
	FUpgradeCost GetRemainingCost_Stat(EPlayerStat StatID) const;

	/** Button colour / enable state. True = player can cover 100% of remaining cost. */
	UFUNCTION(BlueprintPure, Category = "AlphaExilemet|Transaction|Character")
	bool CanAffordFullNow_Stat(EPlayerStat StatID, AAlphaExilemetCharacter* Player) const;

	/** True if this upgrade has not been maxed. */
	UFUNCTION(BlueprintPure, Category = "AlphaExilemet|Transaction|Character")
	bool IsUpgradeAvailable_Stat(EPlayerStat StatID) const;

	/**
	 * Main purchase. Deducts as much as the player has.
	 * TRUE  = fully paid → call UpgradeStat then NotifyComplete_Stat.
	 * FALSE = partial payment → just refresh cost display.
	 */
	UFUNCTION(BlueprintCallable, Category = "AlphaExilemet|Transaction|Character")
	bool PayAndCheckComplete_Stat(EPlayerStat StatID, AAlphaExilemetCharacter* Player);

	/**
	 * Call after a fully-paid character upgrade has been applied.
	 * LevelJustReached = old level + 1.
	 */
	UFUNCTION(BlueprintCallable, Category = "AlphaExilemet|Transaction|Character")
	void NotifyUpgradeComplete_Stat(EPlayerStat StatID, int32 LevelJustReached);

	// =========================================================================
	// UPGRADE API — EShipSystem overloads (WBP_ShipRepairRow)
	//
	// Pass "Row Data System ID" (the EShipSystem pin from Break Ship Repair Row)
	// directly into these nodes.
	// =========================================================================

	UFUNCTION(BlueprintPure, Category = "AlphaExilemet|Transaction|Ship")
	FUpgradeCost GetRemainingCost_Ship(EShipSystem SystemID) const;

	UFUNCTION(BlueprintPure, Category = "AlphaExilemet|Transaction|Ship")
	bool CanAffordFullNow_Ship(EShipSystem SystemID, AAlphaExilemetCharacter* Player) const;

	UFUNCTION(BlueprintPure, Category = "AlphaExilemet|Transaction|Ship")
	bool IsUpgradeAvailable_Ship(EShipSystem SystemID) const;

	UFUNCTION(BlueprintCallable, Category = "AlphaExilemet|Transaction|Ship")
	bool PayAndCheckComplete_Ship(EShipSystem SystemID, AAlphaExilemetCharacter* Player);

	UFUNCTION(BlueprintCallable, Category = "AlphaExilemet|Transaction|Ship")
	void NotifyUpgradeComplete_Ship(EShipSystem SystemID, int32 LevelJustReached);

	// =========================================================================
	// UPGRADE API — FName overloads (WBP_ToolStatBlock)
	//
	// Cached Stat ID is already an FName row name — pass it directly.
	// =========================================================================

	UFUNCTION(BlueprintPure, Category = "AlphaExilemet|Transaction|Tool")
	FUpgradeCost GetRemainingCost(FName UpgradeKey) const;

	UFUNCTION(BlueprintPure, Category = "AlphaExilemet|Transaction|Tool")
	bool CanAffordFullNow(FName UpgradeKey, AAlphaExilemetCharacter* Player) const;

	UFUNCTION(BlueprintPure, Category = "AlphaExilemet|Transaction|Tool")
	bool IsUpgradeAvailable(FName UpgradeKey) const;

	UFUNCTION(BlueprintCallable, Category = "AlphaExilemet|Transaction|Tool")
	bool PayAndCheckComplete(FName UpgradeKey, AAlphaExilemetCharacter* Player);

	UFUNCTION(BlueprintCallable, Category = "AlphaExilemet|Transaction|Tool")
	void NotifyUpgradeComplete(FName UpgradeKey, int32 LevelJustReached);

	// =========================================================================
	// KEY CONVERSION UTILITIES (exposed to BP for debugging if needed)
	// =========================================================================

	/** Converts EPlayerStat to the DataTable row name FName used as UpgradeKey. */
	UFUNCTION(BlueprintPure, Category = "AlphaExilemet|Transaction|Utilities")
	static FName StatToUpgradeKey(EPlayerStat StatID);

	/** Converts EShipSystem to the DataTable row name FName used as UpgradeKey. */
	UFUNCTION(BlueprintPure, Category = "AlphaExilemet|Transaction|Utilities")
	static FName ShipSystemToUpgradeKey(EShipSystem SystemID);

private:
	UUpgradeProgressionManager* GetProgressionManager() const;
};
