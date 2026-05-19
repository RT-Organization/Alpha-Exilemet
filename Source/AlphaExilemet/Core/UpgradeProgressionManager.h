#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "AlphaExilemet/Data/AlphaExilemetTypes.h"
#include "UpgradeProgressionManager.generated.h"

class AAlphaExilemetCharacter;
class UAlphaExilemetSaveGame;
class UDataTable;

/**
 Single source of truth for live upgrade costs
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

	// Returns the current cost for this upgrade key
	UFUNCTION(BlueprintPure, Category = "Progression|Cost")
	FUpgradeCost GetCurrentCost(FName UpgradeKey) const;

	// True if the player has enough currency AND all required materials right now
	UFUNCTION(BlueprintPure, Category = "Progression|Cost")
	bool CanPayAnything(FName UpgradeKey, AAlphaExilemetCharacter* Player) const;

	// True if this upgrade key exists
	UFUNCTION(BlueprintPure, Category = "Progression|Cost")
	bool HasUpgradeAvailable(FName UpgradeKey) const;

	// =========================================================================
	// TRANSACTION
	// =========================================================================
	UFUNCTION(BlueprintCallable, Category = "Progression|Transaction")
	bool PayTowardsUpgrade(FName UpgradeKey, AAlphaExilemetCharacter* Player);

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