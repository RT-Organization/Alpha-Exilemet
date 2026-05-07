#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TutorialDirector.generated.h"

class ULevelSequencePlayer;
class ALevelSequenceActor;
class AAlphaExilemetCharacter;

// ─────────────────────────────────────────────────────────────────────────────
// ATutorialDirector
//
// C++ base class that owns the time-critical, race-condition-prone logic:
//   • Finding and playing CS_TutorialIntro
//   • Disabling / re-enabling player input around the cinematic
//   • Capturing CraterStartPosition when the cinematic ends
//   • Binding the out-of-bounds overlap guard AFTER the player is valid
//
// Everything that is already working in Blueprint (rock counting, skull reveal
// timeline, boundary camera fade) lives in the Blueprint child class
// BP_TutorialDirector and is untouched.
//
// INIT FLOW:
//   GameMode::SpawnNewGamePlayer()
//     → spawns + possesses BP_Player
//     → calls ATutorialDirector::InitializeTutorial()   ← new step
//         → binds overlap guard (player is now guaranteed valid)
//         → finds Cutscene1 by tag
//         → disables input
//         → plays the sequence
//   Sequence ends → OnIntroSequenceFinished()
//         → restores input
//         → captures CraterStartPosition
//         → calls BP_OnIntroFinished() (implementable event for child BP)
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
	/**
	 * Actor tag on the LevelSequenceActor that holds CS_TutorialIntro.
	 * Must match the tag set in the Unreal Details panel on the sequence actor.
	 * Default: "TutorialIntroSeq"
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Tutorial|Config")
	FName IntroSequenceTag = FName("TutorialIntroSeq");

	// ── RUNTIME STATE (readable from child Blueprint) ───────────────────────
public:
	/** The sequence player for CS_TutorialIntro. Cached in InitializeTutorial(). */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Tutorial|Runtime")
	ULevelSequencePlayer* IntroSequencePlayer = nullptr;

	/**
	 * World position captured at the moment CS_TutorialIntro ends.
	 * This is where Finn stands when the player gets control.
	 * The boundary system uses it to teleport the player back inside the bubble.
	 * Child Blueprint reads this via the BlueprintReadOnly getter.
	 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Tutorial|Runtime")
	FVector CraterStartPosition = FVector::ZeroVector;

	// ── PUBLIC INTERFACE ────────────────────────────────────────────────────
public:
	/**
	 * Call this from the GameMode AFTER the player has been spawned and
	 * possessed. It is NOT safe to call from BeginPlay because the player
	 * may not exist yet when the Tutorial sub-level initializes.
	 *
	 * What it does:
	 *   1. Gets the player reference (early-outs if still null).
	 *   2. Finds the Level Sequence actor by tag and caches the Sequence Player.
	 *   3. Binds the OnIntroSequenceFinished callback.
	 *   4. Binds the out-of-bounds overlap guard on the BaseCamp OxygenSphere.
	 *   5. Disables player input.
	 *   6. Plays the intro sequence.
	 */
	UFUNCTION(BlueprintCallable, Category = "Tutorial")
	void InitializeTutorial();

	// ── BLUEPRINT IMPLEMENTABLE EVENTS ──────────────────────────────────────
protected:
	/**
	 * Called immediately after CS_TutorialIntro finishes and input is restored.
	 * Override in BP_TutorialDirector to show any first-frame tutorial UI,
	 * unlock the pickaxe prompt, etc.
	 */
	UFUNCTION(BlueprintImplementableEvent, Category = "Tutorial|Events")
	void BP_OnIntroFinished();

	/**
	 * Called when the player leaves the BaseCamp OxygenSphere during Tutorial.
	 * The C++ base handles the teleport. Override in BP for extra FX (camera fade,
	 * UI flash, etc.) if desired.
	 *
	 * NOTE: The existing boundary camera-fade logic already built in Blueprint
	 * lives in the child class and can call Super or replace this entirely.
	 */
	UFUNCTION(BlueprintImplementableEvent, Category = "Tutorial|Events")
	void BP_OnPlayerLeftBase();

	// ── PRIVATE IMPLEMENTATION ──────────────────────────────────────────────
private:
	/** Latent callback bound to ULevelSequencePlayer::OnStop. */
	UFUNCTION()
	void OnIntroSequenceFinished();

	/**
	 * Overlap end callback bound to the BaseCamp OxygenSphere.
	 * Only fires for BP_Player overlaps. Teleports player to CraterStartPosition
	 * and calls BP_OnPlayerLeftBase for any Blueprint-side FX.
	 */
	UFUNCTION()
	void OnOxygenSphereEndOverlap(
		UPrimitiveComponent* OverlappedComp,
		AActor*              OtherActor,
		UPrimitiveComponent* OtherComp,
		int32                OtherBodyIndex);

	/** Returns the player character cast to AAlphaExilemetCharacter, or nullptr. */
	AAlphaExilemetCharacter* GetTutorialPlayer() const;

	/** Finds and returns the BaseCamp actor in the current world. */
	AActor* FindBaseCamp() const;
};