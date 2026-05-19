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
class ACameraActor;

// ─────────────────────────────────────────────────────────────────────────────
// ATutorialDirector  v17
//
// Changes from v16:
//   - SequenceEndCameraRef REMOVED. No level setup needed by the designer.
//   - Smooth transition now works by reading the PlayerCameraManager's current
//     view location/rotation at the moment the sequence ends. This is exactly
//     where the Sequencer's CineCamera left the camera.
//   - A temporary invisible ACameraActor is spawned at that point and used as
//     the view target during the lerp. It moves toward FirstPersonCameraComponent
//     world transform over CutsceneTransitionBlendTime seconds, then is destroyed.
//   - FirstPersonCamera offset is used directly (not head bone socket) because
//     the camera is attached to the mesh with a custom offset, not at bone origin.
//   - Works with zero extra level setup — just set CutsceneTransitionBlendTime
//     in Class Defaults and compile.
// ─────────────────────────────────────────────────────────────────────────────

UCLASS(Abstract, Blueprintable)
class ALPHAEXILEMET_API ATutorialDirector : public AActor
{
	GENERATED_BODY()

public:
	ATutorialDirector();

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;

public:
	// ── LEVEL REFERENCES ──────────────────────────────────────────────────────

	/** The LevelSequenceActor for the intro cutscene (Cutscene1). */
	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Tutorial|Config",
		meta = (DisplayName = "Intro Sequence"))
	ALevelSequenceActor* IntroSequenceRef = nullptr;

	/** Optional: camera actor for the view before the sequence starts.
	 *  Leave null if the Camera Cut track handles the initial view. */
	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Tutorial|Config",
		meta = (DisplayName = "Cinecam Actor (Pre-Play)"))
	AActor* TutorialCineCamRef = nullptr;

	/** BP_Pickaxe class spawned when the cutscene finishes. Set in Class Defaults. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Tutorial|Config")
	TSubclassOf<AToolBase> TutorialPickaxeClass;

	/** Actor tag identifying the proxy skeletal mesh Spawnable inside the sequence. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Tutorial|Config",
		meta = (DisplayName = "Proxy Character Tag"))
	FName ProxyCharacterTag = FName("CutsceneProxy");

	/**
	 * How long (seconds) the temporary camera takes to slide from the
	 * Sequencer's final CineCamera position into the player's FirstPersonCamera.
	 * 0.0 = instant snap (old behaviour, no lerp).
	 * 0.4–0.7 = recommended for an invisible handoff.
	 *
	 * NO LEVEL SETUP NEEDED — the system reads the camera position automatically
	 * from PlayerCameraManager at the moment the sequence ends.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Tutorial|Config",
		meta = (DisplayName = "Cutscene Transition Blend Time"))
	float CutsceneTransitionBlendTime = 0.5f;

	/**
	 * Distance (cm) at which the temp camera snaps to the player camera and
	 * the view switches. 3.0 cm is imperceptible at normal frame rates.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Tutorial|Config",
		meta = (DisplayName = "Transition Snap Distance (cm)"))
	float TransitionSnapDistance = 3.0f;

	// ── RUNTIME STATE ─────────────────────────────────────────────────────────

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Tutorial|Runtime")
	ULevelSequencePlayer* IntroSequencePlayer = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Tutorial|Runtime")
	FVector CraterStartPosition = FVector::ZeroVector;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Tutorial|Runtime")
	AAlphaExilemetCharacter* CachedPlayer = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Tutorial|Runtime")
	APlayerController* CachedPC = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Tutorial|Skull")
	ASkullProp* SkullRef = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Tutorial|Skull")
	bool bSkullInteractionActive = false;

	// ── PUBLIC INTERFACE ──────────────────────────────────────────────────────

	UFUNCTION(BlueprintCallable, Category = "Tutorial")
	void InitializeTutorial();

	UFUNCTION(BlueprintCallable, Category = "Tutorial|Skull")
	void OnSkullInteracted();

	UFUNCTION(BlueprintCallable, Category = "Tutorial")
	void ClearTutorialPickaxe();

protected:
	// ── BLUEPRINT IMPLEMENTABLE EVENTS ────────────────────────────────────────

	UFUNCTION(BlueprintImplementableEvent, Category = "Tutorial|Events")
	void BP_RegisterWithGameMode();

	UFUNCTION(BlueprintImplementableEvent, Category = "Tutorial|Events")
	void BP_HideHUD();

	/** Called when transition is complete and player has full control.
	 *  Create and add WBP_TutorialOverlay here. */
	UFUNCTION(BlueprintImplementableEvent, Category = "Tutorial|Events")
	void BP_OnIntroFinished();

	UFUNCTION(BlueprintImplementableEvent, Category = "Tutorial|Skull")
	void BP_PlayMagicSpellSound();

	UFUNCTION(BlueprintImplementableEvent, Category = "Tutorial|Skull")
	void BP_ShowInstantBlack();

	UFUNCTION(BlueprintImplementableEvent, Category = "Tutorial|Skull")
	void BP_PlayExplosionSequence();

private:
	// ── PROXY SWAP ────────────────────────────────────────────────────────────

	void HidePlayerForCutscene();
	AActor* FindCutsceneProxy() const;
	void ExecuteProxySwap();

	// ── SMOOTH TRANSITION STATE ───────────────────────────────────────────────

	/**
	 * Temporary ACameraActor spawned at sequence end position.
	 * Lerps toward the player's FirstPersonCameraComponent, then destroyed.
	 * Never visible in the level — purely a view target for the lerp duration.
	 */
	UPROPERTY()
	ACameraActor* TempTransitionCamera = nullptr;

	bool     bTransitionActive        = false;
	float    TransitionElapsed        = 0.0f;
	FVector  TransitionStartLocation  = FVector::ZeroVector;
	FRotator TransitionStartRotation  = FRotator::ZeroRotator;

	/** Called each tick while bTransitionActive. Moves TempTransitionCamera
	 *  toward FirstPersonCameraComponent, fires ExecuteProxySwap on arrival. */
	void TickSmoothTransition(float DeltaTime);

	/** Runs after transition completes (or immediately on instant mode).
	 *  Gives player control, spawns pickaxe, wires boundary guard, calls BP_OnIntroFinished. */
	void FinishCutsceneHandoff();

	// ── SEQUENCE CALLBACK ─────────────────────────────────────────────────────

	UFUNCTION() void OnIntroSequenceFinished();

	UFUNCTION()
	void OnOxygenSphereEndOverlap(UPrimitiveComponent* OverlappedComp,
		AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);

	void ExecuteTeleportToCrater();
	void OnTeleportReadyToMove();
	void OnTeleportComplete();
	void OnSpellDurationComplete();
	void OnSkullPausedBeforeBlack();
	void OnPostBlackDelay();
	void OnLevelSwapReady();

	// ── INPUT HELPERS ─────────────────────────────────────────────────────────

	void SuppressPlayerMoveInput();
	void RestorePlayerMoveInput();
	void LockPlayerInputFull();

	// ── MISC HELPERS ──────────────────────────────────────────────────────────

	AAlphaExilemetCharacter* GetTutorialPlayer() const;
	AActor* FindBaseCamp() const;

	// ── TIMER HANDLES ─────────────────────────────────────────────────────────

	FTimerHandle TeleportFadeOutHandle;
	FTimerHandle TeleportFadeInHandle;
	FTimerHandle SpellDurationHandle;
	FTimerHandle SkullPauseHandle;
	FTimerHandle PostBlackSoundHandle;
	FTimerHandle LevelSwapHandle;

	UPROPERTY()
	AToolBase* SpawnedTutorialPickaxe = nullptr;
};
