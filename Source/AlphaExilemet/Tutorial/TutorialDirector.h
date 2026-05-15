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
// ATutorialDirector  v13
//
// Changes from v12:
//   - BP_RegisterWithGameMode BlueprintImplementableEvent added.
//     Called at the END of BeginPlay so the Director pushes itself to the GM.
//     This replaces the GM's GetAllActorsOfClass → GET[0] → IsValid chain
//     with a direct stored reference, eliminating the race condition entirely.
//
//   - BeginPlay now calls BP_RegisterWithGameMode after the standard setup.
//
// Race condition explanation:
//   GET[0] on an empty array crashes in BP (CallFunc_Array_Get_Item error).
//   The array is empty because GetAllActorsOfClass in the GM runs before
//   the Tutorial sublevel's actors complete BeginPlay.
//   Fix: the Director registers itself with the GM the moment it exists —
//   the GM never needs to search.
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
	 * The LevelSequenceActor for the tutorial intro cutscene.
	 * Assign once by dragging Cutscene1 from the Outliner into this slot
	 * in the placed BP_TutorialDirector's Details panel.
	 * No tags needed. Survives animation changes because Spawnables
	 * travel with the sequence asset, not with the level.
	 */
	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Tutorial|Config",
		meta = (DisplayName = "Intro Sequence"))
	ALevelSequenceActor* IntroSequenceRef = nullptr;

	/**
	 * Optional: camera actor the view snaps to before sequence.Play().
	 * Prevents 1-frame FP flash. Leave null if the LevelSequence
	 * Camera Cut track already handles the view switch.
	 */
	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Tutorial|Config",
		meta = (DisplayName = "Cinecam Actor"))
	AActor* TutorialCineCamRef = nullptr;

	/** BP_Pickaxe class. Set in Class Defaults (not instance — it never changes). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Tutorial|Config")
	TSubclassOf<AToolBase> TutorialPickaxeClass;

	// ── RUNTIME STATE ─────────────────────────────────────────────────────────

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Tutorial|Runtime")
	ULevelSequencePlayer* IntroSequencePlayer = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Tutorial|Runtime")
	FVector CraterStartPosition = FVector::ZeroVector;

	/**
	 * Cached in InitializeTutorial() only — guaranteed non-null at that point.
	 * NEVER cache in BeginPlay: the pawn is not possessed yet.
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

	/** Called by GM_SimulatorGamemode (via stored TutorialDirectorRef) after the player is possessed. */
	UFUNCTION(BlueprintCallable, Category = "Tutorial")
	void InitializeTutorial();

	UFUNCTION(BlueprintCallable, Category = "Tutorial|Skull")
	void OnSkullInteracted();

	UFUNCTION(BlueprintCallable, Category = "Tutorial")
	void ClearTutorialPickaxe();

	// ── BLUEPRINT IMPLEMENTABLE EVENTS ────────────────────────────────────────
protected:
	/**
	 * Called at the END of C++ BeginPlay.
	 * Implement in BP_TutorialDirector to push self-reference to the GM:
	 *
	 *   [Event BP_RegisterWithGameMode]
	 *     → [Get Game Mode → Cast To GM_SimulatorGamemode]
	 *     → [SET TutorialDirectorRef  (Target = As GM Simulator Gamemode, Value = Self)]
	 *
	 * After this, GM_SimulatorGamemode::SpawnNewGamePlayer calls:
	 *   [TutorialDirectorRef → Is Valid]
	 *     True → [Initialize Tutorial  (Target = TutorialDirectorRef)]
	 *
	 * No GetAllActorsOfClass, no array, no GET[0], no crash.
	 * BeginPlay fires while the sublevel loads — by the time SpawnNewGamePlayer
	 * runs, TutorialDirectorRef is already set.
	 */
	UFUNCTION(BlueprintImplementableEvent, Category = "Tutorial|Events")
	void BP_RegisterWithGameMode();

	/** Hide the player's main HUD. Use CachedPlayer (BlueprintReadOnly). */
	UFUNCTION(BlueprintImplementableEvent, Category = "Tutorial|Events")
	void BP_HideHUD();

	/** Cutscene finished. Create and Add WBP_TutorialOverlay to Viewport here. */
	UFUNCTION(BlueprintImplementableEvent, Category = "Tutorial|Events")
	void BP_OnIntroFinished();

	/** Play spell SFX (Play Sound 2D). */
	UFUNCTION(BlueprintImplementableEvent, Category = "Tutorial|Skull")
	void BP_PlayMagicSpellSound();

	/**
	 * Create WBP_TutorialBlackout → Add to Viewport (ZOrder 99).
	 *
	 * BP implementation — two nodes only:
	 *   [Event BP_ShowInstantBlack]
	 *     → [Create WB Tutorial Blackout Widget  (Owning Player = Get Player Controller)]
	 *     → [Add to Viewport  ZOrder = 99]
	 *
	 * The widget's Event Construct binds to AlphaStreamingSubsystem::OnTutorialPlayerReady.
	 * When OnTutorialPlayerReady fires (broadcast by GM after player setup is done),
	 * the widget removes itself.  The widget never calls the GM.
	 */
	UFUNCTION(BlueprintImplementableEvent, Category = "Tutorial|Skull")
	void BP_ShowInstantBlack();

	/** Play explosion SFX at ship location. Guard with IsValid(ExplosionSpawnRef). */
	UFUNCTION(BlueprintImplementableEvent, Category = "Tutorial|Skull")
	void BP_PlayExplosionSequence();

private:
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

	void OnSpellDurationComplete();
	void OnPostBlackDelay();
	void OnLevelSwapReady();

	void SuppressPlayerMoveInput();
	void RestorePlayerMoveInput();
	void LockPlayerInputFull();

	AAlphaExilemetCharacter* GetTutorialPlayer() const;
	AActor*                  FindBaseCamp() const;

	FTimerHandle TeleportFadeOutHandle;
	FTimerHandle TeleportFadeInHandle;
	FTimerHandle SpellDurationHandle;
	FTimerHandle PostBlackSoundHandle;
	FTimerHandle LevelSwapHandle;

	UPROPERTY()
	AToolBase* SpawnedTutorialPickaxe = nullptr;
};