#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "AlphaStreamingSubsystem.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnStreamComplete);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnTutorialToMainComplete);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnTutorialPlayerReady);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnPortalEnterStarted);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnPortalExitStarted);

/**
 * Fires at the START of ReturnToMainMenu(), before any I/O.
 * GM binds here to: destroy the player pawn, clear director refs,
 * show the loading screen widget, and start the fade-out animation.
 *
 * WB_Pause "Main Menu" button:
 *   1. Call StreamingSubsystem → ReturnToMainMenu()
 *   2. Remove WB_Pause from Parent
 *   ─── Everything else is handled by GM via this delegate ───
 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnReturnToMainMenuStarted);

UCLASS()
class ALPHAEXILEMET_API UAlphaStreamingSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	// ── DELEGATES ─────────────────────────────────────────────────────────────

	/** Fires when a level LOAD completes. Never fires for unloads.
	 *  NOTE: does NOT fire for the Tutorial→Main transition (use OnTutorialToMainComplete). */
	UPROPERTY(BlueprintAssignable, Category = "AlphaExilemet|Streaming")
	FOnStreamComplete OnStreamComplete;

	/** Fires when Tutorial→Main streaming finishes. GM binds → SpawnNewGamePlayer. */
	UPROPERTY(BlueprintAssignable, Category = "AlphaExilemet|Streaming")
	FOnTutorialToMainComplete OnTutorialToMainComplete;

	/** GM fires at end of SpawnNewGamePlayer. WB_TutorialBlackout binds → removes itself. */
	UPROPERTY(BlueprintAssignable, BlueprintCallable, Category = "AlphaExilemet|Streaming")
	FOnTutorialPlayerReady OnTutorialPlayerReady;

	/** Fires at START of EnterPortal() before I/O. GM creates loading screen here. */
	UPROPERTY(BlueprintAssignable, Category = "AlphaExilemet|Streaming")
	FOnPortalEnterStarted OnPortalEnterStarted;

	/** Fires when CompletePortalChallenge() is called. GM creates loading screen. */
	UPROPERTY(BlueprintAssignable, Category = "AlphaExilemet|Streaming")
	FOnPortalExitStarted OnPortalExitStarted;

	/**
	 * Fires at the START of ReturnToMainMenu(), before level I/O.
	 *
	 * GM MUST bind to this in InitializeInstance to:
	 *   1. Show loading screen widget (Add to Viewport, ZOrder 100).
	 *   2. Start Fade Out Sequence on the loading screen.
	 *   3. Destroy the player pawn (Get Player Character → Destroy Actor).
	 *   4. Clear TutorialDirectorRef and WakeUpDirectorRef (SET null).
	 *   5. Set CurrentPhase = MainMenu (already done in C++, but GM may use it).
	 *
	 * Nothing else is required — WB_Pause just calls ReturnToMainMenu and closes itself.
	 */
	UPROPERTY(BlueprintAssignable, Category = "AlphaExilemet|Streaming")
	FOnReturnToMainMenuStarted OnReturnToMainMenuStarted;

	// ── LEVEL TRACKING ────────────────────────────────────────────────────────

	/**
	 * The streaming sub-level that is currently active (loaded and visible).
	 * Updated automatically by every streaming function in this class.
	 *
	 * GM InitializeInstance MUST call SetCurrentActiveLevel("MainMenu") at startup
	 * so the subsystem starts in a known state.
	 *
	 * Values: "MainMenu", "Tutorial", "Main", or a portal level name.
	 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AlphaExilemet|Streaming")
	FName CurrentActiveLevel = NAME_None;

	// ── PUBLIC FUNCTIONS ──────────────────────────────────────────────────────

	/** Generic: load one level, optionally unload another. */
	UFUNCTION(BlueprintCallable, Category = "AlphaExilemet|Streaming")
	void StreamLevel(FName LevelToLoad, FName LevelToUnload);

	/**
	 * Returns the player to the Main Menu from ANY level (Tutorial, Main, or Portal).
	 * Uses CurrentActiveLevel to determine what to unload — no hardcoding needed.
	 *
	 * Fires OnReturnToMainMenuStarted BEFORE any I/O so the GM can show the
	 * loading screen and clean up the player immediately.
	 *
	 * WB_Pause "Main Menu" button usage:
	 *   [StreamingSubsystem → ReturnToMainMenu]
	 *   [Remove from Parent]          ← close the pause widget
	 *   ── GM handles all the rest via OnReturnToMainMenuStarted ──
	 */
	UFUNCTION(BlueprintCallable, Category = "AlphaExilemet|Streaming")
	void ReturnToMainMenu();

	/**
	 * Manually set the currently tracked active level.
	 * Call from GM InitializeInstance with "MainMenu" at game startup.
	 */
	UFUNCTION(BlueprintCallable, Category = "AlphaExilemet|Streaming")
	void SetCurrentActiveLevel(FName LevelName) { CurrentActiveLevel = LevelName; }

	/** Returns the name of the currently active streaming level. */
	UFUNCTION(BlueprintPure, Category = "AlphaExilemet|Streaming")
	FName GetCurrentActiveLevel() const { return CurrentActiveLevel; }

	/** Used by LoadSaveAndStream to set the active portal before streaming. */
	UFUNCTION(BlueprintCallable, Category = "AlphaExilemet|Streaming")
	void SetActivePortalName(FName PortalName) { ActivePortalName = PortalName; }

	/** Read-only access to current portal name (for BP debugging). */
	UFUNCTION(BlueprintPure, Category = "AlphaExilemet|Streaming")
	FName GetActivePortalName() const { return ActivePortalName; }

	/**
	 * Tutorial completion:
	 *   - Destroys all player tools and saves a clean state.
	 *   - Sets phase to Main.
	 *   - Loads Main, unloads Tutorial.
	 *   - Fires OnTutorialToMainComplete when done (NOT OnStreamComplete).
	 */
	UFUNCTION(BlueprintCallable, Category = "AlphaExilemet|Streaming")
	void HandleTutorialCompletion();

	/**
	 * Portal entry:
	 *   Phase = InPortal → OnPortalEnterStarted → save state
	 *   → load portal, unload CurrentActiveLevel.
	 */
	UFUNCTION(BlueprintCallable, Category = "AlphaExilemet|Streaming")
	void EnterPortal(FName PortalLevelName, FTransform PlayerReturnTransform);

	/**
	 * Teleports existing pawn to PlayerStart in the active portal level.
	 * Called from GM InPortal switch case after OnStreamComplete.
	 */
	UFUNCTION(BlueprintCallable, Category = "AlphaExilemet|Streaming")
	void TeleportPlayerToPortalStart();

	/**
	 * Portal exit:
	 *   Teleport to PrePortalTransform → survival on → phase = Main
	 *   → load Main, unload portal.
	 */
	UFUNCTION(BlueprintCallable, Category = "AlphaExilemet|Streaming")
	void ExitPortal();

	/**
	 * Called by challenge programmer when challenge is done.
	 * Re-enables survival + broadcasts OnPortalExitStarted.
	 */
	UFUNCTION(BlueprintCallable, Category = "AlphaExilemet|Streaming")
	void CompletePortalChallenge();

	/** DEBUG — force portal exit. Remove before shipping. */
	UFUNCTION(BlueprintCallable, Category = "AlphaExilemet|Streaming|Debug")
	void Debug_ForceCompleteChallenge();

private:
	FName ActivePortalName    = NAME_None;
	int32 LoadLatentUUID      = 0;
	int32 UnloadLatentUUID    = 1000;

	/**
	 * When true, OnStreamLevelLoaded will fire OnTutorialToMainComplete instead
	 * of OnStreamComplete. This prevents the GM's EGamePhase switch from running
	 * the Tutorial setup a second time on the same load event.
	 */
	bool bTutorialTransition  = false;

	UFUNCTION() void OnStreamLevelLoaded();
	UFUNCTION() void OnStreamLevelUnloaded();
};
