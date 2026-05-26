#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "AlphaExilemetSaveGame.h"
#include "AlphaExilemet/AlphaExilemetCharacter.h"
#include "UpgradeProgressionManager.h"
#include "AlphaExilemetGameInstance.generated.h"

UENUM(BlueprintType)
enum class EGamePhase : uint8
{
	None             UMETA(DisplayName = "None"),
	MainMenu         UMETA(DisplayName = "Main Menu"),
	NewGame_Tutorial UMETA(DisplayName = "New Game — Tutorial"),
	Main             UMETA(DisplayName = "Main Gameplay"),
	LoadedGame       UMETA(DisplayName = "Loaded Game"),
	InPortal         UMETA(DisplayName = "In Portal Challenge"),
};

UCLASS()
class ALPHAEXILEMET_API UAlphaExilemetGameInstance : public UGameInstance
{
	GENERATED_BODY()

public:
	// ── PHASE ────────────────────────────────────────────────────────────────

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "AlphaExilemet|Phase")
	EGamePhase CurrentPhase = EGamePhase::None;

	// ── SAVE / LOAD ──────────────────────────────────────────────────────────

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "AlphaExilemet|SaveLoad")
	FString CurrentSaveSlot = "SaveSlot1";

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "AlphaExilemet|SaveLoad")
	UAlphaExilemetSaveGame* LocalSaveRef;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "AlphaExilemet|SaveLoad")
	AAlphaExilemetCharacter* PlayerRef;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "AlphaExilemet|SaveLoad")
	ABaseCamp* BaseRef;

	// ── PROGRESSION ──────────────────────────────────────────────────────────

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AlphaExilemet|Progression")
	UUpgradeProgressionManager* ProgressionManager;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "AlphaExilemet|Progression|DataTables")
	UDataTable* CharacterUpgradeTable;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "AlphaExilemet|Progression|DataTables")
	UDataTable* ToolUpgradeTable;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "AlphaExilemet|Progression|DataTables")
	UDataTable* ShipRepairTable;

	// ── SAVE / LOAD FUNCTIONS ────────────────────────────────────────────────

	UFUNCTION(BlueprintCallable, Category = "AlphaExilemet|SaveLoad")
	bool DoesSaveExist(FString SlotName);

	UFUNCTION(BlueprintCallable, Category = "AlphaExilemet|SaveLoad")
	void CreateNewGame(FString SlotName);

	UFUNCTION(BlueprintCallable, Category = "AlphaExilemet|SaveLoad")
	void SavePlayerData();

	UFUNCTION(BlueprintCallable, Category = "AlphaExilemet|SaveLoad")
	void SetupPlayerData();

	UFUNCTION(BlueprintCallable, Category = "AlphaExilemet|SaveLoad")
	void SaveShipData();

	UFUNCTION(BlueprintCallable, Category = "AlphaExilemet|SaveLoad")
	void SetupShipData();

	UFUNCTION(BlueprintCallable, Category = "AlphaExilemet|SaveLoad")
	void SetupLoadedGame();

	/**
	 * PRIMARY LOAD ENTRY POINT.
	 * Delegates to LevelStreamingManager::LoadSavedGame().
	 * Call from WB_LoadMenu Load Slot button — then Remove from Parent.
	 * Everything else (loading screen, unload, load, phase, delegates) is handled internally.
	 */
	UFUNCTION(BlueprintCallable, Category = "AlphaExilemet|SaveLoad")
	void LoadSavedGame(FString SlotName);

	/**
	 * @deprecated Use LoadSavedGame(). Kept for BP backward compatibility.
	 */
	UFUNCTION(BlueprintCallable, Category = "AlphaExilemet|SaveLoad",
		meta = (DeprecatedFunction, DeprecationMessage = "Use LoadSavedGame() instead."))
	void LoadSaveAndStream(FString SlotName);

	// ── PROGRESSION FUNCTIONS ────────────────────────────────────────────────

	UFUNCTION(BlueprintCallable, Category = "AlphaExilemet|Progression")
	void InitProgressionManager();

	UFUNCTION(BlueprintCallable, Category = "AlphaExilemet|Progression")
	void SetupProgressionData();

	UFUNCTION(BlueprintCallable, Category = "AlphaExilemet|Progression")
	void SaveProgressionData();
	
	
	
	UFUNCTION(BlueprintCallable, Category = "AlphaExilemet|SaveLoad")
	void SyncSaveDataBeforeManualSave();
	
	/** True after WakeUp sequence has played once. Prevents replay on portal exit. */
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "AlphaExilemet|Phase")
	bool bWakeUpSequencePlayed = false;
};
