#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "WakeUpDirector.generated.h"

class ULevelSequencePlayer;
class ALevelSequenceActor;
class AAlphaExilemetCharacter;
class APlayerController;

// ─────────────────────────────────────────────────────────────────────────────
// AWakeUpDirector  v5 — simplified to match TutorialDirector v19
//
// The animator handles the camera animation in Sequencer.
// C++ only needs to: play the sequence, return camera to player when done,
// restore input, enable survival.
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
	// INSTANCE REFERENCES
	// ═════════════════════════════════════════════════════════════════════════

	/** The LevelSequenceActor for the wake-up cutscene in the Main level. */
	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "WakeUp|Config",
		meta = (DisplayName = "Wake Up Sequence"))
	ALevelSequenceActor* WakeUpSequenceRef = nullptr;

	/**
	 * Optional: cinecam to snap to before the sequence plays.
	 * Leave null if the Camera Cuts track handles the first frame.
	 */
	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "WakeUp|Config",
		meta = (DisplayName = "Pre-Sequence Cinecam"))
	AActor* WakeUpCineCamRef = nullptr;
	
	/**
	 * The ship actor placed in the level with Hidden In Game = true.
	 * It will be revealed the moment the wake-up sequence ends.
	 */
	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "WakeUp|Config",
		meta = (DisplayName = "Persistent Ship Actor"))
	AActor* PersistentShipActor = nullptr;
	
	/** Reveals PersistentShipActor if assigned. */
	void RevealPersistentShip();

	// ═════════════════════════════════════════════════════════════════════════
	// CLASS DEFAULTS
	// ═════════════════════════════════════════════════════════════════════════

	/**
	 * Blend time to return the camera to the player after the sequence ends.
	 * 0.0 = instant snap.
	 * 0.3–0.8 = soft blend (use if there is a small gap between the sequence
	 *           camera's last position and the player's FP camera).
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "WakeUp|Config",
		meta = (DisplayName = "Camera Return Blend Time", ClampMin = "0.0"))
	float CameraReturnBlendTime = 0.3f;

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
	 * Called by GM_SimulatorGamemode after Tutorial→Main transition.
	 * GM BP: [WakeUpDirectorRef → Is Valid] → [InitializeWakeUp]
	 */
	UFUNCTION(BlueprintCallable, Category = "WakeUp")
	void InitializeWakeUp();

protected:
	// ═════════════════════════════════════════════════════════════════════════
	// BLUEPRINT IMPLEMENTABLE EVENTS
	// ═════════════════════════════════════════════════════════════════════════

	/** Push self-reference to GM (end of C++ BeginPlay). */
	UFUNCTION(BlueprintImplementableEvent, Category = "WakeUp|Events")
	void BP_RegisterWithGameMode();

	/**
	 * Called after camera returns to player and survival is enabled.
	 * Show HUD, play ambient audio, trigger ship terminal warning, etc.
	 */
	UFUNCTION(BlueprintImplementableEvent, Category = "WakeUp|Events")
	void BP_OnWakeUpComplete();

private:
	UFUNCTION()
	void OnWakeUpSequenceFinished();

	void OnWakeUpSequenceFinishedDeferred();
	void OnCameraReturnComplete();

	FTimerHandle CameraReturnHandle;
};