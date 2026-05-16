#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "AlphaStreamingSubsystem.generated.h"

// ─────────────────────────────────────────────────────────────────────────────
// DELEGATES
// ─────────────────────────────────────────────────────────────────────────────

/** Fires when the LOAD half of a StreamLevel call completes. */
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnStreamComplete);

/** Fires ONLY when the Tutorial→Main transition finishes loading. */
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnTutorialToMainComplete);

/**
 * Broadcast by GM at the END of SpawnNewGamePlayer.
 * WB_TutorialBlackout binds here and plays its fade-out.
 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnTutorialPlayerReady);

/**
 * Fires at the START of EnterPortal(), BEFORE streaming begins.
 * GM BP: bind → create WB_LoadingScreen immediately.
 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnPortalEnterStarted);

/**
 * Fires when CompletePortalChallenge() is called.
 * GM BP: bind → create WB_LoadingScreen, delay, call ExitPortal().
 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnPortalExitStarted);

// ─────────────────────────────────────────────────────────────────────────────

UCLASS()
class ALPHAEXILEMET_API UAlphaStreamingSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	// =========================================================================
	// DELEGATES
	// =========================================================================

	/** Broadcast when a level finishes LOADING (never for unloads). */
	UPROPERTY(BlueprintAssignable, Category = "AlphaExilemet|Streaming")
	FOnStreamComplete OnStreamComplete;

	/** GM binds here → calls SpawnNewGamePlayer when tutorial Main finishes loading. */
	UPROPERTY(BlueprintAssignable, Category = "AlphaExilemet|Streaming")
	FOnTutorialToMainComplete OnTutorialToMainComplete;

	/** GM fires this at end of SpawnNewGamePlayer. WB_TutorialBlackout binds → fades out. */
	UPROPERTY(BlueprintAssignable, BlueprintCallable, Category = "AlphaExilemet|Streaming")
	FOnTutorialPlayerReady OnTutorialPlayerReady;

	/** GM BP: bind → create loading screen BEFORE streaming starts. */
	UPROPERTY(BlueprintAssignable, Category = "AlphaExilemet|Streaming")
	FOnPortalEnterStarted OnPortalEnterStarted;

	/** GM BP: bind → create loading screen, delay, call ExitPortal(). */
	UPROPERTY(BlueprintAssignable, Category = "AlphaExilemet|Streaming")
	FOnPortalExitStarted OnPortalExitStarted;

	// =========================================================================
	// PUBLIC FUNCTIONS
	// =========================================================================

	/**
	 * Loads one level. Separately unloads another if LevelToUnload != NAME_None.
	 * The two operations use different callbacks so OnStreamComplete fires ONCE
	 * (only when the load finishes, not when the unload finishes).
	 */
	UFUNCTION(BlueprintCallable, Category = "AlphaExilemet|Streaming")
	void StreamLevel(FName LevelToLoad, FName LevelToUnload);

	/** Saves progress → loads Main → unloads Tutorial. */
	UFUNCTION(BlueprintCallable, Category = "AlphaExilemet|Streaming")
	void HandleTutorialCompletion();

	/**
	 * PORTAL ENTRY
	 *
	 * Sequence:
	 *  1. CurrentPhase = InPortal
	 *  2. OnPortalEnterStarted → GM shows loading screen immediately
	 *  3. SavePlayerData (health, tools, PrePortalTransform)
	 *  4. LoadStreamLevel(portal)  → OnStreamComplete fires when done
	 *  5. UnloadStreamLevel(Main)  → silent (no broadcast)
	 *
	 * Main IS unloaded so the levels never overlap visually.
	 */
	UFUNCTION(BlueprintCallable, Category = "AlphaExilemet|Streaming")
	void EnterPortal(FName PortalLevelName, FTransform PlayerReturnTransform);

	/**
	 * Called from GM BP InPortal switch case AFTER OnStreamComplete fires.
	 * Moves the existing pawn to the PlayerStart found inside the portal
	 * streaming level — no pawn destroy/re-spawn.
	 */
	UFUNCTION(BlueprintCallable, Category = "AlphaExilemet|Streaming")
	void TeleportPlayerToPortalStart();

	/**
	 * PORTAL EXIT
	 *
	 * Sequence:
	 *  1. Teleport player back to PrePortalTransform (yaw already baked by PortalBase)
	 *  2. Re-enable survival
	 *  3. CurrentPhase = Main
	 *  4. LoadStreamLevel(Main)    → OnStreamComplete fires when done → GM fades screen
	 *  5. UnloadStreamLevel(portal)→ silent
	 *
	 * Call from GM BP AFTER loading screen is fully opaque.
	 */
	UFUNCTION(BlueprintCallable, Category = "AlphaExilemet|Streaming")
	void ExitPortal();

	/**
	 * Called by the challenge programmer when the challenge is done.
	 * Re-enables survival + broadcasts OnPortalExitStarted.
	 * GM BP then shows the loading screen and calls ExitPortal().
	 */
	UFUNCTION(BlueprintCallable, Category = "AlphaExilemet|Streaming")
	void CompletePortalChallenge();

	/** DEBUG — press key to force portal exit. Remove before shipping. */
	UFUNCTION(BlueprintCallable, Category = "AlphaExilemet|Streaming|Debug")
	void Debug_ForceCompleteChallenge();

private:
	FName ActivePortalName    = NAME_None;
	int32 LoadLatentUUID      = 0;   // Used only for LOAD callbacks
	int32 UnloadLatentUUID    = 1000; // Used only for UNLOAD callbacks (different range)
	bool  bTutorialTransition = false;

	/**
	 * Fires when a LOAD completes. Broadcasts OnStreamComplete + tutorial delegate.
	 * Never fires for unloads — unloads use OnStreamLevelUnloaded (silent).
	 */
	UFUNCTION()
	void OnStreamLevelLoaded();

	/**
	 * Fires when an UNLOAD completes. Does nothing — exists only so the
	 * LatentActionInfo has a valid callback target and doesn't crash.
	 */
	UFUNCTION()
	void OnStreamLevelUnloaded();
};