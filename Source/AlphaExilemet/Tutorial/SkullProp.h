#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "SkullProp.generated.h"

class ATutorialDirector;
class APawn;

// ─────────────────────────────────────────────────────────────────────────────
// ASkullProp  v1
//
// C++ base class for BP_SkullProp.
// Change BP_SkullProp's parent class to this in the Blueprint Class Settings.
//
// Responsibilities:
//   - Tick:         look-at player (Yaw only, RInterpTo) + position flicker
//   - C++ state:    bCanInteract, bFlickerActive, SkullHomePosition
//   - Interaction:  HandleInteract() called from BP Event Interact
//   - Director link: SetDirector() called by ATutorialDirector::InitializeTutorial()
//
// What stays in BP_SkullProp:
//   - Event Interact  →  HandleInteract()           [one node]
//   - BP_OnBecameInteractable override (optional VFX when skull is ready)
// ─────────────────────────────────────────────────────────────────────────────

UCLASS(Abstract, Blueprintable)
class ALPHAEXILEMET_API ASkullProp : public AActor
{
	GENERATED_BODY()

public:
	ASkullProp();

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;

	// ── CONFIG ────────────────────────────────────────────────────────────────

	/** How fast the skull rotates to face the player. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Skull|Config")
	float LookAtInterpSpeed = 0.75f;

	/**
	 * Radius of the random position jitter applied each tick while flickering.
	 * Units: cm. 15 = subtle; 40+ = very chaotic.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Skull|Config")
	float FlickerIntensity = 15.0f;

	// ── RUNTIME STATE ─────────────────────────────────────────────────────────

	/**
	 * Set to true by OnRiseComplete() when the skull finishes rising.
	 * Set to false by HandleInteract() after the first valid interaction.
	 * Read by Event Interact in BP to guard double-interaction.
	 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Skull|Runtime")
	bool bCanInteract = false;

	/**
	 * When true, Tick randomly offsets the skull around SkullHomePosition.
	 * Driven by StartFlicker() / StopAndReset().  Do not set directly in BP.
	 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Skull|Runtime")
	bool bFlickerActive = false;

	/**
	 * World location saved by OnRiseComplete() when the skull finishes rising.
	 * StopAndReset() snaps back here, invisible under the black screen.
	 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Skull|Runtime")
	FVector SkullHomePosition = FVector::ZeroVector;

	/**
	 * Set by ATutorialDirector::InitializeTutorial().
	 * Used by HandleInteract() to call back into the director without an
	 * expensive GetActorOfClass search on every player input.
	 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Skull|Runtime")
	ATutorialDirector* DirectorRef = nullptr;

	// ── PUBLIC API  ───────────────────────────────────────────────────────────

	/**
	 * Called by ATutorialDirector::InitializeTutorial() to wire the back-reference.
	 * Must be called before any interaction can reach the director.
	 */
	UFUNCTION(BlueprintCallable, Category = "Skull")
	void SetDirector(ATutorialDirector* Director);

	/**
	 * Called from the BP Timeline Finished pin at the end of SkullRiseTL.
	 *   1. Saves current world location as SkullHomePosition.
	 *   2. Sets bCanInteract = true.
	 *   3. Fires BP_OnBecameInteractable for optional visual feedback.
	 */
	UFUNCTION(BlueprintCallable, Category = "Skull")
	void OnRiseComplete();

	/**
	 * Starts per-tick random position jitter around SkullHomePosition.
	 * Called by ATutorialDirector::OnSkullInteracted().
	 * If SkullHomePosition was never set, saves the current location as a
	 * safety fallback (logs a warning).
	 */
	UFUNCTION(BlueprintCallable, Category = "Skull")
	void StartFlicker();

	/**
	 * Stops flicker and teleports the skull back to SkullHomePosition.
	 * Called by ATutorialDirector::OnSpellDurationComplete() at the exact
	 * instant the black screen appears, so the snap is invisible to the player.
	 */
	UFUNCTION(BlueprintCallable, Category = "Skull")
	void StopAndReset();

	/**
	 * Called from BP "Event Interact" (Interactable interface implementation).
	 * Guards bCanInteract, clears it, then calls DirectorRef→OnSkullInteracted().
	 *
	 * BP_SkullProp Event Interact node chain:
	 *   [Event Interact]  →  [HandleInteract (self)]
	 *   That is the entire BP implementation.
	 */
	UFUNCTION(BlueprintCallable, Category = "Skull")
	void HandleInteract();

	// ── BLUEPRINT EVENTS ──────────────────────────────────────────────────────

	/**
	 * Fired when the skull becomes interactable (end of rise animation).
	 * Implement in BP_SkullProp for optional particle / material feedback.
	 * Not required — leave empty if no visual is needed.
	 */
	UFUNCTION(BlueprintImplementableEvent, Category = "Skull|Events")
	void BP_OnBecameInteractable();

private:
	/**
	 * Cached player pawn for look-at targeting.
	 * Lazy-fetched the first tick the pawn is available so skull placement
	 * in the level does not depend on player spawn order.
	 */
	UPROPERTY()
	APawn* PlayerPawn = nullptr;
};
