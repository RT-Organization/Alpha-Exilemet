#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "CinematicHandoffComponent.generated.h"

class AAlphaExilemetCharacter;
class APlayerController;
class ACameraActor;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnCinematicHandoffComplete);

/**
 * UCinematicHandoffComponent  v3
 *
 * Tick-based ghost camera that travels from the sequence's last CineCamera
 * position to the SK proxy's head bone, then swaps in the player pawn.
 *
 * Changes from v2:
 *   - Safety fallback timer: if BeginHandoff is never called (sequence/proxy
 *     errors), input is restored after SafetyInputRestoreDelay seconds.
 *   - Explicitly clears bConstrainAspectRatio on the player FP camera when
 *     the view target is switched, eliminating Sequencer's black bars.
 *   - OnArrivalAtTarget defers view-target switch by one tick to let
 *     Sequencer finish its internal cinematic-mode teardown.
 */
UCLASS(ClassGroup=(AlphaExilemet), meta=(BlueprintSpawnableComponent),
       DisplayName = "Cinematic Handoff")
class ALPHAEXILEMET_API UCinematicHandoffComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UCinematicHandoffComponent();

	// ── CONFIG ────────────────────────────────────────────────────────────────

	/**
	 * Seconds for the ghost camera to travel from the CineCamera to the head bone.
	 * 0.8 – 1.5 for a wide establishing shot.
	 * 0.3 – 0.6 if the sequence camera ends close to the character.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Handoff|Config",
		meta = (ClampMin = "0.1"))
	float CameraBlendTime = 1.0f;

	/**
	 * Emergency safety valve: if OnHandoffComplete still hasn't fired after
	 * this many seconds since BeginHandoff was called, force-restore input
	 * and fire OnHandoffComplete anyway.
	 *
	 * Prevents the player from being permanently stuck if anything goes wrong
	 * (sequence teardown race condition, no proxy found, etc.).
	 * Default: 8.0s (well past the longest expected blend).
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Handoff|Config",
		meta = (ClampMin = "1.0"))
	float SafetyInputRestoreDelay = 8.0f;

	// ── EVENTS ────────────────────────────────────────────────────────────────

	/** Fires when the player has full camera control and input is safe to restore. */
	UPROPERTY(BlueprintAssignable, Category = "Handoff|Events")
	FOnCinematicHandoffComplete OnHandoffComplete;

	// ── RUNTIME STATE ─────────────────────────────────────────────────────────

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Handoff|Runtime")
	bool bHandoffInProgress = false;

	// ── PUBLIC API ────────────────────────────────────────────────────────────

	/**
	 * Begin the transition.
	 *
	 * @param Player               The hidden BP_Player pawn.
	 * @param PC                   The owning PlayerController.
	 * @param GhostStartLocation   World position of the last active CineCamera.
	 * @param GhostStartRotation   World rotation of the last active CineCamera.
	 * @param CameraTargetLocation World position to travel toward (head bone).
	 * @param PlayerSpawnTransform Where to place the BP_Player (root bone pos + yaw).
	 * @param ProxyActorToDestroy  Optional: SK proxy actor, destroyed at arrival.
	 */
	UFUNCTION(BlueprintCallable, Category = "Handoff")
	void BeginHandoff(
		AAlphaExilemetCharacter* Player,
		APlayerController*       PC,
		FVector                  GhostStartLocation,
		FRotator                 GhostStartRotation,
		FVector                  CameraTargetLocation,
		FTransform               PlayerSpawnTransform,
		AActor*                  ProxyActorToDestroy = nullptr);

	/** Abort without firing OnHandoffComplete. */
	UFUNCTION(BlueprintCallable, Category = "Handoff")
	void CancelHandoff();

	virtual void TickComponent(float DeltaTime, ELevelTick TickType,
		FActorComponentTickFunction* ThisTickFunction) override;

private:
	FVector  TravelStartPos;
	FVector  TravelEndPos;
	FQuat    TravelStartQuat;
	FQuat    TravelEndQuat;
	float    TravelElapsed = 0.0f;

	FTransform PendingPlayerSpawn;

	UPROPERTY() AAlphaExilemetCharacter* CachedPlayer = nullptr;
	UPROPERTY() APlayerController*       CachedPC     = nullptr;
	UPROPERTY() ACameraActor*            GhostCamera  = nullptr;
	UPROPERTY() AActor*    PendingProxyToDestroy       = nullptr;

	FTimerHandle SafetyHandle;

	void OnArrivalAtTarget();
	void SafetyRestoreInput(); // fires if OnHandoffComplete never fires in time
};
