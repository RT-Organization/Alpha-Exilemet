#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "WakeUpDirector.generated.h"

class ULevelSequencePlayer;
class ALevelSequenceActor;
class AAlphaExilemetCharacter;
class APlayerController;
class ACameraActor;

// ─────────────────────────────────────────────────────────────────────────────
// AWakeUpDirector  v3
//
// Changes from v2:
//   - SequenceEndCameraRef REMOVED. No level setup needed.
//   - Reads camera transform from PlayerCameraManager at sequence end.
//   - Spawns TempTransitionCamera there, lerps it to FirstPersonCameraComponent,
//     then destroys it and hands control back to the player.
//   - Identical system to ATutorialDirector's smooth transition.
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

	/** The LevelSequenceActor for the wake-up cutscene (LS_WakeUp). */
	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "WakeUp|Config",
		meta = (DisplayName = "Wake Up Sequence"))
	ALevelSequenceActor* WakeUpSequenceRef = nullptr;

	/** Optional: cinecam for the view before the sequence starts.
	 *  Leave null if the Camera Cut track handles the initial view. */
	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "WakeUp|Config",
		meta = (DisplayName = "Cinecam Actor (Pre-Play)"))
	AActor* WakeUpCineCamRef = nullptr;

	/**
	 * How long (seconds) the temp camera takes to slide into the player's
	 * FirstPersonCameraComponent after the wake-up sequence ends.
	 * 0.0 = instant. 0.4–0.7 recommended.
	 * NO LEVEL SETUP NEEDED — reads from PlayerCameraManager automatically.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "WakeUp|Config",
		meta = (DisplayName = "Transition Blend Time"))
	float WakeUpTransitionBlendTime = 0.5f;

	/** Distance (cm) at which the temp camera snaps and the swap fires. */
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

	/** Called by GM_SimulatorGamemode inside SpawnNewGamePlayer,
	 *  only when Phase == Main (Tutorial→Main transition). */
	UFUNCTION(BlueprintCallable, Category = "WakeUp")
	void InitializeWakeUp();

protected:
	// ── BLUEPRINT IMPLEMENTABLE EVENTS ────────────────────────────────────────

	/** Push self-reference to GM: GM.WakeUpDirectorRef = Self. */
	UFUNCTION(BlueprintImplementableEvent, Category = "WakeUp|Events")
	void BP_RegisterWithGameMode();

	/** Called when transition fully completes. Show HUD, play ambient audio, etc. */
	UFUNCTION(BlueprintImplementableEvent, Category = "WakeUp|Events")
	void BP_OnWakeUpComplete();

private:
	// ── SMOOTH TRANSITION STATE ───────────────────────────────────────────────

	UPROPERTY()
	ACameraActor* TempTransitionCamera = nullptr;

	bool     bTransitionActive       = false;
	float    TransitionElapsed       = 0.0f;
	FVector  TransitionStartLocation = FVector::ZeroVector;
	FRotator TransitionStartRotation = FRotator::ZeroRotator;

	void TickSmoothTransition(float DeltaTime);
	void OnTransitionComplete();

	UFUNCTION() void OnWakeUpSequenceFinished();
};
