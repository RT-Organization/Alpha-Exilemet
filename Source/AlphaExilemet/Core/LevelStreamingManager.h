#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "LevelStreamingManager.generated.h"

// ─────────────────────────────────────────────────────────────────────────────
// ENUMS
// ─────────────────────────────────────────────────────────────────────────────

/**
 * Every level in the project as a typed enum.
 * This replaces all FName("Tutorial"), FName("Main") hardcoding everywhere.
 * Add new portal levels here as needed.
 */
UENUM(BlueprintType)
enum class EGameLevel : uint8
{
	None            UMETA(DisplayName = "None"),
	MainMenu        UMETA(DisplayName = "Main Menu"),
	Tutorial        UMETA(DisplayName = "Tutorial"),
	Main            UMETA(DisplayName = "Main"),
	Portal_Desert   UMETA(DisplayName = "Portal_Desert"),
	Portal_Canyon   UMETA(DisplayName = "Portal_Canyon"),
	Portal_Ice      UMETA(DisplayName = "Portal_Ice"),
	Portal_Lava     UMETA(DisplayName = "Portal_Lava"),
	Portal_Swamp    UMETA(DisplayName = "Portal_Swamp"),
};

// ─────────────────────────────────────────────────────────────────────────────
// DELEGATES
// ─────────────────────────────────────────────────────────────────────────────

/**
 * Fired when the new level is fully loaded and the minimum loading screen
 * display time has elapsed. This is the ONLY place where game logic should
 * react to a level transition completing.
 *
 * Parameter: the level that just finished loading.
 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnLevelTransitionComplete, EGameLevel, NewLevel);

/**
 * Fired at the START of a transition, before any I/O.
 * Use this to clean up the current level (destroy player, clear refs, etc.).
 *
 * Parameters: level we are leaving, level we are going to.
 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnLevelTransitionStarted, EGameLevel, FromLevel, EGameLevel, ToLevel);

// ─────────────────────────────────────────────────────────────────────────────
// ULevelStreamingManager
//
// SINGLE AUTHORITY for all level streaming in the project.
//
// DESIGN PRINCIPLES:
//   1. One function per transition type. No raw FName streaming anywhere else.
//   2. The manager owns the loading screen lifecycle completely:
//        - Show loading screen (fade in)
//        - Stream out old level
//        - Stream in new level
//        - Enforce MinLoadingScreenTime (minimum display seconds)
//        - Fade out loading screen
//        - Fire OnLevelTransitionComplete
//   3. CurrentLevel is always authoritative — set before any I/O fires.
//   4. All game logic (GM, directors, etc.) binds to OnLevelTransitionComplete
//      and reacts based on CurrentLevel. Nothing else drives game logic.
//
// HOW TO USE:
//   - GM BeginPlay: bind to OnLevelTransitionComplete and OnLevelTransitionStarted.
//   - WB_LoadMenu Load button: call LoadSavedGame(SlotName).
//   - WB_Pause Main Menu button: call ReturnToMainMenu().
//   - PortalBase: call EnterPortal(PortalLevel, ReturnTransform).
//   - AlphaStreamingSubsystem skull sequence: call CompleteMainTutorial().
//   - Challenge complete: call ExitPortal().
//   - Nobody else calls LoadStreamLevel or UnloadStreamLevel directly.
// ─────────────────────────────────────────────────────────────────────────────

UCLASS()
class ALPHAEXILEMET_API ULevelStreamingManager : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	// ── CONFIGURATION (set in BP GameInstance Class Defaults) ─────────────────

	/**
	 * The loading screen widget class to create during transitions.
	 * Set this in BP_GameInstance Class Defaults or assign from GM.
	 * Must expose a StartFadeIn() and StartFadeOut() Blueprint event.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "LevelStreaming|Config")
	TSubclassOf<UUserWidget> LoadingScreenClass;

	/**
	 * Minimum seconds the loading screen stays visible.
	 * Prevents the screen from flashing instantly on cached/fast loads.
	 * The loading screen will NOT fade out until BOTH the level is loaded
	 * AND this time has elapsed.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "LevelStreaming|Config")
	float MinLoadingScreenTime = 2.0f;

	// ── STATE (read-only, authoritative) ─────────────────────────────────────

	/** The level that is currently loaded and active. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "LevelStreaming|State")
	EGameLevel CurrentLevel = EGameLevel::None;

	/** True while a transition is in progress. Prevents double-transitions. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "LevelStreaming|State")
	bool bTransitionInProgress = false;

	// ── DELEGATES ─────────────────────────────────────────────────────────────

	/**
	 * Bind in GM BeginPlay (or InitializeInstance).
	 * Fires when the new level is loaded AND MinLoadingScreenTime has elapsed.
	 * This is the ONLY event that drives GM logic (spawn player, setup game, etc.)
	 */
	UPROPERTY(BlueprintAssignable, Category = "LevelStreaming")
	FOnLevelTransitionComplete OnLevelTransitionComplete;

	/**
	 * Bind in GM BeginPlay.
	 * Fires at the START of a transition.
	 * Use to: destroy player pawn, clear director refs, any pre-transition cleanup.
	 */
	UPROPERTY(BlueprintAssignable, Category = "LevelStreaming")
	FOnLevelTransitionStarted OnLevelTransitionStarted;

	// ── PUBLIC TRANSITION FUNCTIONS ───────────────────────────────────────────

	/**
	 * Called by BP_MenuPlanet (or any New Game button) when starting a fresh game.
	 *
	 * bSeamless = true  → NO loading screen. The level loads silently in the
	 *                      background. Use this when the MainMenu animation ends
	 *                      with the camera exactly where the Tutorial camera starts,
	 *                      so the transition is invisible. OnLevelTransitionComplete
	 *                      fires as soon as the level is loaded (no min time wait).
	 *
	 * bSeamless = false → Normal loading screen transition (same as LoadSavedGame).
	 *
	 * BP_MenuPlanet usage:
	 *   [StartNewGameTransition custom event]
	 *     → [animate planet / align camera]
	 *     → [Get Game Instance → Cast → Get Subsystem (LevelStreamingManager)]
	 *     → [StartNewGame (SlotName="SaveSlot1", bSeamless=true)]
	 *     → [Remove from Parent on any overlay widget]
	 */
	UFUNCTION(BlueprintCallable, Category = "LevelStreaming")
	void StartNewGame(const FString& SlotName, bool bSeamless = false);

	/**
	 * Called by WB_LoadMenu when the player picks a save slot.
	 * Reads the save file, determines the correct level, shows loading screen,
	 * unloads MainMenu, loads the target level.
	 *
	 * WB_LoadMenu only needs to call this ONE function and then Remove from Parent.
	 * Everything else (loading screen, phase, streaming) is handled internally.
	 */
	UFUNCTION(BlueprintCallable, Category = "LevelStreaming")
	void LoadSavedGame(const FString& SlotName);

	/**
	 * Called by WB_Pause "Main Menu" button.
	 * Shows loading screen, destroys player, unloads current level, loads MainMenu.
	 *
	 * WB_Pause only needs to call this ONE function and then Remove from Parent.
	 */
	UFUNCTION(BlueprintCallable, Category = "LevelStreaming")
	void ReturnToMainMenu();

	/**
	 * Called by the skull sequence end (via AlphaStreamingSubsystem).
	 * Clears player tools, shows loading screen, unloads Tutorial, loads Main.
	 * Fires OnLevelTransitionComplete(Main) when done — GM calls InitializeWakeUp.
	 */
	UFUNCTION(BlueprintCallable, Category = "LevelStreaming")
	void CompleteTutorialAndLoadMain();

	/**
	 * Called by PortalBase::EnterPortalLevel().
	 * Shows loading screen, saves return transform, unloads Main, loads portal level.
	 */
	UFUNCTION(BlueprintCallable, Category = "LevelStreaming")
	void EnterPortal(EGameLevel PortalLevel, FTransform PlayerReturnTransform);

	/**
	 * Called when the portal challenge is complete.
	 * Shows loading screen, restores player to return transform, unloads portal, loads Main.
	 */
	UFUNCTION(BlueprintCallable, Category = "LevelStreaming")
	void ExitPortal();

	/**
	 * Debug: force complete a portal challenge.
	 */
	UFUNCTION(BlueprintCallable, Category = "LevelStreaming|Debug")
	void Debug_ForceExitPortal();

	// ── SETUP ─────────────────────────────────────────────────────────────────

	/**
	 * Called from GM InitializeInstance.
	 * Tells the manager that the game started on MainMenu (L_Persistent already loaded it).
	 * Also passes the loading screen class if not set in CDO.
	 */
	UFUNCTION(BlueprintCallable, Category = "LevelStreaming")
	void InitializeAtMainMenu(TSubclassOf<UUserWidget> InLoadingScreenClass);

	// ── PORTAL HELPERS (called by subsystem for backward compat) ─────────────

	/** Teleports the player pawn to the PlayerStart in the active portal. */
	UFUNCTION(BlueprintCallable, Category = "LevelStreaming")
	void TeleportPlayerToPortalStart();

	/** Re-enables survival and fires OnPortalExitStarted (kept for challenge code). */
	UFUNCTION(BlueprintCallable, Category = "LevelStreaming")
	void CompletePortalChallenge();

	// ── UTILITY ───────────────────────────────────────────────────────────────

	/** Convert EGameLevel to the FName used by LoadStreamLevel. */
	UFUNCTION(BlueprintPure, Category = "LevelStreaming")
	static FName LevelToName(EGameLevel Level);

	/** Convert FName from save file to EGameLevel. */
	UFUNCTION(BlueprintPure, Category = "LevelStreaming")
	static EGameLevel NameToLevel(FName Name);

	/** True if this level is a portal. */
	UFUNCTION(BlueprintPure, Category = "LevelStreaming")
	static bool IsPortalLevel(EGameLevel Level);

	// ── KEPT FOR LEGACY / PORTAL CHALLENGE CODE ───────────────────────────────

	/**
	 * Legacy delegates — kept so existing portal challenge Blueprint code
	 * that binds to these doesn't need to change.
	 */
	UPROPERTY(BlueprintAssignable, Category = "LevelStreaming|Legacy")
	FOnLevelTransitionStarted OnPortalEnterComplete;   // fires after portal loads

	UPROPERTY(BlueprintAssignable, Category = "LevelStreaming|Legacy")
	FOnLevelTransitionStarted OnPortalExitStarted;     // fires when challenge done

private:
	// ── INTERNAL STATE ────────────────────────────────────────────────────────

	EGameLevel PendingLevel      = EGameLevel::None;
	EGameLevel PortalReturnFrom  = EGameLevel::Main; // what to return to after portal

	FTransform PortalReturnTransform;

	bool  bLevelLoaded          = false;
	bool  bMinTimeElapsed       = false;
	float LoadingScreenElapsed  = 0.0f;

	/**
	 * When true the current transition skips the loading screen entirely.
	 * Both bLevelLoaded and bMinTimeElapsed are considered immediately met.
	 * Used for the MainMenu→Tutorial seamless camera handoff in BP_MenuPlanet.
	 */
	bool  bSeamlessTransition   = false;

	int32 LoadLatentUUID        = 0;
	int32 UnloadLatentUUID      = 1000;

	/**
	 * Weak pointer to the active loading screen widget.
	 * TWeakObjectPtr is used instead of a raw UPROPERTY pointer so that
	 * HideLoadingScreen() can null our reference while the widget itself
	 * remains alive in UMG memory to complete its fade-out animation and
	 * call Remove from Parent. A raw UPROPERTY pointer would keep the widget
	 * alive even after Remove from Parent, preventing GC.
	 */
	TWeakObjectPtr<UUserWidget> ActiveLoadingScreen;

	// ── INTERNAL TRANSITION PIPELINE ──────────────────────────────────────────

	/**
	 * The single internal function that drives every transition.
	 *
	 * Order:
	 *   1. Guard (bTransitionInProgress)
	 *   2. Fire OnLevelTransitionStarted(CurrentLevel, NewLevel)
	 *   3. Broadcast pre-cleanup to GM
	 *   4. Show loading screen (fade in)
	 *   5. Start MinLoadingScreenTime timer
	 *   6. Unload OldLevel (if not None)
	 *   7. Load NewLevel → OnLevelLoaded callback
	 *   8. When BOTH loaded AND min time elapsed → OnBothConditionsMet
	 *   9. Fade out loading screen
	 *  10. Fire OnLevelTransitionComplete(NewLevel)
	 *  11. Clear bTransitionInProgress
	 */
	void BeginTransition(EGameLevel NewLevel, EGameLevel OldLevel, bool bSeamless = false);

	void ShowLoadingScreen();
	void HideLoadingScreen();

	UFUNCTION() void OnLevelLoaded();
	UFUNCTION() void OnLevelUnloaded();
	void OnMinTimeElapsed();
	void OnBothConditionsMet(); // called when loaded AND min time elapsed

	FTimerHandle MinTimeHandle;

	// Player cleanup helpers
	void DestroyPlayerPawn();
	class AAlphaExilemetCharacter* GetLocalPlayer() const;
};