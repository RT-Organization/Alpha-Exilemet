#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "CinematicHandoffComponent.generated.h"

class AAlphaExilemetCharacter;
class APlayerController;
class ACameraActor;

// ─────────────────────────────────────────────────────────────────────────────
// DELEGATE
// ─────────────────────────────────────────────────────────────────────────────
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnCinematicHandoffComplete);

/**
 * UCinematicHandoffComponent  v2
 *
 * Handles the cutscene → gameplay camera transition.
 *
 * ──────────────────────────────────────────────────────────────────────
 * WHAT IT DOES (frame timeline)
 * ──────────────────────────────────────────────────────────────────────
 *
 *  Frame 0  – Sequence OnStop fires. Last CineCamera is at its final position.
 *  Frame 0  – Ghost ACameraActor spawned at LastCineCamera's EXACT world transform.
 *  Frame 0  – PC view snaps to Ghost (zero time — image is identical to last seq frame).
 *
 *  Frames 1…N  – Ghost camera SMOOTHLY TRAVELS (ticked every frame) from the
 *                LastCineCamera position toward the SK proxy's HEAD BONE world position.
 *                Player pawn is still hidden at PlayerStart — nobody sees it.
 *
 *  Frame N  – Ghost has arrived at the head bone.
 *  Frame N  – BP_Player is teleported to the SK proxy's ROOT BONE position (still hidden).
 *  Frame N  – BP_Player is unhidden (camera is AT the head — no visible pop).
 *  Frame N  – PC view target snapped to BP_Player (instant — ghost and player FP cam
 *             are at the same world location → seamless).
 *  Frame N  – Ghost destroyed. OnHandoffComplete fires.
 *
 * ──────────────────────────────────────────────────────────────────────
 * WHY BONE POSITIONS (not actor location / PlayerSpawnMarker)
 * ──────────────────────────────────────────────────────────────────────
 *  The animator's Spawnable SK often has an incorrect pivot (actor origin ≠
 *  character root). Using bone queries solves this completely:
 *    GetBoneLocation("head")  → exact world position of the head joint
 *    GetBoneLocation("root")  → exact world position of the foot joint
 *  Both are independent of the actor's pivot offset.
 *
 * ──────────────────────────────────────────────────────────────────────
 * USAGE
 * ──────────────────────────────────────────────────────────────────────
 *  1. Component is created in TutorialDirector / WakeUpDirector constructor.
 *  2. Director calls BeginHandoff() from its OnSequenceFinished callback.
 *  3. Bind OnHandoffComplete to restore input, show HUD, spawn pickaxe, etc.
 *
 * ──────────────────────────────────────────────────────────────────────
 * SEQUENCER REQUIREMENT (one-time per sequence)
 * ──────────────────────────────────────────────────────────────────────
 *  The SK_Manny Spawnable track MUST have "When Finished = Keep State".
 *  This keeps the actor alive when OnStop fires so we can query bone positions.
 *  After BeginHandoff captures the positions, it destroys the SK itself.
 */
UCLASS(ClassGroup=(AlphaExilemet), meta=(BlueprintSpawnableComponent),
       DisplayName = "Cinematic Handoff")
class ALPHAEXILEMET_API UCinematicHandoffComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UCinematicHandoffComponent();

	// ── CONFIGURATION ─────────────────────────────────────────────────────────

	/**
	 * Total seconds for the ghost camera to travel from the last CineCamera
	 * position to the SK proxy's head bone.
	 *
	 * Recommended values:
	 *   0.8 – 1.5s  general travel
	 *   0.3 – 0.6s  if the sequence camera already ends close to the head
	 *
	 * Set in the Director's Class Defaults → Cinematic Handoff → Camera Travel Time.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Handoff|Config",
		meta = (ClampMin = "0.1", UIMin = "0.1"))
	float CameraBlendTime = 1.0f;

	/**
	 * Optional extra blend from ghost → player FP cam at the very end.
	 *
	 * Keep at 0 when the sequence camera ends close to the head (seamless snap).
	 * Set to 0.1 – 0.2 only if there is a visible pop at the moment of player
	 * appearance (usually caused by a large discrepancy between the ghost's
	 * final position and the player FP camera socket position).
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Handoff|Config",
		meta = (ClampMin = "0.0", UIMin = "0.0"))
	float FinalSnapBlendTime = 0.0f;

	// ── EVENTS ────────────────────────────────────────────────────────────────

	/**
	 * Fires when the ghost camera reaches the head bone AND the player has
	 * been placed and given camera control.
	 *
	 * Bind here to: restore movement input, show HUD, spawn pickaxe, enable
	 * survival, wire boundary guard, etc.
	 */
	UPROPERTY(BlueprintAssignable, Category = "Handoff|Events")
	FOnCinematicHandoffComplete OnHandoffComplete;

	// ── RUNTIME STATE ─────────────────────────────────────────────────────────

	/** True while the ghost is travelling or the final blend is in progress. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Handoff|Runtime")
	bool bHandoffInProgress = false;

	// ── PUBLIC API ────────────────────────────────────────────────────────────

	/**
	 * Begin the cutscene → gameplay handoff.
	 *
	 * Call immediately from the Level Sequence's OnStop callback.
	 *
	 * @param Player               The hidden BP_Player pawn.
	 * @param PC                   The owning PlayerController.
	 * @param GhostStartLocation   World location of the last-active CineCamera.
	 *                             Pass LastTutorialCineCamera->GetActorLocation().
	 * @param GhostStartRotation   World rotation of the last-active CineCamera.
	 * @param CameraTargetLocation World location to travel toward (SK head bone).
	 *                             Computed from SkMesh->GetBoneLocation(HeadBoneName).
	 * @param PlayerSpawnTransform FTransform at which to place BP_Player.
	 *                             Computed from root bone location + proxy yaw.
	 *                             Player is teleported here when the ghost arrives
	 *                             at CameraTargetLocation — still hidden, no pop.
	 * @param ProxyActorToDestroy  Optional: the SK proxy actor. If valid, it is
	 *                             destroyed after player placement so the two
	 *                             meshes never overlap visually.
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

	/** Abort. OnHandoffComplete will NOT fire. */
	UFUNCTION(BlueprintCallable, Category = "Handoff")
	void CancelHandoff();

	// Tick-enabled conditionally (only while bHandoffInProgress).
	virtual void TickComponent(
		float DeltaTime,
		ELevelTick TickType,
		FActorComponentTickFunction* ThisTickFunction) override;

private:
	// Travel state
	FVector  TravelStartPos;
	FVector  TravelEndPos;
	FQuat    TravelStartQuat;
	FQuat    TravelEndQuat;
	float    TravelElapsed = 0.0f;

	FTransform PendingPlayerSpawn;

	UPROPERTY()
	AAlphaExilemetCharacter* CachedPlayer = nullptr;

	UPROPERTY()
	APlayerController* CachedPC = nullptr;

	UPROPERTY()
	ACameraActor* GhostCamera = nullptr;

	UPROPERTY()
	AActor* PendingProxyToDestroy = nullptr;

	// Called when travel alpha reaches 1.0
	void OnArrivalAtTarget();
};
