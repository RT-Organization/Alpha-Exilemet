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
// ATutorialDirector  v14
//
// Changes from v13:
//   - Skull sequence timing fixed:
//       OnSpellDurationComplete:  StopAndReset skull → 0.5 s pause (player sees it still)
//       OnSkullPausedBeforeBlack: LockInput → BP_ShowInstantBlack → 1.0 s
//       OnPostBlackDelay:         BP_PlayExplosionSequence → 3.0 s
//       OnLevelSwapReady:         HandleTutorialCompletion
//
//   - SkullPauseHandle added.
//   - OnSkullPausedBeforeBlack added (private).
//
// Sequence intent:
//   Player watches skull flicker for 5.5 s.
//   Skull snaps to home and stands still for 0.5 s — player sees it stop.
//   Input locks + black screen appears.
//   Explosion SFX plays under black.
//   Level swaps.
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
	// ── LEVEL REFERENCES — assign in the placed instance Details panel ────────

	/**
	 * The LevelSequenceActor for the intro cutscene.
	 * Assign by selecting the placed BP_TutorialDirector → Details
	 *   → Tutorial|Config → Intro Sequence → drag Cutscene1 from Outliner.
	 */
	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Tutorial|Config",
		meta = (DisplayName = "Intro Sequence"))
	ALevelSequenceActor* IntroSequenceRef = nullptr;

	/**
	 * Optional cinecam actor. View snaps here before sequence plays to avoid
	 * 1-frame first-person flash. Leave null if the Camera Cut track handles it.
	 */
	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Tutorial|Config",
		meta = (DisplayName = "Cinecam Actor"))
	AActor* TutorialCineCamRef = nullptr;

	/** BP_Pickaxe class to spawn when the cutscene ends. Set in Class Defaults. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Tutorial|Config")
	TSubclassOf<AToolBase> TutorialPickaxeClass;

	// ── RUNTIME STATE ─────────────────────────────────────────────────────────

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Tutorial|Runtime")
	ULevelSequencePlayer* IntroSequencePlayer = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Tutorial|Runtime")
	FVector CraterStartPosition = FVector::ZeroVector;

	/**
	 * Cached in InitializeTutorial() — guaranteed non-null at that point.
	 * GM calls InitializeTutorial() after the pawn is spawned and possessed.
	 * NEVER cache the player in BP BeginPlay.
	 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Tutorial|Runtime")
	AAlphaExilemetCharacter* CachedPlayer = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Tutorial|Runtime")
	APlayerController* CachedPC = nullptr;

	/** Set in BP BeginPlay via GetActorOfClass(ASkullProp) → SET SkullRef. */
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Tutorial|Skull")
	ASkullProp* SkullRef = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Tutorial|Skull")
	bool bSkullInteractionActive = false;

	// ── PUBLIC INTERFACE ──────────────────────────────────────────────────────

	/** Called by GM after player is spawned + possessed. */
	UFUNCTION(BlueprintCallable, Category = "Tutorial")
	void InitializeTutorial();

	/**
	 * Called by ASkullProp::HandleInteract().
	 *
	 * Full timing (all driven by C++ timers — no BP delays):
	 *   0.0 s  skull flicker starts, spell SFX — player FREE TO MOVE & LOOK
	 *   5.5 s  skull snaps to home (StopAndReset), player still free
	 *   6.0 s  (+0.5s pause) input locked + black screen
	 *   7.0 s  (+1.0s under black) explosion SFX
	 *  10.0 s  (+3.0s) Tutorial→Main level swap
	 */
	UFUNCTION(BlueprintCallable, Category = "Tutorial|Skull")
	void OnSkullInteracted();

	UFUNCTION(BlueprintCallable, Category = "Tutorial")
	void ClearTutorialPickaxe();

protected:
	// ── BLUEPRINT IMPLEMENTABLE EVENTS ────────────────────────────────────────

	/**
	 * Called at the END of C++ BeginPlay.
	 * Push self-reference to the GM so it can call InitializeTutorial()
	 * without a GetAllActorsOfClass search (eliminates the race condition).
	 *
	 *   [Event BP_RegisterWithGameMode]
	 *     → [Get Game Mode → Cast To GM_SimulatorGamemode]
	 *     → [SET TutorialDirectorRef  (Value = Self)]
	 */
	UFUNCTION(BlueprintImplementableEvent, Category = "Tutorial|Events")
	void BP_RegisterWithGameMode();

	/** Hide player's main HUD. Use CachedPlayer (BlueprintReadOnly). */
	UFUNCTION(BlueprintImplementableEvent, Category = "Tutorial|Events")
	void BP_HideHUD();

	/** Cutscene finished. Create + Add WBP_TutorialOverlay to Viewport. */
	UFUNCTION(BlueprintImplementableEvent, Category = "Tutorial|Events")
	void BP_OnIntroFinished();

	/** Play spell SFX (Play Sound 2D). */
	UFUNCTION(BlueprintImplementableEvent, Category = "Tutorial|Skull")
	void BP_PlayMagicSpellSound();

	/**
	 * Create WBP_TutorialBlackout → Add to Viewport (ZOrder 99).
	 * Called AFTER the 0.5 s skull-visible pause, so the player has already
	 * seen the skull standing still.
	 *
	 * Two-node BP implementation:
	 *   [Event BP_ShowInstantBlack]
	 *     → [Create WB Tutorial Blackout Widget (Owning Player = Get Player Controller)]
	 *     → [Add to Viewport  ZOrder = 99]
	 */
	UFUNCTION(BlueprintImplementableEvent, Category = "Tutorial|Skull")
	void BP_ShowInstantBlack();

	/** Play explosion SFX at ship location. Guard with IsValid(ExplosionSpawnRef). */
	UFUNCTION(BlueprintImplementableEvent, Category = "Tutorial|Skull")
	void BP_PlayExplosionSequence();

private:
	// ── SEQUENCE CALLBACKS ────────────────────────────────────────────────────

	UFUNCTION()
	void OnIntroSequenceFinished();

	UFUNCTION()
	void OnOxygenSphereEndOverlap(
		UPrimitiveComponent* OverlappedComp,
		AActor*              OtherActor,
		UPrimitiveComponent* OtherComp,
		int32                OtherBodyIndex);

	void ExecuteTeleportToCrater();
	void OnTeleportReadyToMove();
	void OnTeleportComplete();

	// Skull sequence — four stages
	void OnSpellDurationComplete();   // 5.5s: skull stops, 0.5s pause begins
	void OnSkullPausedBeforeBlack();  // 0.5s: player saw skull still → lock + black
	void OnPostBlackDelay();          // 1.0s under black: explosion SFX
	void OnLevelSwapReady();          // 3.0s: Tutorial→Main swap

	// ── INPUT HELPERS ─────────────────────────────────────────────────────────

	void SuppressPlayerMoveInput();
	void RestorePlayerMoveInput();
	void LockPlayerInputFull();

	// ── HELPERS ───────────────────────────────────────────────────────────────

	AAlphaExilemetCharacter* GetTutorialPlayer() const;
	AActor*                  FindBaseCamp() const;

	// ── TIMER HANDLES ─────────────────────────────────────────────────────────

	FTimerHandle TeleportFadeOutHandle;
	FTimerHandle TeleportFadeInHandle;
	FTimerHandle SpellDurationHandle;    // 5.5s spell → skull stops
	FTimerHandle SkullPauseHandle;       // 0.5s skull visible still → black screen
	FTimerHandle PostBlackSoundHandle;   // 1.0s under black → explosion
	FTimerHandle LevelSwapHandle;        // 3.0s → level swap

	UPROPERTY()
	AToolBase* SpawnedTutorialPickaxe = nullptr;
};
