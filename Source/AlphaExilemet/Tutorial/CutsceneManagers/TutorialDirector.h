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
class USkeletalMeshComponent;
class UCinematicHandoffComponent;
class ACameraActor;

// ─────────────────────────────────────────────────────────────────────────────
// ATutorialDirector  v17
//
// Changes from v16:
//   - PlayerSpawnMarker REMOVED. No manual marker placement needed.
//   - Bone-based player positioning:
//       * SK_Manny is found by ProxySkeletonTag after sequence stops.
//       * GetBoneLocation(HeadBoneName) → camera travel destination.
//       * GetBoneLocation(RootBoneName) → player spawn position.
//       Both are independent of the SK's (wrong) actor pivot.
//   - PersistentShipActor + CutsceneShipTag added.
//       The SHIP_1 Spawnable's transform is copied to PersistentShipActor
//       the frame the sequence ends, then PersistentShipActor is shown.
//       This seamlessly replaces the disappearing Spawnable ship.
//   - CinematicHandoffComponent now travels the ghost camera FROM the last
//       CineCamera position TOWARD the head bone (not toward the player's
//       FP cam position). At arrival the player is placed and control is returned.
//   - Auto-detection of last CineCamera added:
//       If LastTutorialCineCamera is null, the component tries PC->GetViewTarget()
//       at the moment OnStop fires.
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
	 * Handles the ghost-camera travel from the last CineCamera toward the
	 * SK_Manny head bone, and the subsequent player swap.
	 *
	 * Configure in BP_TutorialDirector Class Defaults → Cinematic Handoff:
	 *   Camera Blend Time    – seconds to travel (0.8–1.5 recommended)
	 *   Final Snap Blend Time – 0 = instant snap at destination (preferred)
	 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Tutorial|Components")
	UCinematicHandoffComponent* CinematicHandoff;

	// ═════════════════════════════════════════════════════════════════════════
	// INSTANCE REFERENCES  (assign in placed Details panel — one-time setup)
	// ═════════════════════════════════════════════════════════════════════════

	/** The LevelSequenceActor for the Tutorial intro cutscene. */
	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Tutorial|Config",
		meta = (DisplayName = "Intro Sequence"))
	ALevelSequenceActor* IntroSequenceRef = nullptr;

	/**
	 * The CineCameraActor active on LS_Tutorial's LAST FRAME.
	 *
	 * How to find it:
	 *   Open LS_Tutorial → Sequencer. Scrub to the last frame.
	 *   The camera highlighted in the Camera Cuts track is this one.
	 *   Drag it from the Outliner into this slot.
	 *
	 * If left null, the component tries to auto-detect via PC->GetViewTarget()
	 * at the time OnStop fires (reliable only if the sequence hasn't released
	 * the camera yet — assign manually for guaranteed results).
	 */
	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Tutorial|Config",
		meta = (DisplayName = "Last Sequence CineCamera"))
	ACameraActor* LastTutorialCineCamera = nullptr;

	/**
	 * Level-placed persistent ship actor (hidden by default, shown when the
	 * sequence ends to replace the disappearing SHIP_1 Spawnable).
	 *
	 * Setup:
	 *   1. Duplicate or re-create the ship in the level at the same position
	 *      as SHIP_1 inside the sequence — OR leave it anywhere and the script
	 *      will copy the Spawnable's transform automatically at sequence end
	 *      (requires CutsceneShipTag to be set on the Spawnable).
	 *   2. Set the actor Hidden In Game in the Details panel (ticked by default).
	 *   3. Drag it into this slot.
	 */
	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Tutorial|Config",
		meta = (DisplayName = "Persistent Ship Actor"))
	AActor* PersistentShipActor = nullptr;

	/**
	 * Optional cinecam to snap to BEFORE the sequence plays, preventing a
	 * 1-frame FP-camera flash. Leave null if the Camera Cuts track handles it.
	 */
	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Tutorial|Config",
		meta = (DisplayName = "Pre-Sequence Cinecam"))
	AActor* TutorialCineCamRef = nullptr;

	// ═════════════════════════════════════════════════════════════════════════
	// CLASS DEFAULTS  (set once in BP_TutorialDirector Class Defaults)
	// ═════════════════════════════════════════════════════════════════════════

	/** BP_Pickaxe class to spawn when the handoff finishes. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Tutorial|Config")
	TSubclassOf<AToolBase> TutorialPickaxeClass;

	/**
	 * Actor tag on the SKM_Manny Spawnable in the Tutorial sequence.
	 *
	 * REQUIRED SETUP (one-time, done in Sequencer):
	 *   1. In LS_Tutorial, select the SKM_Manny track header.
	 *   2. In the Details panel, expand "Actor Tags" and add: "SKM_Manny"
	 *      (or whatever value you set here).
	 *   3. Also set "When Finished" to "Keep State" on that track.
	 *      This keeps the actor alive when OnStop fires so we can query bones.
	 *
	 * Default: "SKM_Manny"
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Tutorial|Config|Tags",
		meta = (DisplayName = "Proxy Skeleton Tag"))
	FName ProxySkeletonTag = FName("SKM_Manny");

	/**
	 * Actor tag on the SHIP_1 Spawnable in the Tutorial sequence.
	 * Used to copy its world transform to PersistentShipActor at sequence end.
	 *
	 * If no tag is found the PersistentShipActor is shown at its current
	 * level position (you must place it manually in that case).
	 *
	 * Default: "CutsceneShip"
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Tutorial|Config|Tags",
		meta = (DisplayName = "Cutscene Ship Tag"))
	FName CutsceneShipTag = FName("CutsceneShip");

	/**
	 * Name of the head bone in the proxy SK.
	 * The ghost camera travels to this bone's world position.
	 * Standard Manny skeleton: "head"
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Tutorial|Config|Bones",
		meta = (DisplayName = "Head Bone Name"))
	FName HeadBoneName = FName("head");

	/**
	 * Name of the root/feet bone in the proxy SK.
	 * The player pawn is placed at this bone's world position.
	 * Standard Manny skeleton: "root"  (sits at floor level)
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Tutorial|Config|Bones",
		meta = (DisplayName = "Root Bone Name"))
	FName RootBoneName = FName("root");

	// ═════════════════════════════════════════════════════════════════════════
	// RUNTIME STATE
	// ═════════════════════════════════════════════════════════════════════════

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Tutorial|Runtime")
	ULevelSequencePlayer* IntroSequencePlayer = nullptr;

	/**
	 * Player's world transform at the start of gameplay (after handoff).
	 * Stored as FTransform so BOTH position AND facing direction are saved.
	 * The OxygenSphere boundary guard teleports the player here (with rotation)
	 * if they walk too far — fixes the old "wrong facing" bug.
	 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Tutorial|Runtime")
	FTransform CraterStartTransform;

	/** Non-null after InitializeTutorial(). Never cache in BeginPlay. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Tutorial|Runtime")
	AAlphaExilemetCharacter* CachedPlayer = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Tutorial|Runtime")
	APlayerController* CachedPC = nullptr;

	/** Set in BP BeginPlay via GetActorOfClass(ASkullProp). */
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
	 * 0.0s flicker + SFX → 5.5s skull stops → 6.0s input lock + black
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

	/** Push self-reference to GM (called at end of C++ BeginPlay). */
	UFUNCTION(BlueprintImplementableEvent, Category = "Tutorial|Events")
	void BP_RegisterWithGameMode();

	/** Hide player's main HUD widget. */
	UFUNCTION(BlueprintImplementableEvent, Category = "Tutorial|Events")
	void BP_HideHUD();

	/** Handoff complete. Create + Add WBP_TutorialOverlay to viewport. */
	UFUNCTION(BlueprintImplementableEvent, Category = "Tutorial|Events")
	void BP_OnIntroFinished();

	/** Play spell SFX (Play Sound 2D). */
	UFUNCTION(BlueprintImplementableEvent, Category = "Tutorial|Skull")
	void BP_PlayMagicSpellSound();

	/** Create WBP_TutorialBlackout → Add to Viewport (ZOrder 99). */
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

	UFUNCTION()
	void OnCinematicHandoffComplete();

	// ── Boundary guard ────────────────────────────────────────────────────────
	UFUNCTION()
	void OnOxygenSphereEndOverlap(
		UPrimitiveComponent* OverlappedComp,
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
	AActor*                  FindBaseCamp() const;

	// ── Timers ────────────────────────────────────────────────────────────────
	FTimerHandle TeleportFadeOutHandle;
	FTimerHandle TeleportFadeInHandle;
	FTimerHandle SpellDurationHandle;
	FTimerHandle SkullPauseHandle;
	FTimerHandle PostBlackSoundHandle;
	FTimerHandle LevelSwapHandle;

	UPROPERTY()
	AToolBase* SpawnedTutorialPickaxe = nullptr;
};
