#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "AlphaStreamingSubsystem.generated.h"

// ─────────────────────────────────────────────────────────────────────────────
// DELEGATES
// ─────────────────────────────────────────────────────────────────────────────

/** Fires on EVERY level-load completion (portal in, portal out, tutorial→main). */
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnStreamComplete);

/** Fires ONLY when the Tutorial→Main transition finishes loading. */
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnTutorialToMainComplete);

/**
 * Fires at the START of EnterPortal(), before the level begins streaming.
 * GameMode BP binds here → creates WB_LoadingScreen immediately.
 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnPortalEnterStarted);

/**
 * Fires when CompletePortalChallenge() is called by the challenge programmer.
 * GameMode BP binds here → creates WB_LoadingScreen, then calls ExitPortal().
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

	/** Broadcast whenever any streamed level finishes loading. */
	UPROPERTY(BlueprintAssignable, Category = "AlphaExilemet|Streaming")
	FOnStreamComplete OnStreamComplete;

	/**
	 * Broadcast ONLY when the Tutorial → Main transition completes.
	 * WB_TutorialBlackout binds to this in Event Construct and calls
	 * GameMode::SpawnPlayerFromTutorial when it fires.
	 */
	UPROPERTY(BlueprintAssignable, Category = "AlphaExilemet|Streaming")
	FOnTutorialToMainComplete OnTutorialToMainComplete;

	/**
	 * Fires at the START of EnterPortal() — before any streaming begins.
	 * GameMode BP: bind → Create WB_LoadingScreen and store the ref.
	 */
	UPROPERTY(BlueprintAssignable, Category = "AlphaExilemet|Streaming")
	FOnPortalEnterStarted OnPortalEnterStarted;

	/**
	 * Fires when the challenge programmer calls CompletePortalChallenge().
	 * GameMode BP: bind → Create WB_LoadingScreen → small delay → ExitPortal().
	 */
	UPROPERTY(BlueprintAssignable, Category = "AlphaExilemet|Streaming")
	FOnPortalExitStarted OnPortalExitStarted;

	// =========================================================================
	// PUBLIC FUNCTIONS
	// =========================================================================

	/**
	 * Generic: loads one level and (optionally) unloads another.
	 * Pass NAME_None as LevelToUnload to skip the unload step.
	 */
	UFUNCTION(BlueprintCallable, Category = "AlphaExilemet|Streaming")
	void StreamLevel(FName LevelToLoad, FName LevelToUnload);

	/** Saves progress → loads Main → unloads Tutorial. Fires OnTutorialToMainComplete on completion. */
	UFUNCTION(BlueprintCallable, Category = "AlphaExilemet|Streaming")
	void HandleTutorialCompletion();

	/**
	 * Loads the portal level ON TOP of Main (Main stays resident).
	 * Saves PrePortalTransform (already yaw-offset by APortalBase) and sets
	 * ActivePortalName for ExitPortal().
	 *
	 * Fires OnPortalEnterStarted immediately so the GameMode BP can show
	 * the loading screen before streaming begins.
	 */
	UFUNCTION(BlueprintCallable, Category = "AlphaExilemet|Streaming")
	void EnterPortal(FName PortalLevelName, FTransform PlayerEntryTransform);

	/**
	 * Unloads the active portal level and teleports the player back to the
	 * position saved by EnterPortal() (PrePortalTransform).
	 * Main was never unloaded, so no reload is needed.
	 * Re-enables bIsSurvivalActive on the player automatically.
	 *
	 * Call this from the GameMode BP AFTER the loading screen is visible,
	 * triggered by OnPortalExitStarted.
	 */
	UFUNCTION(BlueprintCallable, Category = "AlphaExilemet|Streaming")
	void ExitPortal();

	/**
	 * Called by the challenge programmer (or their BP) to signal that the
	 * portal challenge is done.
	 *
	 * What this does in order:
	 *   1. Re-enables bIsSurvivalActive on the player.
	 *   2. Broadcasts OnPortalExitStarted so the GameMode BP can show the
	 *      loading screen.
	 *
	 * The GameMode BP is then responsible for:
	 *   - Playing the loading screen fade-in.
	 *   - Calling ExitPortal() after the screen is fully opaque.
	 *   - Fading out the loading screen when OnStreamComplete fires.
	 */
	UFUNCTION(BlueprintCallable, Category = "AlphaExilemet|Streaming")
	void CompletePortalChallenge();

private:
	// =========================================================================
	// STATE
	// =========================================================================

	/** Name of the portal level currently loaded; NAME_None when not in a portal. */
	FName ActivePortalName = NAME_None;

	/** Incremented per StreamLevel call so each latent action has a unique UUID. */
	int32 LatentUUID = 0;

	/**
	 * True from the moment HandleTutorialCompletion() fires until
	 * OnStreamLevelLoaded() has broadcast OnTutorialToMainComplete.
	 */
	bool bTutorialTransition = false;

	/** Latent callback — fires when the most-recent LoadStreamLevel finishes. */
	UFUNCTION()
	void OnStreamLevelLoaded();
};
