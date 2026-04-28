#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "AlphaExilemet/Data/AlphaExilemetTypes.h"
#include "AlphaExilemetSaveGame.generated.h"

UCLASS()
class ALPHAEXILEMET_API UAlphaExilemetSaveGame : public USaveGame
{
	GENERATED_BODY()

public:
	UAlphaExilemetSaveGame();

	/* ----------------------------- */
	/* STATUS                        */
	/* ----------------------------- */
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "SaveData|Status")
	float SavedHealth;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "SaveData|Status")
	float SavedOxygen;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "SaveData|Status")
	float SavedCurrency;

	/* ----------------------------- */
	/* POS & ROT                     */
	/* ----------------------------- */
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "SaveData|Pos&Rot")
	FTransform PlayerLocation;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "SaveData|Pos&Rot")
	FTransform PlayerCamera;

	/* ----------------------------- */
	/* UPGRADES                      */
	/* ----------------------------- */
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "SaveData|Upgrades")
	TMap<EPlayerStat, int32> SavedUpgradeLevels;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "SaveData|Upgrades")
	TMap<EShipSystem, int32> SavedShipRepairLevels;

	/* ----------------------------- */
	/* TOOL UPGRADES & INVENTORY     */
	/* ----------------------------- */
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "SaveData|Tools")
	TMap<FName, int32> SavedToolUpgrades;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "SaveData|Tools")
	TMap<FName, int32> SavedHarvestedOres;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "SaveData|Tools")
	TMap<FName, int32> SavedHarvestedSlime;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "SaveData|Tools")
	TMap<FName, int32> SavedHarvestedGas;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "SaveData|Tools")
	TArray<TSubclassOf<class AToolBase>> SavedOwnedToolClasses;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "SaveData|Tools")
	int32 SavedActiveToolIndex = -1;

	/* ----------------------------- */
	/* PARTIAL UPGRADE PAYMENTS      */
	/* ----------------------------- */
	/**
	 * Stores the REMAINING cost of every upgrade that has been partially paid.
	 * Key   = DataTable row name (same FName used as UpgradeKey in widgets).
	 * Value = How much is still owed (currency + materials).
	 *
	 * Fully-paid upgrades are NOT stored here — once an upgrade is complete
	 * UUpgradeProgressionManager seeds the next level's cost from the DataTable.
	 *
	 * Upgrades that have never been touched are also NOT stored here — they
	 * are re-seeded fresh from the DataTable on each game load.
	 */
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "SaveData|Progression")
	TMap<FName, FUpgradeCost> SavedRemainingCosts;
};
