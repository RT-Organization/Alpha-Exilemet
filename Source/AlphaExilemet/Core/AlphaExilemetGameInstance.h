#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "AlphaExilemetSaveGame.h"
#include "AlphaExilemet/AlphaExilemetCharacter.h"
#include "UpgradeProgressionManager.h"
#include "AlphaExilemetGameInstance.generated.h"

// ─────────────────────────────────────────────────────────────────────────────
// EGamePhase
// Single source of truth for what the game is currently doing.
// Set BEFORE streaming any level. Read in OnAnyLevelStreamComplete.
// Never use level name strings for routing decisions.
// ─────────────────────────────────────────────────────────────────────────────
UENUM(BlueprintType)
enum class EGamePhase : uint8
{
	None             UMETA(DisplayName = "None"),
	MainMenu         UMETA(DisplayName = "Main Menu"),
	NewGame_Tutorial UMETA(DisplayName = "New Game — Tutorial"),
	Main             UMETA(DisplayName = "Main Gameplay"),
	LoadedGame       UMETA(DisplayName = "Loaded Game"),

	/**
	 * Set by AlphaStreamingSubsystem::EnterPortal() BEFORE streaming begins.
	 * Cleared back to Main by ExitPortal() BEFORE the unload completes.
	 *
	 * GM switch reads this in OnAnyLevelStreamComplete:
	 *   InPortal → TeleportPlayerToPortalStart + disable survival + fade out screen.
	 *   Main     → if loading screen ref is valid → fade out screen (covers portal exit).
	 */
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

	UFUNCTION(BlueprintCallable, Category = "AlphaExilemet|SaveLoad")
	void SavePlayerData();

	UFUNCTION(BlueprintCallable, Category = "AlphaExilemet|SaveLoad")
	void SetupPlayerData();

	UFUNCTION(BlueprintCallable, Category = "AlphaExilemet|SaveLoad")
	void SaveShipData();

	UFUNCTION(BlueprintCallable, Category = "AlphaExilemet|SaveLoad")
	void SetupShipData();

	/**
	 * One call that runs the full load sequence in the correct order.
	 * Call this from GM LoadGamePlayer instead of the four functions individually.
	 *
	 * Internally calls:
	 *   SetupPlayerData()        — teleports to saved position if bHasValidTransform
	 *   SetupShipData()          — restores ship repair levels
	 *   InitProgressionManager() — seeds upgrade costs from DataTables
	 *   SetupProgressionData()   — overlays saved partial payments
	 */
	UFUNCTION(BlueprintCallable, Category = "AlphaExilemet|SaveLoad")
	void SetupLoadedGame();

	// ── PROGRESSION FUNCTIONS ────────────────────────────────────────────────

	UFUNCTION(BlueprintCallable, Category = "AlphaExilemet|Progression")
	void InitProgressionManager();

	UFUNCTION(BlueprintCallable, Category = "AlphaExilemet|Progression")
	void SetupProgressionData();

	UFUNCTION(BlueprintCallable, Category = "AlphaExilemet|Progression")
	void SaveProgressionData();
};