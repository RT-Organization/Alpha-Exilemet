#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "WakeUpDirector.generated.h"

class ULevelSequencePlayer;
class ALevelSequenceActor;
class AAlphaExilemetCharacter;
class APlayerController;
class UCinematicHandoffComponent;
class APlayerSpawnMarker;
class ACameraActor;

// ─────────────────────────────────────────────────────────────────────────────
// AWakeUpDirector  v2
//
// Changes from v1:
//   - CinematicHandoffComponent added (same component as TutorialDirector).
//     The old instant camera handoff in OnWakeUpSequenceFinished() is replaced
//     by the ghost-camera blend system for a seamless transition.
//
//   - LastWakeUpCineCamera property added.
//     Assign the CineCamera active on the wake-up sequence's last frame.
//
//   - WakeUpPlayerSpawnMarker property added.
//     A BP_PlayerSpawnMarker placed at the SK's feet on the last frame of the
//     wake-up cutscene. Solves the same SK pivot-offset problem as Tutorial.
//
//   - OnWakeUpHandoffComplete() added as the callback that fires when the blend
//     finishes. Survival, input, and HUD are enabled from there.
// ─────────────────────────────────────────────────────────────────────────────

UCLASS(Abstract, Blueprintable)
class ALPHAEXILEMET_API AWakeUpDirector : public AActor
{
	GENERATED_BODY()

public:
	AWakeUpDirector();

protected:
	virtual void BeginPlay() override;

public:
	// ═════════════════════════════════════════════════════════════════════════
	// COMPONENTS
	// ═════════════════════════════════════════════════════════════════════════

	/**
	 * Handles the ghost-camera blend from the wake-up sequence's last CineCamera
	 * position to the BP_Player's FirstPersonCamera.
	 *
	 * Configure in BP_WakeUpDirector Class Defaults → Cinematic Handoff:
	 *   CameraBlendTime  — seconds to blend (0.4–0.8 recommended)
	 *   BlendFunction    — VTBlend_EaseInOut for most transitions
	 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "WakeUp|Components")
	UCinematicHandoffComponent* CinematicHandoff;

	// ═════════════════════════════════════════════════════════════════════════
	// LEVEL REFERENCES — assign in the placed INSTANCE Details panel
	// ═════════════════════════════════════════════════════════════════════════

	/**
	 * The LevelSequenceActor for the wake-up cutscene in the Main level.
	 * Assign once: select placed BP_WakeUpDirector → Details → WakeUp|Config.
	 */
	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "WakeUp|Config",
		meta = (DisplayName = "Wake Up Sequence"))
	ALevelSequenceActor* WakeUpSequenceRef = nullptr;

	/**
	 * The CineCameraActor active on the LAST FRAME of the wake-up sequence.
	 *
	 * HOW TO FIND IT:
	 *   1. Open the wake-up LevelSequence in Sequencer.
	 *   2. Scrub to the last frame.
	 *   3. The camera highlighted in the Camera Cuts track is the one.
	 *   4. Drag it here from the Outliner.
	 *
	 * The handoff component spawns a ghost camera at this actor's exact world
	 * transform so the view is frozen seamlessly when the sequence ends.
	 */
	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "WakeUp|Config",
		meta = (DisplayName = "Last Sequence CineCamera"))
	ACameraActor* LastWakeUpCineCamera = nullptr;

	/**
	 * A BP_PlayerSpawnMarker placed at the SK proxy's FEET on the last frame
	 * of the wake-up cutscene.
	 *
	 * Same placement workflow as TutorialDirector:
	 *   1. Scrub the wake-up sequence to the last frame.
	 *   2. Place the marker at the SK's foot contact point on the ground.
	 *   3. Rotate the forward arrow toward where the player should look.
	 *   4. Drag it here.
	 *
	 * If null, falls back to the current player transform (will look wrong).
	 */
	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "WakeUp|Config",
		meta = (DisplayName = "Wake Up Player Spawn Marker"))
	APlayerSpawnMarker* WakeUpPlayerSpawnMarker = nullptr;

	/**
	 * Optional cinecam to snap to BEFORE the sequence plays, avoiding a
	 * 1-frame FP flash. Leave null if the Camera Cut track handles it.
	 */
	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "WakeUp|Config",
		meta = (DisplayName = "Cinecam Actor (Pre-Sequence)"))
	AActor* WakeUpCineCamRef = nullptr;

	// ═════════════════════════════════════════════════════════════════════════
	// RUNTIME STATE
	// ═════════════════════════════════════════════════════════════════════════

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "WakeUp|Runtime")
	ULevelSequencePlayer* WakeUpSequencePlayer = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "WakeUp|Runtime")
	AAlphaExilemetCharacter* CachedPlayer = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "WakeUp|Runtime")
	APlayerController* CachedPC = nullptr;

	// ═════════════════════════════════════════════════════════════════════════
	// PUBLIC INTERFACE
	// ═════════════════════════════════════════════════════════════════════════

	/**
	 * Called by GM_SimulatorGamemode inside SpawnNewGamePlayer,
	 * ONLY when transitioning from Tutorial (CurrentLevel == Tutorial).
	 *
	 * GM BP check:
	 *   [WakeUpDirectorRef → Is Valid]
	 *     True  → [InitializeWakeUp (Target = WakeUpDirectorRef)]
	 *     False → skip (loaded game)
	 */
	UFUNCTION(BlueprintCallable, Category = "WakeUp")
	void InitializeWakeUp();

protected:
	// ═════════════════════════════════════════════════════════════════════════
	// BLUEPRINT IMPLEMENTABLE EVENTS
	// ═════════════════════════════════════════════════════════════════════════

	/**
	 * Called at END of C++ BeginPlay. Push self-reference to GM.
	 *
	 *   [Event BP_RegisterWithGameMode]
	 *     → [Get Game Mode → Cast To GM_SimulatorGamemode]
	 *     → [SET WakeUpDirectorRef (Value = Self)]
	 */
	UFUNCTION(BlueprintImplementableEvent, Category = "WakeUp|Events")
	void BP_RegisterWithGameMode();

	/**
	 * Called after the camera handoff blend finishes — player has full control.
	 * Use to: show HUD, remove WB_TutorialBlackout, play ambient audio, etc.
	 * Survival is already enabled by the time this fires.
	 */
	UFUNCTION(BlueprintImplementableEvent, Category = "WakeUp|Events")
	void BP_OnWakeUpComplete();

private:
	UFUNCTION()
	void OnWakeUpSequenceFinished();

	/**
	 * Fires when the CinematicHandoff blend completes.
	 * Enables survival, restores input, signals the blackout widget, calls BP_OnWakeUpComplete.
	 */
	UFUNCTION()
	void OnWakeUpHandoffComplete();
};
