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

	/**
	 * Saves player state to disk.
	 *
	 * TUTORIAL GUARD: No-op when CurrentLevelName == "Tutorial".
	 *   Tutorial always restarts — progress is never persisted.
	 *
	 * PORTAL: Saves the portal level name as-is.
	 *   On load, the player is streamed into the portal and the challenge
	 *   restarts from scratch. Only stats/tools are restored, not challenge state.
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
	 * Single entry point for the Load Menu.
	 * Must be called AFTER the loading screen is visible AND
	 * GM.LoadingScreenRef has been set.
	 *
	 * Routing:
	 *   "Tutorial" / none → NewGame_Tutorial → stream Tutorial
	 *   portal name       → InPortal         → stream portal, restart challenge
	 *   "Main"            → LoadedGame        → stream Main normally
	 *
	 * WB_LoadMenu Load Slot chain (minimal):
	 *   [Create Loading Screen → Add to Viewport → SET Ref Loading]
	 *   [Get Game Mode → Cast → SET LoadingScreenRef]
	 *   [Get Game Instance → Cast → LoadSaveAndStream(Slot Save)]
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
	bool IsPortalLevel(const FName& LevelName) const;
};
