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
 * UCinematicHandoffComponent
 *
 * Add this component to any Director actor (TutorialDirector, WakeUpDirector)
 * that needs to perform a smooth cutscene → gameplay camera transition.
 *
 * ──────────────────────────────────────────────────────────────────────
 * THE PROBLEM THIS SOLVES
 * ──────────────────────────────────────────────────────────────────────
 * When a Level Sequence ends:
 *   - The Camera Cut track releases control → view snaps back to the player
 *   - The player pawn is hidden at PlayerStart, far from the SK proxy
 *   - Result: a jarring 1-frame flicker
 *
 * ──────────────────────────────────────────────────────────────────────
 * HOW IT WORKS (frame timeline)
 * ──────────────────────────────────────────────────────────────────────
 *   Frame 0 – Sequence OnStop fires. Last CineCamera is still in the world.
 *   Frame 0 – Ghost ACameraActor spawned at the EXACT CineCamera world transform.
 *   Frame 0 – PC view target = Ghost (zero-time snap — invisible).
 *   Frame 0 – Player pawn teleported to PlayerSpawnTransform (still hidden).
 *   Frame 0 – Player pawn unhidden (camera is still at Ghost, player not visible).
 *   Frame 0 – SetViewTargetWithBlend(Player, BlendTime) starts.
 *   Frame N – Blend finishes. Ghost destroyed. OnHandoffComplete fires.
 *
 * ──────────────────────────────────────────────────────────────────────
 * USAGE
 * ──────────────────────────────────────────────────────────────────────
 *   1. Add this component to BP_TutorialDirector / BP_WakeUpDirector.
 *   2. Call BeginHandoff() from the C++ OnStop callback.
 *   3. Bind OnHandoffComplete to restore input, show HUD, etc.
 *
 * ──────────────────────────────────────────────────────────────────────
 * SPAWN MARKER
 * ──────────────────────────────────────────────────────────────────────
 *   Because animator SK proxies often have incorrect pivot points, the
 *   caller must supply the PlayerSpawnTransform explicitly.
 *   Use APlayerSpawnMarker placed at the SK's feet on the last frame,
 *   or supply a manually crafted FTransform.
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
	 * Duration of the camera blend from ghost → player camera.
	 * 0.0  = instant snap (use if CineCamera ends exactly at player head).
	 * 0.4–0.8 = recommended for most transitions.
	 * Set in BP_TutorialDirector Class Defaults → Cinematic Handoff.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Handoff|Config")
	float CameraBlendTime = 0.5f;

	/**
	 * Blend function for the camera transition.
	 * VTBlend_EaseInOut gives the most natural, cinematic feel.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Handoff|Config")
	TEnumAsByte<EViewTargetBlendFunction> BlendFunction =
		EViewTargetBlendFunction::VTBlend_EaseInOut;

	/**
	 * Exponent for EaseIn/EaseOut blend functions.
	 * 2.0 = smooth; higher values = sharper ease.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Handoff|Config")
	float BlendExponent = 2.0f;

	// ── EVENTS ────────────────────────────────────────────────────────────────

	/**
	 * Fires when the camera blend finishes and the player has full visual control.
	 * Bind here to: restore movement input, show HUD, enable survival, etc.
	 *
	 * In TutorialDirector → bind to OnCinematicHandoffComplete (C++ UFUNCTION).
	 * In WakeUpDirector   → bind to OnWakeUpHandoffComplete    (C++ UFUNCTION).
	 */
	UPROPERTY(BlueprintAssignable, Category = "Handoff|Events")
	FOnCinematicHandoffComplete OnHandoffComplete;

	// ── RUNTIME STATE ─────────────────────────────────────────────────────────

	/** True while the blend is in progress. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Handoff|Runtime")
	bool bHandoffInProgress = false;

	// ── PUBLIC API ────────────────────────────────────────────────────────────

	/**
	 * Begin the cutscene → gameplay camera handoff.
	 *
	 * Call this immediately from the Level Sequence OnStop callback.
	 *
	 * @param LastCineCamera       The CineCameraActor that was active on the
	 *                              sequence's last frame. Assign in the Director's
	 *                              Details panel ("Last Sequence CineCamera").
	 *                              If null, falls back to the player's camera position.
	 *
	 * @param Player               The hidden BP_Player pawn.
	 *
	 * @param PC                   The owning PlayerController.
	 *
	 * @param PlayerSpawnTransform World transform where the player pawn should
	 *                              appear. Use APlayerSpawnMarker::GetSpawnTransform()
	 *                              or supply the proxy's root transform manually.
	 */
	UFUNCTION(BlueprintCallable, Category = "Handoff")
	void BeginHandoff(
		AActor*                  LastCineCamera,
		AAlphaExilemetCharacter* Player,
		APlayerController*       PC,
		FTransform               PlayerSpawnTransform);

	/**
	 * Abort a running handoff (e.g. if the level is about to change).
	 * OnHandoffComplete will NOT fire after Cancel.
	 */
	UFUNCTION(BlueprintCallable, Category = "Handoff")
	void CancelHandoff();

private:
	UPROPERTY()
	ACameraActor* GhostCamera = nullptr;

	UPROPERTY()
	APlayerController* CachedPC = nullptr;

	FTimerHandle BlendCompleteHandle;

	void OnBlendComplete();
};
