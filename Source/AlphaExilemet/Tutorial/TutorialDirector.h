#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TutorialDirector.generated.h"

class ULevelSequencePlayer;
class ALevelSequenceActor;
class AAlphaExilemetCharacter;
class AToolBase;

// ─────────────────────────────────────────────────────────────────────────────
// ATutorialDirector  v8
//
// KEY DESIGN DECISIONS in this version:
//
// 1. BOUNDARY TELEPORT — C++ does NOT teleport.
//    C++ only calls BP_OnPlayerLeftBase(). The Blueprint override owns the
//    full sequence: disable input → fade to black → teleport → fade clear →
//    enable input. This way the player never sees the teleport.
//
// 2. PICKAXE — spawned and equipped in OnIntroSequenceFinished().
//    If TutorialPickaxeClass is null the spawn is skipped silently.
//    On Tutorial→Main the streaming subsystem clears the inventory via
//    HandleTutorialCompletion() which calls SavePlayerData() — the pickaxe
//    is NOT saved because it's a tutorial-only actor. The GameMode also
//    calls ClearTutorialInventory() before InitializePlayer() in Main.
//
// 3. HUD — hidden at the start of InitializeTutorial().
//    BP_OnIntroFinished() is the hook to fade the HUD back in.
//    C++ calls BP_HideHUD() and BP_ShowHUDWithFade() at the right moments.
//
// 4. RACE CONDITION FIX (L_Persistent bug) —
//    InitializeTutorial() is called by SpawnNewGamePlayer in the GameMode.
//    BUT the Director only exists after Tutorial finishes streaming in.
//    The fix: SpawnNewGamePlayer uses a short Delay (0.3s already there)
//    then calls InitializeTutorial. Since StreamLevel is async, the
//    Director may not exist yet. The correct fix is described in the
//    Blueprint guide: use OnStreamComplete delegate from AlphaStreamingSubsystem
//    to trigger SpawnNewGamePlayer instead of firing it immediately.
//    See Group M in the implementation guide.
//
// ─────────────────────────────────────────────────────────────────────────────

UCLASS(Abstract, Blueprintable)
class ALPHAEXILEMET_API ATutorialDirector : public AActor
{
	GENERATED_BODY()

public:
	ATutorialDirector();

protected:
	virtual void BeginPlay() override;

	// ── CONFIGURATION ──────────────────────────────────────────────────────
public:
	/** Tag on the LevelSequenceActor. Default: TutorialIntroSeq. Case-sensitive. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Tutorial|Config")
	FName IntroSequenceTag = FName("TutorialIntroSeq");

	/**
	 * Assign BP_Pickaxe here in the Blueprint CDO (Class Defaults).
	 * Spawned and equipped when the cinematic ends.
	 * Must be cleared before the Tutorial→Main transition.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Tutorial|Config")
	TSubclassOf<AToolBase> TutorialPickaxeClass;

	// ── RUNTIME STATE ────────────────────────────────────────────────────────
public:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Tutorial|Runtime")
	ULevelSequencePlayer* IntroSequencePlayer = nullptr;

	/**
	 * Captured when the cinematic ends — where Finn stands in the crater.
	 * Used by BP_OnPlayerLeftBase to know where to teleport the player back.
	 * IsZero() == true until the cinematic ends: guards against premature
	 * overlap events during level load.
	 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Tutorial|Runtime")
	FVector CraterStartPosition = FVector::ZeroVector;

	/**
	 * Set from BP_TutorialDirector's StartSkullReveal after skull is positioned.
	 * BP_SkullProp calls DirectorRef.OnSkullInteracted() directly — no GetActorOfClass.
	 */
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Tutorial|Skull")
	AActor* SkullRef = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Tutorial|Skull")
	bool bSkullInteractionActive = false;

	// ── PUBLIC INTERFACE ────────────────────────────────────────────────────
public:
	/**
	 * Called by GM_SimulatorGamemode after player is spawned and possessed.
	 * NOT safe from BeginPlay — Tutorial actors are not guaranteed to exist yet.
	 */
	UFUNCTION(BlueprintCallable, Category = "Tutorial")
	void InitializeTutorial();

	/**
	 * Called by BP_SkullProp via stored DirectorRef — no GetActorOfClass.
	 * Starts the skull flicker + spell sound → instant black → explosion → level swap.
	 */
	UFUNCTION(BlueprintCallable, Category = "Tutorial|Skull")
	void OnSkullInteracted();

	/**
	 * Clears the tutorial pickaxe from the player inventory.
	 * Called before the Tutorial→Main transition (HandleTutorialCompletion).
	 * Also destroys the pickaxe actor from the world.
	 */
	UFUNCTION(BlueprintCallable, Category = "Tutorial")
	void ClearTutorialPickaxe();

	// ── BLUEPRINT IMPLEMENTABLE EVENTS ──────────────────────────────────────
protected:
	/**
	 * Called immediately when InitializeTutorial starts — before the cinematic plays.
	 * Use this to hide the HUD instantly (set HUD widget visibility to Hidden).
	 * The player has just been possessed so the HUD may have appeared briefly.
	 */
	UFUNCTION(BlueprintImplementableEvent, Category = "Tutorial|Events")
	void BP_HideHUD();

	/**
	 * Called after the cinematic ends and input is restored.
	 * Use this to fade the HUD back in (play a UMG fade-in animation on the HUD widget).
	 * C++ already: restored input, spawned + equipped the pickaxe.
	 * You add: HUD fade-in, any hint UI or subtitle.
	 */
	UFUNCTION(BlueprintImplementableEvent, Category = "Tutorial|Events")
	void BP_OnIntroFinished();

	/**
	 * Called when the player leaves the BaseCamp OxygenSphere.
	 * C++ does NOT teleport — this event owns the full sequence:
	 *   1. Disable input (Set Ignore Move Input).
	 *   2. Start Camera Fade to black (1.0s).
	 *   3. Delay 1.0s.
	 *   4. Set Actor Location to CraterStartPosition (teleport here, invisible).
	 *   5. Start Camera Fade to clear (1.0s).
	 *   6. Enable input.
	 * CraterStartPosition is readable from Blueprint via the BlueprintReadOnly property.
	 */
	UFUNCTION(BlueprintImplementableEvent, Category = "Tutorial|Events")
	void BP_OnPlayerLeftBase();

	/** Set bFlickerActive = true on the skull (via SkullRef cast to BP_SkullProp). */
	UFUNCTION(BlueprintImplementableEvent, Category = "Tutorial|Skull")
	void BP_StartSkullFlicker();

	/** Play the 5-second magic spell sound. */
	UFUNCTION(BlueprintImplementableEvent, Category = "Tutorial|Skull")
	void BP_PlayMagicSpellSound();

	/**
	 * Create WB_TutorialBlackout at full opacity (no fade) and add to viewport ZOrder 99.
	 * WB_TutorialBlackout Event Construct auto-binds to OnTutorialToMainComplete.
	 * CRITICAL: must fire 4.5 seconds before HandleTutorialCompletion.
	 */
	UFUNCTION(BlueprintImplementableEvent, Category = "Tutorial|Skull")
	void BP_ShowInstantBlack();

	/** Play explosion + shockwave sounds simultaneously. */
	UFUNCTION(BlueprintImplementableEvent, Category = "Tutorial|Skull")
	void BP_PlayExplosionSequence();

	// ── PRIVATE ─────────────────────────────────────────────────────────────
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

	// Weak reference to the spawned tutorial pickaxe for cleanup
	UPROPERTY()
	AToolBase* SpawnedTutorialPickaxe = nullptr;
};
