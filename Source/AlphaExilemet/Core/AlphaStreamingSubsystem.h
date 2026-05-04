#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "AlphaStreamingSubsystem.generated.h"

// ─────────────────────────────────────────────────────────────────────────────
// DELEGATES
// Declared at file scope so Blueprints can bind to them as Event Dispatchers.
// ─────────────────────────────────────────────────────────────────────────────

/** Fires on EVERY level-load completion (portal in, portal out, tutorial→main). */
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnStreamComplete);

/** Fires ONLY when the Tutorial→Main transition finishes loading. */
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnTutorialToMainComplete);

// ─────────────────────────────────────────────────────────────────────────────

UCLASS()
class ALPHAEXILEMET_API UAlphaStreamingSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	// ─────────────────────────────────────────────────────────────────────────
	// DELEGATES (public assignable — Blueprints bind here)
	// ─────────────────────────────────────────────────────────────────────────

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

	// ─────────────────────────────────────────────────────────────────────────
	// PUBLIC FUNCTIONS
	// ─────────────────────────────────────────────────────────────────────────

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
	 * Saves PrePortalTransform and sets ActivePortalName for ExitPortal().
	 */
	UFUNCTION(BlueprintCallable, Category = "AlphaExilemet|Streaming")
	void EnterPortal(FName PortalLevelName, FTransform PlayerEntryTransform);

	/**
	 * Unloads the active portal level. Main was never unloaded, so no reload needed.
	 * Restores the player to PrePortalTransform.
	 */
	UFUNCTION(BlueprintCallable, Category = "AlphaExilemet|Streaming")
	void ExitPortal();

private:
	// ─────────────────────────────────────────────────────────────────────────
	// STATE
	// ─────────────────────────────────────────────────────────────────────────

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
