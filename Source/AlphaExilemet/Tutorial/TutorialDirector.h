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
// ATutorialDirector  v16
//
// Changes from v15:
//   - SmoothCutsceneTransition system added.
//     The intro cutscene ends with a CineCamera positioned at the player's
//     head socket. Instead of an instant snap, we:
//       1. Keep the CineCamera as the view target after the sequence ends.
//       2. Move the CineCamera smoothly toward the real player's head socket
//          over CutsceneTransitionBlendTime seconds using a tick-based lerp.
//       3. Once the CineCamera is close enough (< 2 cm), we hide the proxy,
//          unhide the player, switch view target to the player, and give input.
//     This makes the cutscene-to-gameplay transition invisible.
//
//   - WakeUpDirector gets the same system (same parameters, separate flow).
//   - ProxySwap is now deferred until SmoothCutsceneTransition completes.
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

	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Tutorial|Config",
		meta = (DisplayName = "Intro Sequence"))
	ALevelSequenceActor* IntroSequenceRef = nullptr;

	/** The CineCamera actor that the sequence ends on. The smooth transition
	 *  will move this camera into the player head socket position, then swap.
	 *  Must be the SAME CineCamera that is the last active cut in the sequence.
	 *  Assign in the placed BP_TutorialDirector instance Details panel. */
	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Tutorial|Config",
		meta = (DisplayName = "Sequence End CineCamera"))
	AActor* SequenceEndCameraRef = nullptr;

	/** Optional: camera actor for the cinematic view before sequence plays. */
	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Tutorial|Config",
		meta = (DisplayName = "Cinecam Actor (Pre-Play)"))
	AActor* TutorialCineCamRef = nullptr;

	/** BP_Pickaxe class to spawn when the cutscene ends. Set in Class Defaults. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Tutorial|Config")
	TSubclassOf<AToolBase> TutorialPickaxeClass;

	/** Actor tag identifying the proxy skeletal mesh inside the Level Sequence. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Tutorial|Config",
		meta = (DisplayName = "Proxy Character Tag"))
	FName ProxyCharacterTag = FName("CutsceneProxy");

	/**
	 * Time in seconds for the CineCamera to smoothly move into the player's
	 * head socket before the view target switches to the player.
	 * 0.0 = instant snap (same as old behaviour).
	 * 0.3–0.6 = recommended for a smooth, invisible handoff.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Tutorial|Config",
		meta = (DisplayName = "Cutscene Transition Blend Time"))
	float CutsceneTransitionBlendTime = 0.5f;

	/**
	 * Distance threshold (cm) at which the CineCamera is considered "at" the
	 * player head socket. When the camera gets closer than this, the swap fires.
	 * Default 3.0 cm is essentially invisible at normal play speeds.
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

	/**
	 * The deferred proxy swap. Called once the smooth camera transition
	 * completes (or immediately if CutsceneTransitionBlendTime == 0).
	 * Teleports the player to the proxy's last position, unhides the player,
	 * hides the proxy, and gives camera + input back to the player.
	 */
	void ExecuteProxySwap();

	// ── SMOOTH TRANSITION STATE ───────────────────────────────────────────────

	/** True while the CineCamera is lerping toward the player head socket. */
	bool bTransitionActive = false;

	/** Elapsed time since the smooth transition started. */
	float TransitionElapsed = 0.0f;

	/** World position of the player head socket captured at sequence end. */
	FVector TransitionTargetLocation = FVector::ZeroVector;

	/** World rotation of the player's first-person camera at sequence end. */
	FRotator TransitionTargetRotation = FRotator::ZeroRotator;

	/** World location of the CineCamera at the moment the sequence ended. */
	FVector TransitionStartLocation = FVector::ZeroVector;

	/** Rotation of the CineCamera at the moment the sequence ended. */
	FRotator TransitionStartRotation = FRotator::ZeroRotator;

	/**
	 * Tick-based smooth lerp of SequenceEndCameraRef toward the player head socket.
	 * Fires ExecuteProxySwap when within TransitionSnapDistance.
	 */
	void TickSmoothTransition(float DeltaTime);

	// ── SEQUENCE CALLBACKS ────────────────────────────────────────────────────

	UFUNCTION() void OnIntroSequenceFinished();

	UFUNCTION()
	void OnOxygenSphereEndOverlap(
		UPrimitiveComponent* OverlappedComp,
		AActor*              OtherActor,
		UPrimitiveComponent* OtherComp,
		int32                OtherBodyIndex);

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
	AActor*                  FindBaseCamp() const;

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
