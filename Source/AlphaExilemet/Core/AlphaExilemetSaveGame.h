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
	/* LEVEL                         */
	/* ----------------------------- */
	
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "SaveData|Level")
	FName CurrentLevelName;

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

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "SaveData|Level")
	FTransform PrePortalTransform;
	
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

	/**
	 * How many empty (unfired) spheres the gas rod was holding at save time.
	 * Full spheres are stored in SavedHarvestedGas (gas type → count).
	 */
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "SaveData|Tools")
	int32 SavedEmptySphereCount = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "SaveData|Tools")
	TArray<TSubclassOf<class AToolBase>> SavedOwnedToolClasses;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "SaveData|Tools")
	int32 SavedActiveToolIndex = -1;

	/* ----------------------------- */
	/* PARTIAL UPGRADE PAYMENTS      */
	/* ----------------------------- */
	/**
	 * Remaining cost for upgrades that have been partially paid.
	 * Key   = DataTable row name (the UpgradeKey used in widgets).
	 * Value = How much is still owed (currency + materials).
	 *
	 * Untouched upgrades → NOT stored here (re-seeded from DataTable on load).
	 * Fully-paid upgrades → NOT stored here (manager seeds the next level).
	 */
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "SaveData|Progression")
	TMap<FName, FUpgradeCost> SavedRemainingCosts;
};
