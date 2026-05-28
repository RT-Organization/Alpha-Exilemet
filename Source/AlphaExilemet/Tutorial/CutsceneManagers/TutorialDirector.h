#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TutorialDirector.generated.h"

class ULevelSequencePlayer;
class ALevelSequenceActor;
class AAlphaExilemetCharacter;
class AToolBase;
class ASkullProp;
class APlayerController;
class UCharacterMovementComponent;

// ─────────────────────────────────────────────────────────────────────────────
// ATutorialDirector  v19 — simplified
//
// The animator animates the camera in Sequencer including the move to the
// player's head. This class only needs to:
//   1. Hide the player during the cutscene.
//   2. When the sequence ends: reveal the ship, switch camera to player, give input.
//   3. Spawn the pickaxe.
//   4. Handle the OxygenSphere boundary guard.
//   5. Handle the skull sequence → Tutorial→Main transition.
// ─────────────────────────────────────────────────────────────────────────────

UCLASS(Abstract, Blueprintable)
class ALPHAEXILEMET_API ATutorialDirector : public AActor
{
	GENERATED_BODY()

public:
	ATutorialDirector();

protected:
	virtual void BeginPlay() override;

public:
	// ═════════════════════════════════════════════════════════════════════════
	// INSTANCE REFERENCES  (assign in placed actor Details panel)
	// ═════════════════════════════════════════════════════════════════════════

	/**
	 * The LevelSequenceActor for the intro cutscene (LS_Tutorial).
	 * Drag it from the Outliner into this slot.
	 */
	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Tutorial|Config",
		meta = (DisplayName = "Intro Sequence"))
	ALevelSequenceActor* IntroSequenceRef = nullptr;

	/**
	 * The ship actor placed in the level with Hidden In Game = true.
	 * It will be revealed the moment the intro sequence ends.
	 *
	 * How to set up:
	 *   1. Place your ship Static/Skeletal Mesh actor in the Tutorial level.
	 *   2. In its Details panel: Rendering → Hidden In Game = CHECKED.
	 *   3. Drag it into this slot on the placed BP_TutorialDirector.
	 */
	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Tutorial|Config",
		meta = (DisplayName = "Persistent Ship Actor"))
	AActor* PersistentShipActor = nullptr;

	/**
	 * Optional: cinecam to snap to before the sequence plays.
	 * Prevents a 1-frame first-person flash on the very first frame.
	 * Leave null if the Camera Cuts track handles the first frame.
	 */
	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Tutorial|Config",
		meta = (DisplayName = "Pre-Sequence Cinecam"))
	AActor* TutorialCineCamRef = nullptr;

	// ═════════════════════════════════════════════════════════════════════════
	// CLASS DEFAULTS  (set in BP_TutorialDirector Class Defaults)
	// ═════════════════════════════════════════════════════════════════════════

	/** BP_Pickaxe class spawned after the sequence ends. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Tutorial|Config")
	TSubclassOf<AToolBase> TutorialPickaxeClass;

	/**
	 * How long (seconds) to smoothly blend from the sequence's last CineCamera
	 * back to the player's first-person camera when the sequence ends.
	 *
	 * 0.0 = instant snap (use if animator already moved CineCamera to player head).
	 * 0.3–0.8 = soft blend (use if there is a small gap between CineCamera
	 *           and the player head position at the last frame).
	 *
	 * The animator should already end the camera at or very close to the player
	 * head in the sequence. This is just a safety smoothing value.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Tutorial|Config",
		meta = (DisplayName = "Camera Return Blend Time", ClampMin = "0.0"))
	float CameraReturnBlendTime = 0.3f;

	// ═════════════════════════════════════════════════════════════════════════
	// RUNTIME STATE
	// ═════════════════════════════════════════════════════════════════════════
	
	// Inside TutorialDirector.h -> RUNTIME STATE section
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Tutorial|Runtime")
	bool bIsTeleporting = false;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Tutorial|Runtime")
	ULevelSequencePlayer* IntroSequencePlayer = nullptr;

	/**
	 * Player world transform saved right after input is restored.
	 * Stores position + rotation so the OxygenSphere boundary teleport
	 * restores both (fixes the old "wrong facing direction" bug).
	 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Tutorial|Runtime")
	FTransform CraterStartTransform;

	/** Cached after InitializeTutorial(). Never cache in BeginPlay. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Tutorial|Runtime")
	AAlphaExilemetCharacter* CachedPlayer = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Tutorial|Runtime")
	APlayerController* CachedPC = nullptr;

	/** Set via BP BeginPlay: GetActorOfClass(ASkullProp) → SET SkullRef. */
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Tutorial|Skull")
	ASkullProp* SkullRef = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Tutorial|Skull")
	bool bSkullInteractionActive = false;

	// ═════════════════════════════════════════════════════════════════════════
	// PUBLIC INTERFACE
	// ═════════════════════════════════════════════════════════════════════════

	/** Called by GM after player is spawned and possessed. */
	UFUNCTION(BlueprintCallable, Category = "Tutorial")
	void InitializeTutorial();

	/** Called by ASkullProp::HandleInteract(). */
	UFUNCTION(BlueprintCallable, Category = "Tutorial|Skull")
	void OnSkullInteracted();

	/** Called by the streaming subsystem before Tutorial→Main swap. */
	UFUNCTION(BlueprintCallable, Category = "Tutorial")
	void ClearTutorialPickaxe();

protected:
	// ═════════════════════════════════════════════════════════════════════════
	// BLUEPRINT IMPLEMENTABLE EVENTS
	// ═════════════════════════════════════════════════════════════════════════

	/** Push self-reference to GM (called at end of C++ BeginPlay). */
	UFUNCTION(BlueprintImplementableEvent, Category = "Tutorial|Events")
	void BP_RegisterWithGameMode();

	/** Hide the player's main HUD widget. */
	UFUNCTION(BlueprintImplementableEvent, Category = "Tutorial|Events")
	void BP_HideHUD();

	/**
	 * Sequence finished, camera returned to player, input restored.
	 * Create and add WBP_TutorialOverlay to viewport here.
	 */
	UFUNCTION(BlueprintImplementableEvent, Category = "Tutorial|Events")
	void BP_OnIntroFinished();

	/** Play magic spell SFX. */
	UFUNCTION(BlueprintImplementableEvent, Category = "Tutorial|Skull")
	void BP_PlayMagicSpellSound();

	/** Create WBP_TutorialBlackout and add to viewport (ZOrder 99). */
	UFUNCTION(BlueprintImplementableEvent, Category = "Tutorial|Skull")
	void BP_ShowInstantBlack();

	/** Play explosion SFX. Guard with IsValid(ExplosionSpawnRef). */
	UFUNCTION(BlueprintImplementableEvent, Category = "Tutorial|Skull")
	void BP_PlayExplosionSequence();

private:
	// ── Cutscene ──────────────────────────────────────────────────────────────
	void HidePlayerForCutscene();

	UFUNCTION()
	void OnIntroSequenceFinished();

	// Deferred by one tick so Sequencer finishes its internal teardown first.
	void OnIntroSequenceFinishedDeferred();

	// ── Post-sequence ─────────────────────────────────────────────────────────
	/** Reveals PersistentShipActor if assigned. */
	void RevealPersistentShip();

	/**
	 * Blends PC view target back to the player camera.
	 * Uses CameraReturnBlendTime (0 = instant snap).
	 */
	void ReturnCameraToPlayer();

	/** Runs after CameraReturnBlendTime elapses — restores full player control. */
	void OnCameraReturnComplete();

	// ── Boundary guard ────────────────────────────────────────────────────────
	UFUNCTION()
	void OnOxygenSphereEndOverlap(
		UPrimitiveComponent* OverlappedComponent,
		AActor*              OtherActor,
		UPrimitiveComponent* OtherComp,
		int32                OtherBodyIndex);

	void ExecuteTeleportToCrater();
	void OnTeleportReadyToMove();
	void OnTeleportComplete();

	// ── Skull sequence ────────────────────────────────────────────────────────
	void OnSpellDurationComplete();
	void OnSkullPausedBeforeBlack();
	void OnPostBlackDelay();
	void OnLevelSwapReady();

	// ── Input ─────────────────────────────────────────────────────────────────
	void SuppressPlayerMoveInput();
	void RestorePlayerMoveInput();
	void LockPlayerInputFull();

	// ── Misc ──────────────────────────────────────────────────────────────────
	AActor* FindBaseCamp() const;

	// ── Timer handles ─────────────────────────────────────────────────────────
	FTimerHandle CameraReturnHandle;
	FTimerHandle TeleportFadeOutHandle;
	FTimerHandle TeleportFadeInHandle;
	FTimerHandle SpellDurationHandle;
	FTimerHandle SkullPauseHandle;
	FTimerHandle PostBlackSoundHandle;
	FTimerHandle LevelSwapHandle;

	UPROPERTY()
	AToolBase* SpawnedTutorialPickaxe = nullptr;
};