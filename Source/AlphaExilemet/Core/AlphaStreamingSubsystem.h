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
 * Fires after the GameMode has finished spawning and setting up the player
 * in the Main level following a Tutorial→Main transition.
 *
 * WHO FIRES IT:
 *   GM_SimulatorGamemode BP — at the very END of SpawnNewGamePlayer,
 *   after InitializePlayer, SetupPlayerGame, and survival flags are set.
 *
 * WHO BINDS TO IT:
 *   WB_TutorialBlackout — binds in Event Construct, removes itself when it fires.
 *   This is the ONLY thing the widget does with game state.
 *   The widget does NOT call anything on the GameMode.
 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnTutorialPlayerReady);

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
	 * Broadcast ONLY when the Tutorial → Main transition completes loading.
	 *
	 * WHO BINDS: GM_SimulatorGamemode — in its BeginPlay or InitializeInstance.
	 * When this fires, the GM calls SpawnNewGamePlayer.
	 * WB_TutorialBlackout does NOT bind here anymore.
	 */
	UPROPERTY(BlueprintAssignable, Category = "AlphaExilemet|Streaming")
	FOnTutorialToMainComplete OnTutorialToMainComplete;

	/**
	 * Broadcast by GM_SimulatorGamemode at the END of SpawnNewGamePlayer,
	 * after the player is fully initialized and the level is ready.
	 *
	 * WHO BINDS: WB_TutorialBlackout — binds in Event Construct.
	 * When this fires, the widget plays its fade-out and removes itself.
	 * This is the ONLY way the widget learns it is safe to disappear.
	 *
	 * HOW TO BROADCAST (in GM BP, last node of SpawnNewGamePlayer):
	 *   [Get Game Instance → Get Subsystem (Alpha Streaming Subsystem)]
	 *   → [On Tutorial Player Ready → Broadcast]
	 */
	UPROPERTY(BlueprintAssignable, BlueprintCallable, Category = "AlphaExilemet|Streaming")
	FOnTutorialPlayerReady OnTutorialPlayerReady;

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

	UFUNCTION(BlueprintCallable, Category = "AlphaExilemet|Streaming")
	void EnterPortal(FName PortalLevelName, FTransform PlayerEntryTransform);

	UFUNCTION(BlueprintCallable, Category = "AlphaExilemet|Streaming")
	void ExitPortal();

	UFUNCTION(BlueprintCallable, Category = "AlphaExilemet|Streaming")
	void CompletePortalChallenge();

private:
	FName ActivePortalName = NAME_None;
	int32 LatentUUID = 0;
	bool bTutorialTransition = false;

	UFUNCTION()
	void OnStreamLevelLoaded();
};