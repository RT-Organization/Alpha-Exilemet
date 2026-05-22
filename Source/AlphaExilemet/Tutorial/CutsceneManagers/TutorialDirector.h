#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TutorialDirector.generated.h"

// ─────────────────────────────────────────────────────────────────────────────
// FORWARD DECLARATIONS
// ─────────────────────────────────────────────────────────────────────────────
class ULevelSequencePlayer;
class ALevelSequenceActor;
class AAlphaExilemetCharacter;
class AToolBase;
class ASkullProp;
class APlayerController;
class UCharacterMovementComponent;
class USkeletalMeshComponent;
class UCinematicHandoffComponent;
class APlayerSpawnMarker;
class ACameraActor;

// ─────────────────────────────────────────────────────────────────────────────
// ATutorialDirector  v16
//
// Changes from v15:
//   - CinematicHandoffComponent added.
//     Replaces the old proxy-swap logic with a proper ghost-camera blend.
//     The animator NO LONGER needs to position the CineCamera at the exact
//     player head position — the component handles the blend automatically.
//
//   - LastTutorialCineCamera property added.
//     Assign this to the CineCamera that is active on the sequence's LAST FRAME.
//     The handoff component spawns a ghost camera at that exact world transform.
//
//   - TutorialPlayerSpawnMarker property added.
//     A BP_PlayerSpawnMarker placed in the level at the SK's foot position on
//     the last frame. Solves the "wrong SK pivot" problem completely.
//
//   - CraterStartPosition (FVector) replaced by CraterStartTransform (FTransform).
//     Fixes the bug where the player was teleported back to the crater but
//     faced the wrong direction because only position was saved.
//
//   - OnCinematicHandoffComplete() added as private callback.
//     Everything that previously happened after ExecuteProxySwap() now runs
//     here, after the camera blend finishes.
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
	// COMPONENTS
	// ═════════════════════════════════════════════════════════════════════════

	/**
	 * Handles the ghost-camera blend from the sequence's last CineCamera
	 * position to the BP_Player's FirstPersonCamera after the intro ends.
	 *
	 * Configure in BP_TutorialDirector Class Defaults → Cinematic Handoff:
	 *   CameraBlendTime  — seconds to blend (0.4–0.8 recommended)
	 *   BlendFunction    — VTBlend_EaseInOut for most transitions
	 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Tutorial|Components")
	UCinematicHandoffComponent* CinematicHandoff;

	// ═════════════════════════════════════════════════════════════════════════
	// LEVEL REFERENCES — assign in the placed INSTANCE Details panel
	// ═════════════════════════════════════════════════════════════════════════

	/**
	 * The LevelSequenceActor for the intro cutscene.
	 * Assign: placed BP_TutorialDirector → Details → Tutorial|Config.
	 */
	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Tutorial|Config",
		meta = (DisplayName = "Intro Sequence"))
	ALevelSequenceActor* IntroSequenceRef = nullptr;

	/**
	 * The CineCameraActor that is ACTIVE ON THE LAST FRAME of the intro sequence.
	 *
	 * HOW TO FIND IT:
	 *   1. Open LS_Tutorial in Sequencer.
	 *   2. Scrub to the very last frame.
	 *   3. Look at the Camera Cuts track — whichever camera is highlighted is the one.
	 *   4. Drag it from the Outliner into this slot.
	 *
	 * The handoff component spawns a ghost camera at this actor's EXACT world
	 * transform, so the view is frozen seamlessly when the sequence ends.
	 */
	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Tutorial|Config",
		meta = (DisplayName = "Last Sequence CineCamera"))
	ACameraActor* LastTutorialCineCamera = nullptr;

	/**
	 * An APlayerSpawnMarker placed at the SK proxy's FEET on the last frame.
	 *
	 * WHY: The animator's Spawnable SK often has an incorrect pivot point, so
	 * we cannot derive the player's root position from the SK's actor location.
	 * The marker is placed manually at the correct foot position.
	 *
	 * HOW TO PLACE:
	 *   1. Scrub LS_Tutorial to the last frame in Sequencer.
	 *   2. In the viewport, identify where the SK character's feet touch the ground.
	 *   3. Place BP_PlayerSpawnMarker at that position, facing the look direction.
	 *   4. Drag it into this slot.
	 *
	 * If null, falls back to the CutsceneProxy actor's transform (old behavior).
	 */
	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Tutorial|Config",
		meta = (DisplayName = "Tutorial Player Spawn Marker"))
	APlayerSpawnMarker* TutorialPlayerSpawnMarker = nullptr;

	/**
	 * Optional cinecam actor to snap to BEFORE the sequence plays, to avoid
	 * a 1-frame first-person flash. Leave null if the Camera Cut track handles it.
	 */
	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Tutorial|Config",
		meta = (DisplayName = "Cinecam Actor (Pre-Sequence)"))
	AActor* TutorialCineCamRef = nullptr;

	/** BP_Pickaxe class to spawn when the handoff finishes. Set in Class Defaults. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Tutorial|Config")
	TSubclassOf<AToolBase> TutorialPickaxeClass;

	/**
	 * Actor tag used to find the animator's Spawnable proxy SK in the sequence.
	 * Only used as a FALLBACK if TutorialPlayerSpawnMarker is null.
	 * Default: "CutsceneProxy".
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Tutorial|Config",
		meta = (DisplayName = "Proxy Character Tag (Fallback)"))
	FName ProxyCharacterTag = FName("CutsceneProxy");

	// ═════════════════════════════════════════════════════════════════════════
	// RUNTIME STATE
	// ═════════════════════════════════════════════════════════════════════════

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Tutorial|Runtime")
	ULevelSequencePlayer* IntroSequencePlayer = nullptr;

	/**
	 * Player's world transform at the moment gameplay begins after the intro.
	 * Saved AFTER the handoff blend completes so it includes position + rotation.
	 *
	 * Used by the OxygenSphere boundary guard: if the player exits the safe zone
	 * they are teleported back HERE (position AND facing direction restored).
	 *
	 * Replaces the old CraterStartPosition (FVector) — fixes the bug where the
	 * player was teleported back facing the wrong direction.
	 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Tutorial|Runtime")
	FTransform CraterStartTransform;

	/** Non-null after InitializeTutorial(). Never cache in BeginPlay. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Tutorial|Runtime")
	AAlphaExilemetCharacter* CachedPlayer = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Tutorial|Runtime")
	APlayerController* CachedPC = nullptr;

	/** Set in BP BeginPlay via GetActorOfClass(ASkullProp) → SET SkullRef. */
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Tutorial|Skull")
	ASkullProp* SkullRef = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Tutorial|Skull")
	bool bSkullInteractionActive = false;

	// ═════════════════════════════════════════════════════════════════════════
	// PUBLIC INTERFACE
	// ═════════════════════════════════════════════════════════════════════════

	/** Called by GM after player is spawned + possessed. */
	UFUNCTION(BlueprintCallable, Category = "Tutorial")
	void InitializeTutorial();

	/**
	 * Called by ASkullProp::HandleInteract().
	 * Timing: 0.0s flicker + SFX → 5.5s skull stops → 6.0s input lock + black
	 * → 7.0s explosion SFX → 10.0s Tutorial→Main swap.
	 */
	UFUNCTION(BlueprintCallable, Category = "Tutorial|Skull")
	void OnSkullInteracted();

	UFUNCTION(BlueprintCallable, Category = "Tutorial")
	void ClearTutorialPickaxe();

protected:
	// ═════════════════════════════════════════════════════════════════════════
	// BLUEPRINT IMPLEMENTABLE EVENTS
	// ═════════════════════════════════════════════════════════════════════════

	/** Called at END of C++ BeginPlay. Push self-reference to GM. */
	UFUNCTION(BlueprintImplementableEvent, Category = "Tutorial|Events")
	void BP_RegisterWithGameMode();

	/** Hide player's main HUD. */
	UFUNCTION(BlueprintImplementableEvent, Category = "Tutorial|Events")
	void BP_HideHUD();

	/** Handoff blend finished. Create + Add WBP_TutorialOverlay to viewport. */
	UFUNCTION(BlueprintImplementableEvent, Category = "Tutorial|Events")
	void BP_OnIntroFinished();

	/** Play spell SFX (Play Sound 2D). */
	UFUNCTION(BlueprintImplementableEvent, Category = "Tutorial|Skull")
	void BP_PlayMagicSpellSound();

	/**
	 * Create WBP_TutorialBlackout → Add to Viewport (ZOrder 99).
	 * Two nodes: Create widget → Add to Viewport.
	 */
	UFUNCTION(BlueprintImplementableEvent, Category = "Tutorial|Skull")
	void BP_ShowInstantBlack();

	/** Play explosion SFX at ship location. Guard with IsValid(ExplosionSpawnRef). */
	UFUNCTION(BlueprintImplementableEvent, Category = "Tutorial|Skull")
	void BP_PlayExplosionSequence();

private:
	// ═════════════════════════════════════════════════════════════════════════
	// INTERNAL HELPERS
	// ═════════════════════════════════════════════════════════════════════════

	// -- Cutscene setup --
	void HidePlayerForCutscene();

	// -- Cutscene end --
	UFUNCTION()
	void OnIntroSequenceFinished();

	/**
	 * Called by CinematicHandoff::OnHandoffComplete after the camera blend ends.
	 * This is where everything that used to follow ExecuteProxySwap() now lives:
	 * restore input, save crater transform, wire boundary, spawn pickaxe, show UI.
	 */
	UFUNCTION()
	void OnCinematicHandoffComplete();

	// -- Fallback proxy handling (used only when TutorialPlayerSpawnMarker is null) --
	AActor* FindCutsceneProxy() const;

	// -- Boundary guard --
	UFUNCTION()
	void OnOxygenSphereEndOverlap(
		UPrimitiveComponent* OverlappedComp,
		AActor*              OtherActor,
		UPrimitiveComponent* OtherComp,
		int32                OtherBodyIndex);

	// -- Teleport sequence (boundary violation) --
	void ExecuteTeleportToCrater();
	void OnTeleportReadyToMove();
	void OnTeleportComplete();

	// -- Skull sequence --
	void OnSpellDurationComplete();
	void OnSkullPausedBeforeBlack();
	void OnPostBlackDelay();
	void OnLevelSwapReady();

	// -- Input --
	void SuppressPlayerMoveInput();
	void RestorePlayerMoveInput();
	void LockPlayerInputFull();

	// -- Misc --
	AAlphaExilemetCharacter* GetTutorialPlayer() const;
	AActor*                  FindBaseCamp() const;

	// -- Timer handles --
	FTimerHandle TeleportFadeOutHandle;
	FTimerHandle TeleportFadeInHandle;
	FTimerHandle SpellDurationHandle;
	FTimerHandle SkullPauseHandle;
	FTimerHandle PostBlackSoundHandle;
	FTimerHandle LevelSwapHandle;

	UPROPERTY()
	AToolBase* SpawnedTutorialPickaxe = nullptr;
};
