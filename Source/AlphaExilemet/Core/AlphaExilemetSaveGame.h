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
	// Matches your "Character Upgrades" category
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "SaveData|Upgrades")
	TMap<EPlayerStat, int32> SavedUpgradeLevels;

	// Matches your "Ship Upgrades" category
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "SaveData|Upgrades")
	TMap<EShipSystem, int32> SavedShipRepairLevels;

	/* ----------------------------- */
	/* NEW: TOOL UPGRADES & INVENTORY*/
	/* ----------------------------- */
	// Stores all tool progression levels centrally
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
};