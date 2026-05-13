#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TutorialDirector.generated.h"

class ULevelSequencePlayer;
class ALevelSequenceActor;
class AAlphaExilemetCharacter;
class AToolBase;
class ASkullProp;               // ← typed ref replaces AActor*
class APlayerController;
class UCharacterMovementComponent;

// ─────────────────────────────────────────────────────────────────────────────
// ATutorialDirector  v11
//
// Changes from v10:
//   - CachedPlayer + CachedPC added; set in InitializeTutorial() (not BeginPlay).
//     This is the ONLY safe moment — the player is guaranteed spawned by then.
//     Remove ChaceVariable macro + PlayerRef variable from BP_TutorialDirector.
//
//   - SkullRef type: AActor* → ASkullProp*
//     Enables direct C++ calls (StartFlicker, StopAndReset, OnRiseComplete)
//     with no cast overhead and no BP event indirection.
//
//   - BP_OnPlayerLeftBase REMOVED — replaced by ExecuteTeleportToCrater() in C++.
//     Full camera-fade + teleport + re-enable is handled by three timer stages.
//
//   - BP_StartSkullFlicker REMOVED — OnSkullInteracted() calls SkullRef→StartFlicker().
//   - BP_StopAndResetSkull REMOVED — OnSpellDurationComplete() calls SkullRef→StopAndReset().
//
//   - OnSkullInteracted() updated: does NOT lock input anymore.
//     Player is free to move during the flicker sequence.
//     Input is locked inside OnSpellDurationComplete() at the instant-black moment.
//
//   - LockPlayerInputFull() / SuppressPlayerMoveInput() / RestorePlayerMoveInput()
//     added for explicit, readable input state management.
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
	// ── CONFIG ────────────────────────────────────────────────────────────────

	/** Tag on the LevelSequenceActor for CS_TutorialIntro. Default: TutorialIntroSeq */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Tutorial|Config")
	FName IntroSequenceTag = FName("TutorialIntroSeq");

	/**
	 * BP_Pickaxe class. Assign in BP_TutorialDirector Class Defaults.
	 * Spawned + equipped when cutscene ends. Cleared on skull interaction.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Tutorial|Config")
	TSubclassOf<AToolBase> TutorialPickaxeClass;

	// ── RUNTIME STATE ─────────────────────────────────────────────────────────

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Tutorial|Runtime")
	ULevelSequencePlayer* IntroSequencePlayer = nullptr;

	/**
	 * Captured in OnIntroSequenceFinished. The player's crater position.
	 * Zero until the cutscene ends — guards premature overlap events.
	 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Tutorial|Runtime")
	FVector CraterStartPosition = FVector::ZeroVector;

	/**
	 * Cached in InitializeTutorial() — GUARANTEED non-null at that point
	 * because GM_SimulatorGamemode calls InitializeTutorial() after spawning
	 * the player.
	 *
	 * USE THIS in all BP implementable events instead of a BP-side PlayerRef.
	 * Never cache the player in BP BeginPlay — it fires before the pawn is
	 * possessed and GetPlayerCharacter(0) returns null.
	 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Tutorial|Runtime")
	AAlphaExilemetCharacter* CachedPlayer = nullptr;

	/**
	 * Player controller cached alongside CachedPlayer in InitializeTutorial().
	 * Used for input mode changes, camera fades, and move-input suppression.
	 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Tutorial|Runtime")
	APlayerController* CachedPC = nullptr;

	/**
	 * Typed skull reference — set in BP_TutorialDirector BeginPlay via
	 * GetActorOfClass(ASkullProp) → SET SkullRef.
	 * Must be set before InitializeTutorial() is called (it wires DirectorRef
	 * on the skull).
	 */
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Tutorial|Skull")
	ASkullProp* SkullRef = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Tutorial|Skull")
	bool bSkullInteractionActive = false;

	// ── PUBLIC INTERFACE ──────────────────────────────────────────────────────

	/** Called by GM_SimulatorGamemode after player spawned + possessed. */
	UFUNCTION(BlueprintCallable, Category = "Tutorial")
	void InitializeTutorial();

	/**
	 * Called by ASkullProp::HandleInteract() when the player presses Interact
	 * on the skull and bCanInteract is true.
	 *
	 * Sequence:
	 *   - Removes tutorial pickaxe
	 *   - Starts skull flicker (C++ directly)
	 *   - Plays spell SFX (BP event)
	 *   - Player remains FREE TO MOVE for 5.5s
	 *   - At 5.5s → OnSpellDurationComplete locks input + shows black screen
	 */
	UFUNCTION(BlueprintCallable, Category = "Tutorial|Skull")
	void OnSkullInteracted();

	/** Removes the tutorial pickaxe. Auto-called by OnSkullInteracted(). */
	UFUNCTION(BlueprintCallable, Category = "Tutorial")
	void ClearTutorialPickaxe();

	// ── BLUEPRINT IMPLEMENTABLE EVENTS ────────────────────────────────────────
protected:
	/**
	 * Called at the START of InitializeTutorial().
	 * Hide the player's main HUD widget here.
	 * Use CachedPlayer (BlueprintReadOnly) — do NOT call GetPlayerCharacter
	 * and do NOT use a separately-cached BP PlayerRef.
	 */
	UFUNCTION(BlueprintImplementableEvent, Category = "Tutorial|Events")
	void BP_HideHUD();

	/** Called after the cutscene ends. Create and add WBP_TutorialOverlay here. */
	UFUNCTION(BlueprintImplementableEvent, Category = "Tutorial|Events")
	void BP_OnIntroFinished();

	/** Play the magic/spell SFX (Play Sound 2D). */
	UFUNCTION(BlueprintImplementableEvent, Category = "Tutorial|Skull")
	void BP_PlayMagicSpellSound();

	/**
	 * Create WBP_TutorialBlackout at opacity 1.0, ZOrder 99, Add to Viewport.
	 * This is called by OnSpellDurationComplete() — input is already locked
	 * and the skull is already reset before this fires.
	 */
	UFUNCTION(BlueprintImplementableEvent, Category = "Tutorial|Skull")
	void BP_ShowInstantBlack();

	/**
	 * Play the explosion SFX at the ship actor's world location.
	 * Use ExplosionSpawnRef (public, assignable in editor) for the position.
	 * Guard with IsValid(ExplosionSpawnRef) before Play Sound At Location.
	 */
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

	// Teleport sequence (replaces BP_OnPlayerLeftBase entirely)
	void ExecuteTeleportToCrater();   // Entry — locks move input, fades out
	void OnTeleportReadyToMove();     // After 1s fade-out: teleport, fade in
	void OnTeleportComplete();        // After 1s fade-in: re-enable move input

	// Skull interaction sequence
	void OnSpellDurationComplete();   // 5.5s after interaction: black + stop skull
	void OnPostBlackDelay();          // 1.0s after black: explosion SFX
	void OnLevelSwapReady();          // 3.0s after explosion: level swap

	// ── INPUT HELPERS ─────────────────────────────────────────────────────────

	/**
	 * Soft lock used during the teleport sequence.
	 * Only blocks move input. Movement component stays in Walking mode so
	 * gravity and physics continue to work correctly during the fade.
	 */
	void SuppressPlayerMoveInput();

	/**
	 * Restores move input after SuppressPlayerMoveInput().
	 * Also forces MovementMode back to Walking in case it drifted to None.
	 */
	void RestorePlayerMoveInput();

	/**
	 * Hard lock used at the instant-black moment of the skull sequence.
	 * Disables the movement component entirely + switches to UI-only input.
	 * Never reversed — the player enters the Main level fresh.
	 */
	void LockPlayerInputFull();

	// ── HELPERS ───────────────────────────────────────────────────────────────

	AAlphaExilemetCharacter* GetTutorialPlayer() const;
	AActor*                  FindBaseCamp() const;

	// ── TIMER HANDLES ─────────────────────────────────────────────────────────

	FTimerHandle TeleportFadeOutHandle;   // fires to execute teleport after fade-out
	FTimerHandle TeleportFadeInHandle;    // fires to restore input after fade-in
	FTimerHandle SpellDurationHandle;     // 5.5s spell → instant black
	FTimerHandle PostBlackSoundHandle;    // 1.0s under black → explosion SFX
	FTimerHandle LevelSwapHandle;         // 3.0s after explosion → level swap

	// ── OWNED ACTORS ──────────────────────────────────────────────────────────

	UPROPERTY()
	AToolBase* SpawnedTutorialPickaxe = nullptr;
};
