#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "AlphaStreamingSubsystem.generated.h"

// ─────────────────────────────────────────────────────────────────────────────
// DELEGATES
// ─────────────────────────────────────────────────────────────────────────────

/** Fires on EVERY level-load or level-unload completion. */
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnStreamComplete);

/** Fires ONLY when the Tutorial→Main transition finishes loading. */
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnTutorialToMainComplete);

/**
 * Broadcast by GM_SimulatorGamemode at the END of SpawnNewGamePlayer,
 * after the player is fully initialized and the level is ready.
 * WB_TutorialBlackout binds here and plays its fade-out.
 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnTutorialPlayerReady);

/**
 * Fires at the START of EnterPortal(), BEFORE streaming begins.
 * GM BP: bind → create WB_LoadingScreen, add to viewport, store ref.
 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnPortalEnterStarted);

/**
 * Fires when CompletePortalChallenge() is called.
 * GM BP: bind → create WB_LoadingScreen, short delay, call ExitPortal().
 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnPortalExitStarted);

// ─────────────────────────────────────────────────────────────────────────────

UCLASS()
class ALPHAEXILEMET_API UAlphaStreamingSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	// =========================================================================
	// DELEGATES  (BP binds here)
	// =========================================================================

	UPROPERTY(BlueprintAssignable, Category = "AlphaExilemet|Streaming")
	FOnStreamComplete OnStreamComplete;

	/**
	 * GM_SimulatorGamemode binds here → calls SpawnNewGamePlayer.
	 * WB_TutorialBlackout does NOT bind here.
	 */
	UPROPERTY(BlueprintAssignable, Category = "AlphaExilemet|Streaming")
	FOnTutorialToMainComplete OnTutorialToMainComplete;

	/**
	 * GM fires this at the END of SpawnNewGamePlayer.
	 * WB_TutorialBlackout binds here → fades out when it fires.
	 */
	UPROPERTY(BlueprintAssignable, BlueprintCallable, Category = "AlphaExilemet|Streaming")
	FOnTutorialPlayerReady OnTutorialPlayerReady;

	/** GM BP: bind → create loading screen immediately, BEFORE streaming starts. */
	UPROPERTY(BlueprintAssignable, Category = "AlphaExilemet|Streaming")
	FOnPortalEnterStarted OnPortalEnterStarted;

	/** GM BP: bind → create loading screen, delay, call ExitPortal(). */
	UPROPERTY(BlueprintAssignable, Category = "AlphaExilemet|Streaming")
	FOnPortalExitStarted OnPortalExitStarted;

	// =========================================================================
	// PUBLIC FUNCTIONS
	// =========================================================================

	/** Generic: loads one level, optionally unloads another. Pass NAME_None to skip unload. */
	UFUNCTION(BlueprintCallable, Category = "AlphaExilemet|Streaming")
	void StreamLevel(FName LevelToLoad, FName LevelToUnload);

	/** Saves progress → loads Main → unloads Tutorial. Fires OnTutorialToMainComplete when done. */
	UFUNCTION(BlueprintCallable, Category = "AlphaExilemet|Streaming")
	void HandleTutorialCompletion();

	/**
	 * Sets CurrentPhase = InPortal, broadcasts OnPortalEnterStarted (loading screen),
	 * saves player state + PrePortalTransform, then loads the portal level on top of Main.
	 * Main is NEVER unloaded. OnStreamComplete fires when loading is done.
	 */
	UFUNCTION(BlueprintCallable, Category = "AlphaExilemet|Streaming")
	void EnterPortal(FName PortalLevelName, FTransform PlayerReturnTransform);

	/**
	 * Teleports the existing player pawn to the PlayerStart inside the currently
	 * active portal streaming level — no pawn destroy/respawn needed.
	 *
	 * Call from the GM BP InPortal switch case AFTER OnStreamComplete fires.
	 */
	UFUNCTION(BlueprintCallable, Category = "AlphaExilemet|Streaming")
	void TeleportPlayerToPortalStart();

	/**
	 * Sets CurrentPhase = Main, teleports player back to PrePortalTransform (already
	 * yaw-rotated by APortalBase), re-enables survival, then unloads the portal level.
	 * OnStreamComplete fires when the unload is done → GM fades out the loading screen.
	 *
	 * Call from GM BP AFTER the loading screen is fully opaque.
	 */
	UFUNCTION(BlueprintCallable, Category = "AlphaExilemet|Streaming")
	void ExitPortal();

	/**
	 * Called by the other programmer when their challenge is complete.
	 * Re-enables survival, broadcasts OnPortalExitStarted → GM shows loading
	 * screen and calls ExitPortal().
	 */
	UFUNCTION(BlueprintCallable, Category = "AlphaExilemet|Streaming")
	void CompletePortalChallenge();

	/**
	 * DEBUG ONLY — simulates challenge completion to test the return-to-Main flow.
	 * Bind to a key in the GM BP (e.g. keyboard O or F9). Remove before shipping.
	 */
	UFUNCTION(BlueprintCallable, Category = "AlphaExilemet|Streaming|Debug")
	void Debug_ForceCompleteChallenge();

private:
	FName ActivePortalName    = NAME_None;
	int32 LatentUUID          = 0;
	bool  bTutorialTransition = false;

	UFUNCTION()
	void OnStreamLevelLoaded();
};