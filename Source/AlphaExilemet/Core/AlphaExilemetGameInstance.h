#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "AlphaExilemetSaveGame.h"
#include "AlphaExilemet/AlphaExilemetCharacter.h"
#include "UpgradeProgressionManager.h"
#include "AlphaExilemetGameInstance.generated.h"

// ─────────────────────────────────────────────────────────────────────────────
// EGamePhase
//
// Single source of truth for what the game is currently doing.
// Replace ALL level-name string comparisons with this.
//
//   MainMenu        — sitting on the main menu, no gameplay.
//   NewGame_Tutorial— player started a new game; Tutorial is loading or active.
//   Main            — Tutorial complete; player is in main gameplay.
//   LoadedGame      — player loaded an existing save; spawning in Main.
//
// Set this BEFORE streaming any level so the GM knows what to do when
// OnStreamComplete fires.
// ─────────────────────────────────────────────────────────────────────────────

UENUM(BlueprintType)
enum class EGamePhase : uint8
{
    None             UMETA(DisplayName = "None"),
    MainMenu         UMETA(DisplayName = "Main Menu"),
    NewGame_Tutorial UMETA(DisplayName = "New Game — Tutorial"),
    Main             UMETA(DisplayName = "Main Gameplay"),
    LoadedGame       UMETA(DisplayName = "Loaded Game"),
};

UCLASS()
class ALPHAEXILEMET_API UAlphaExilemetGameInstance : public UGameInstance
{
    GENERATED_BODY()

public:
    // ── PHASE ────────────────────────────────────────────────────────────────

    /**
     * Current game phase. Set this BEFORE calling any StreamLevel.
     * GM_SimulatorGamemode reads this in OnStreamComplete to decide what to do.
     * Never check a level name string — always check this.
     */
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

    // ── UPGRADE PROGRESSION ──────────────────────────────────────────────────

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

    /**
     * Creates a fresh save with bHasValidTransform = false.
     * Sets CurrentPhase = NewGame_Tutorial automatically.
     */
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

    // ── PROGRESSION MANAGER ───────────────────────────────────────────────────

    UFUNCTION(BlueprintCallable, Category = "AlphaExilemet|Progression")
    void InitProgressionManager();

    UFUNCTION(BlueprintCallable, Category = "AlphaExilemet|Progression")
    void SetupProgressionData();

    UFUNCTION(BlueprintCallable, Category = "AlphaExilemet|Progression")
    void SaveProgressionData();
};
