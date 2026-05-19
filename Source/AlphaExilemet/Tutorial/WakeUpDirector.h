#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "WakeUpDirector.generated.h"

class ULevelSequencePlayer;
class ALevelSequenceActor;
class AAlphaExilemetCharacter;
class APlayerController;

// ─────────────────────────────────────────────────────────────────────────────
// AWakeUpDirector  v2
//
// Changes from v1:
//   - Smooth CineCamera transition added.
//     The wake-up cutscene ends with a CineCamera at the player's head.
//     Instead of an instant camera hand-back we lerp the CineCamera
//     into the player head socket over WakeUpTransitionBlendTime seconds,
//     then execute the view target swap. Same system as ATutorialDirector.
//
// WORKFLOW:
//   1. Placed BP_WakeUpDirector in Main level.
//   2. Assign WakeUpSequenceRef in Details panel.
//   3. Assign SequenceEndCameraRef (the CineCamera the sequence ends on).
//   4. GM calls InitializeWakeUp() after SpawnNewGamePlayer (Tutorial→Main only).
//   5. After the smooth transition completes:
//        - Player gets camera + input back.
//        - Survival enabled.
//        - OnTutorialPlayerReady broadcast.
// ─────────────────────────────────────────────────────────────────────────────

UCLASS(Abstract, Blueprintable)
class ALPHAEXILEMET_API AWakeUpDirector : public AActor
{
	GENERATED_BODY()

public:
	AWakeUpDirector();

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;

public:
	// ── LEVEL REFERENCES ──────────────────────────────────────────────────────

	/** The LevelSequenceActor for the wake-up cutscene in the Main level. */
	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "WakeUp|Config",
		meta = (DisplayName = "Wake Up Sequence"))
	ALevelSequenceActor* WakeUpSequenceRef = nullptr;

	/**
	 * The CineCamera that the wake-up sequence ends on.
	 * The smooth transition moves this camera to the player's head socket.
	 * Must be the last active Camera Cut track actor in the sequence.
	 * Assign in the placed BP_WakeUpDirector Details panel.
	 */
	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "WakeUp|Config",
		meta = (DisplayName = "Sequence End CineCamera"))
	AActor* SequenceEndCameraRef = nullptr;

	/** Optional cinecam before sequence plays. Leave null if Camera Cut handles it. */
	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "WakeUp|Config",
		meta = (DisplayName = "Cinecam Actor (Pre-Play)"))
	AActor* WakeUpCineCamRef = nullptr;

	/**
	 * Time in seconds for the CineCamera to lerp into the player's head socket.
	 * 0.0 = instant camera hand-back (same as old behaviour).
	 * 0.3–0.6 = recommended for a smooth, invisible handoff.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "WakeUp|Config",
		meta = (DisplayName = "Transition Blend Time"))
	float WakeUpTransitionBlendTime = 0.5f;

	/**
	 * Distance threshold (cm) at which the CineCamera snaps to the head socket.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "WakeUp|Config",
		meta = (DisplayName = "Transition Snap Distance (cm)"))
	float TransitionSnapDistance = 3.0f;

	// ── RUNTIME STATE ─────────────────────────────────────────────────────────

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "WakeUp|Runtime")
	ULevelSequencePlayer* WakeUpSequencePlayer = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "WakeUp|Runtime")
	AAlphaExilemetCharacter* CachedPlayer = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "WakeUp|Runtime")
	APlayerController* CachedPC = nullptr;

	// ── PUBLIC INTERFACE ──────────────────────────────────────────────────────

	/**
	 * Called by GM_SimulatorGamemode inside SpawnNewGamePlayer,
	 * ONLY when transitioning from Tutorial (GamePhase == NewGame_Tutorial).
	 */
	UFUNCTION(BlueprintCallable, Category = "WakeUp")
	void InitializeWakeUp();

protected:
	// ── BLUEPRINT IMPLEMENTABLE EVENTS ────────────────────────────────────────

	/**
	 * Called at END of C++ BeginPlay.
	 * Push self-reference to GM: GM.WakeUpDirectorRef = Self.
	 */
	UFUNCTION(BlueprintImplementableEvent, Category = "WakeUp|Events")
	void BP_RegisterWithGameMode();

	/**
	 * Called when the wake-up transition fully completes (after smooth blend).
	 * Show HUD, play ambient sounds, trigger ship terminal warning, etc.
	 */
	UFUNCTION(BlueprintImplementableEvent, Category = "WakeUp|Events")
	void BP_OnWakeUpComplete();

private:
	// ── SMOOTH TRANSITION STATE ───────────────────────────────────────────────

	bool     bTransitionActive       = false;
	float    TransitionElapsed       = 0.0f;
	FVector  TransitionStartLocation = FVector::ZeroVector;
	FRotator TransitionStartRotation = FRotator::ZeroRotator;
	FVector  TransitionTargetLocation = FVector::ZeroVector;
	FRotator TransitionTargetRotation = FRotator::ZeroRotator;

	void TickSmoothTransition(float DeltaTime);

	/**
	 * Fires when the smooth transition reaches the head socket.
	 * Restores input, enables survival, broadcasts OnTutorialPlayerReady.
	 */
	void OnTransitionComplete();

	UFUNCTION()
	void OnWakeUpSequenceFinished();
};
