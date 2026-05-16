#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "AlphaExilemetSaveGame.h"
#include "AlphaExilemet/AlphaExilemetCharacter.h"
#include "UpgradeProgressionManager.h"
#include "AlphaExilemetGameInstance.generated.h"

// ─────────────────────────────────────────────────────────────────────────────
// EGamePhase  (unchanged)
// ─────────────────────────────────────────────────────────────────────────────
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

	/** Creates a fresh save. Sets CurrentPhase = NewGame_Tutorial automatically. */
	UFUNCTION(BlueprintCallable, Category = "AlphaExilemet|SaveLoad")
	void CreateNewGame(FString SlotName);

	/**
	 * Saves player state to disk.
	 *
	 * TUTORIAL GUARD: If CurrentLevelName is "Tutorial", this is a no-op.
	 * Tutorial progress is never persisted — quitting in Tutorial will restart
	 * the Tutorial on next load. Only Main-level state is ever written.
	 *
	 * PORTAL GUARD: If CurrentLevelName is a portal level name, the save writes
	 * CurrentLevelName = "Main" instead (so on load, the player resumes in Main,
	 * not stuck in a portal that needs to be re-streamed from scratch).
	 * Portal challenges always restart on load — progress inside them is not saved.
	 */
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
	 * Single entry point for loading any save slot from the Load Menu.
	 * Replaces the entire WB_LoadMenu "Load Slot Game" BP chain.
	 *
	 * What it does in order:
	 *   1. Loads the save file from disk → sets LocalSaveRef.
	 *   2. Reads CurrentLevelName from the save.
	 *   3. Decides which level to stream and what phase to enter:
	 *        "Tutorial" → phase = NewGame_Tutorial, load Tutorial (restart from scratch)
	 *        portal name → phase = LoadedGame, load Main only (portal is restarted fresh)
	 *        "Main"     → phase = LoadedGame, load Main normally
	 *   4. Sets CurrentPhase.
	 *   5. Starts streaming via AlphaStreamingSubsystem.
	 *      GM's OnAnyLevelStreamComplete fires when done → routes by phase.
	 *
	 * The loading screen must already be visible before calling this.
	 * Call from WB_LoadMenu Load Slot:
	 *   [Create + Add Loading Screen] → [LoadSaveAndStream(SlotName)]
	 *   That replaces the entire old BP chain.
	 */
	UFUNCTION(BlueprintCallable, Category = "AlphaExilemet|SaveLoad")
	void LoadSaveAndStream(FString SlotName);

	// ── PROGRESSION FUNCTIONS ────────────────────────────────────────────────

	UFUNCTION(BlueprintCallable, Category = "AlphaExilemet|Progression")
	void InitProgressionManager();

	UFUNCTION(BlueprintCallable, Category = "AlphaExilemet|Progression")
	void SetupProgressionData();

	UFUNCTION(BlueprintCallable, Category = "AlphaExilemet|Progression")
	void SaveProgressionData();

private:
	/**
	 * Set of level names that are portal levels (not Tutorial, not Main).
	 * Used by SavePlayerData() to detect portal-level saves and redirect them to Main.
	 * If you add a new portal level, add its FName here.
	 * Alternatively, check if CurrentLevelName != "Tutorial" && != "Main".
	 */
	bool IsPortalLevel(const FName& LevelName) const;
};