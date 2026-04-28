#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "AlphaExilemetSaveGame.h"
#include "AlphaExilemet/AlphaExilemetCharacter.h"
#include "AlphaExilemet/BaseCamp.h"
#include "UpgradeProgressionManager.h"
#include "AlphaExilemetGameInstance.generated.h"

UCLASS()
class ALPHAEXILEMET_API UAlphaExilemetGameInstance : public UGameInstance
{
	GENERATED_BODY()

public:
	// -------------------------------------------------------------------------
	// SAVE / LOAD
	// -------------------------------------------------------------------------
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "AlphaExilemet|SaveLoad")
	FString CurrentSaveSlot = "SaveSlot1";

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "AlphaExilemet|SaveLoad")
	UAlphaExilemetSaveGame* LocalSaveRef;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "AlphaExilemet|SaveLoad")
	AAlphaExilemetCharacter* PlayerRef;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "AlphaExilemet|SaveLoad")
	ABaseCamp* BaseRef;

	// -------------------------------------------------------------------------
	// UPGRADE PROGRESSION MANAGER
	// -------------------------------------------------------------------------

	/**
	 * The single source of truth for live upgrade costs at runtime.
	 * Widgets and terminals should read costs from here — NOT from DataTables.
	 *
	 * Set the three DataTable references below in the Blueprint CDO so the
	 * manager can seed itself on game start.
	 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AlphaExilemet|Progression")
	UUpgradeProgressionManager* ProgressionManager;

	/** Assign DT_CharacterUpgrades here in the GameInstance Blueprint CDO. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "AlphaExilemet|Progression|DataTables")
	UDataTable* CharacterUpgradeTable;

	/** Assign DT_ToolUpgrades here in the GameInstance Blueprint CDO. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "AlphaExilemet|Progression|DataTables")
	UDataTable* ToolUpgradeTable;

	/** Assign DT_ShipRepairs here in the GameInstance Blueprint CDO. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "AlphaExilemet|Progression|DataTables")
	UDataTable* ShipRepairTable;

	// -------------------------------------------------------------------------
	// CORE SAVE / LOAD FUNCTIONS (existing)
	// -------------------------------------------------------------------------
	UFUNCTION(BlueprintCallable, Category = "AlphaExilemet|SaveLoad")
	void SavePlayerData();

	UFUNCTION(BlueprintCallable, Category = "AlphaExilemet|SaveLoad")
	void SetupPlayerData();

	UFUNCTION(BlueprintCallable, Category = "AlphaExilemet|SaveLoad")
	void SaveShipData();

	UFUNCTION(BlueprintCallable, Category = "AlphaExilemet|SaveLoad")
	void SetupShipData();

	// -------------------------------------------------------------------------
	// PROGRESSION MANAGER FUNCTIONS (new)
	// -------------------------------------------------------------------------

	/**
	 * Creates the ProgressionManager (if needed) and seeds it from the DataTables
	 * using the CURRENT levels on PlayerRef and BaseRef.
	 *
	 * CALL ORDER: After SetupPlayerData() and SetupShipData() so levels are loaded.
	 * For a fresh game (no save), call it right after the player spawns.
	 */
	UFUNCTION(BlueprintCallable, Category = "AlphaExilemet|Progression")
	void InitProgressionManager();

	/**
	 * Overlays any previously-saved partial payments onto RuntimeCosts.
	 * Call AFTER InitProgressionManager().
	 */
	UFUNCTION(BlueprintCallable, Category = "AlphaExilemet|Progression")
	void SetupProgressionData();

	/**
	 * Saves the current RuntimeCosts (including partial payments) to LocalSaveRef.
	 * Call alongside SavePlayerData() and SaveShipData().
	 */
	UFUNCTION(BlueprintCallable, Category = "AlphaExilemet|Progression")
	void SaveProgressionData();
};
