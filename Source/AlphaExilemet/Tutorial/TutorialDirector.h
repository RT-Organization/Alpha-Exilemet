#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TutorialDirector.generated.h"

class ULevelSequencePlayer;
class ALevelSequenceActor;
class AAlphaExilemetCharacter;
class AToolBase;

// ─────────────────────────────────────────────────────────────────────────────
// ATutorialDirector  v9
//
// Bug fixes in this version vs v8:
//
// BUG 1 FIX — 1-frame FP view before cutscene:
//   InitializeTutorial() now calls SetViewTargetWithBlend(CineCameraActor, 0.0f)
//   immediately before Play(). Tag the CineCameraActor "TutorialCineCam".
//
// BUG 3 FIX — BP_OnPlayerLeftBase never firing:
//   OxygenSphere EndOverlap binding moved from InitializeTutorial() to
//   OnIntroSequenceFinished(). This guarantees BaseCamp is fully initialized
//   AND CraterStartPosition is set before any overlap event can fire.
//
// All other behavior identical to v8.
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
	// ── CONFIG (assign in BP Class Defaults) ──────────────────────────────────

	/** Tag on the LevelSequenceActor for CS_TutorialIntro. Default: TutorialIntroSeq */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Tutorial|Config")
	FName IntroSequenceTag = FName("TutorialIntroSeq");

	/**
	 * BP_Pickaxe class. Assign in BP_TutorialDirector Class Defaults.
	 * Spawned + equipped when cutscene ends. Cleared on skull interaction.
	 * If null: spawn is skipped silently (check Output Log for warning).
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Tutorial|Config")
	TSubclassOf<AToolBase> TutorialPickaxeClass;

	// ── RUNTIME STATE ────────────────────────────────────────────────────────

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Tutorial|Runtime")
	ULevelSequencePlayer* IntroSequencePlayer = nullptr;

	/**
	 * Captured in OnIntroSequenceFinished — where Finn stands at crater end.
	 * Read in BP_OnPlayerLeftBase via "Crater Start Position" property drag.
	 * Zero until cutscene ends (guards premature overlap events).
	 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Tutorial|Runtime")
	FVector CraterStartPosition = FVector::ZeroVector;

	/**
	 * Set by BP_TutorialDirector StartSkullReveal, after skull is positioned.
	 * BP_SkullProp calls DirectorRef.OnSkullInteracted() — no GetActorOfClass.
	 */
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Tutorial|Skull")
	AActor* SkullRef = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Tutorial|Skull")
	bool bSkullInteractionActive = false;

	// ── PUBLIC INTERFACE ──────────────────────────────────────────────────────

	/**
	 * Called by GM_SimulatorGamemode after player is spawned + possessed.
	 * NOT safe from BeginPlay.
	 *
	 * v9 change: forces SetViewTargetWithBlend(CineCameraActor, 0) before Play()
	 * to eliminate the 1-frame FP view. Requires CineCameraActor tagged "TutorialCineCam".
	 */
	UFUNCTION(BlueprintCallable, Category = "Tutorial")
	void InitializeTutorial();

	/**
	 * Called by BP_SkullProp via stored DirectorRef — no GetActorOfClass.
	 * Starts the skull flicker + spell → instant black → explosion → level swap.
	 * Also calls ClearTutorialPickaxe() before the level swap.
	 */
	UFUNCTION(BlueprintCallable, Category = "Tutorial|Skull")
	void OnSkullInteracted();

	/**
	 * Removes the tutorial pickaxe from OwnedTools and destroys the actor.
	 * Called automatically by OnSkullInteracted().
	 * Can also be called manually from Blueprint if needed.
	 */
	UFUNCTION(BlueprintCallable, Category = "Tutorial")
	void ClearTutorialPickaxe();

	// ── BLUEPRINT IMPLEMENTABLE EVENTS ────────────────────────────────────────
protected:
	/**
	 * Called at start of InitializeTutorial — before cutscene.
	 * Use: Get PlayerHUDRef from BP_Player -> Set Visibility Hidden.
	 */
	UFUNCTION(BlueprintImplementableEvent, Category = "Tutorial|Events")
	void BP_HideHUD();

	/**
	 * Called after cutscene ends and input is restored.
	 * C++ has already: restored input, equipped pickaxe.
	 * NOTE: Do NOT show the HUD here. HUD stays hidden during Tutorial gameplay.
	 * Use: show optional tutorial hint text only.
	 */
	UFUNCTION(BlueprintImplementableEvent, Category = "Tutorial|Events")
	void BP_OnIntroFinished();

	/**
	 * Called when player leaves OxygenSphere. C++ does NOT teleport.
	 * BP owns the full sequence:
	 *   SetIgnoreMoveInput(true)
	 *   -> CameraFade(0->1, 1s) -> Delay(1s)
	 *   -> SetActorLocation(CraterStartPosition)    <- teleport here (invisible)
	 *   -> CameraFade(1->0, 1s) -> Delay(1s)
	 *   -> SetIgnoreMoveInput(false)
	 * Read CraterStartPosition via drag-off Self -> "Crater Start Position".
	 */
	UFUNCTION(BlueprintImplementableEvent, Category = "Tutorial|Events")
	void BP_OnPlayerLeftBase();

	/** Set bFlickerActive=true on SkullRef. Cast SkullRef to BP_SkullProp. */
	UFUNCTION(BlueprintImplementableEvent, Category = "Tutorial|Skull")
	void BP_StartSkullFlicker();

	/** Play the 5-second magic spell sound. */
	UFUNCTION(BlueprintImplementableEvent, Category = "Tutorial|Skull")
	void BP_PlayMagicSpellSound();

	/**
	 * Create WB_TutorialBlackout at opacity 1.0 (no fade). Add to viewport ZOrder 99.
	 * Widget Event Construct auto-binds OnTutorialToMainComplete.
	 * Must fire 4.5s before HandleTutorialCompletion.
	 */
	UFUNCTION(BlueprintImplementableEvent, Category = "Tutorial|Skull")
	void BP_ShowInstantBlack();

	/** Play DistantExplosionSFX + ShockwaveSFX simultaneously. */
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

	void OnSpellDurationComplete();
	void OnPostBlackDelay();
	void OnLevelSwapReady();

	AAlphaExilemetCharacter* GetTutorialPlayer() const;
	AActor*                  FindBaseCamp() const;

	FTimerHandle SpellDurationHandle;
	FTimerHandle PostBlackSoundHandle;
	FTimerHandle LevelSwapHandle;

	UPROPERTY()
	AToolBase* SpawnedTutorialPickaxe = nullptr;
};
