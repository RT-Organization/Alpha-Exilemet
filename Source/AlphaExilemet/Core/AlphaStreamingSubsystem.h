#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "LevelStreamingManager.h"
#include "AlphaStreamingSubsystem.generated.h"

// ─────────────────────────────────────────────────────────────────────────────
// LEGACY DELEGATES — kept so existing Blueprint bindings compile unchanged.
// The real logic is in ULevelStreamingManager.
// ─────────────────────────────────────────────────────────────────────────────
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnStreamComplete);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnTutorialToMainComplete);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnTutorialPlayerReady);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnPortalEnterStarted);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnPortalExitStarted);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnReturnToMainMenuStarted);

/**
 * UAlphaStreamingSubsystem — LEGACY SHIM
 *
 * All streaming logic has moved to ULevelStreamingManager.
 * This class is kept ONLY so that existing Blueprint nodes
 * (Bind Event to OnStreamComplete, etc.) do not break.
 *
 * HOW TO MIGRATE BLUEPRINTS:
 *   - Replace "Get Subsystem (AlphaStreamingSubsystem) → ReturnToMainMenu"
 *     with "Get Subsystem (LevelStreamingManager) → ReturnToMainMenu"
 *   - Replace "HandleTutorialCompletion" → "CompleteTutorialAndLoadMain"
 *   - Replace "EnterPortal" → "EnterPortal" (same name, EGameLevel param now)
 *   - Bind to LevelStreamingManager.OnLevelTransitionComplete instead of
 *     OnStreamComplete / OnTutorialToMainComplete.
 *   - OnTutorialPlayerReady stays on this class (WakeUpDirector uses it).
 *
 * Do NOT add new logic here. Add it to LevelStreamingManager.
 */
UCLASS()
class ALPHAEXILEMET_API UAlphaStreamingSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	// ── LEGACY DELEGATES (kept for BP backward compat) ─────────────────────

	/** @deprecated Bind to LevelStreamingManager.OnLevelTransitionComplete instead. */
	UPROPERTY(BlueprintAssignable, Category = "Legacy|Streaming")
	FOnStreamComplete OnStreamComplete;

	/** @deprecated Bind to LevelStreamingManager.OnLevelTransitionComplete instead. */
	UPROPERTY(BlueprintAssignable, Category = "Legacy|Streaming")
	FOnTutorialToMainComplete OnTutorialToMainComplete;

	/**
	 * Still used by WakeUpDirector and WB_TutorialBlackout.
	 * GM calls this at end of SpawnNewGamePlayer after InitializeWakeUp.
	 * NOT deprecated — keep this.
	 */
	UPROPERTY(BlueprintAssignable, BlueprintCallable, Category = "AlphaExilemet|Streaming")
	FOnTutorialPlayerReady OnTutorialPlayerReady;

	/** @deprecated Use LevelStreamingManager.OnLevelTransitionStarted instead. */
	UPROPERTY(BlueprintAssignable, Category = "Legacy|Streaming")
	FOnPortalEnterStarted OnPortalEnterStarted;

	/** @deprecated Use LevelStreamingManager.OnPortalExitStarted instead. */
	UPROPERTY(BlueprintAssignable, Category = "Legacy|Streaming")
	FOnPortalExitStarted OnPortalExitStarted;

	/** @deprecated Use LevelStreamingManager.OnLevelTransitionStarted instead. */
	UPROPERTY(BlueprintAssignable, Category = "Legacy|Streaming")
	FOnReturnToMainMenuStarted OnReturnToMainMenuStarted;

	// ── FORWARDING FUNCTIONS — forward to LevelStreamingManager ───────────

	/** @deprecated Use LevelStreamingManager::ReturnToMainMenu() */
	UFUNCTION(BlueprintCallable, Category = "Legacy|Streaming",
		meta = (DeprecatedFunction, DeprecationMessage = "Use LevelStreamingManager::ReturnToMainMenu()"))
	void ReturnToMainMenu();

	/** @deprecated Use LevelStreamingManager::CompleteTutorialAndLoadMain() */
	UFUNCTION(BlueprintCallable, Category = "Legacy|Streaming",
		meta = (DeprecatedFunction, DeprecationMessage = "Use LevelStreamingManager::CompleteTutorialAndLoadMain()"))
	void HandleTutorialCompletion();

	/** @deprecated Use LevelStreamingManager::CompletePortalChallenge() */
	UFUNCTION(BlueprintCallable, Category = "Legacy|Streaming",
		meta = (DeprecatedFunction, DeprecationMessage = "Use LevelStreamingManager::CompletePortalChallenge()"))
	void CompletePortalChallenge();

	/** @deprecated Use LevelStreamingManager::Debug_ForceExitPortal() */
	UFUNCTION(BlueprintCallable, Category = "Legacy|Streaming|Debug")
	void Debug_ForceCompleteChallenge();

private:
	ULevelStreamingManager* GetManager() const;
};
