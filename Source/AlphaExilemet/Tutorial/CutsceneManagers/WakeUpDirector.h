#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "WakeUpDirector.generated.h"

class ULevelSequencePlayer;
class ALevelSequenceActor;
class AAlphaExilemetCharacter;
class APlayerController;
class UCinematicHandoffComponent;
class ACameraActor;

// ─────────────────────────────────────────────────────────────────────────────
// AWakeUpDirector  v3
//
// Changes from v2:
//   - PlayerSpawnMarker REMOVED. Uses bone-based positioning (same as TutorialDirector v17).
//   - ProxySkeletonTag + HeadBoneName + RootBoneName added.
//   - CinematicHandoffComponent now travels ghost FROM last CineCamera TO head bone.
//   - LastWakeUpCineCamera auto-detected via PC->GetViewTarget() if not assigned.
// ─────────────────────────────────────────────────────────────────────────────

UCLASS(Abstract, Blueprintable)
class ALPHAEXILEMET_API AWakeUpDirector : public AActor
{
	GENERATED_BODY()

public:
	AWakeUpDirector();

protected:
	virtual void BeginPlay() override;

public:
	// ═════════════════════════════════════════════════════════════════════════
	// COMPONENTS
	// ═════════════════════════════════════════════════════════════════════════

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "WakeUp|Components")
	UCinematicHandoffComponent* CinematicHandoff;

	// ═════════════════════════════════════════════════════════════════════════
	// INSTANCE REFERENCES  (assign in placed Details panel)
	// ═════════════════════════════════════════════════════════════════════════

	/** The LevelSequenceActor for the wake-up cutscene in the Main level. */
	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "WakeUp|Config",
		meta = (DisplayName = "Wake Up Sequence"))
	ALevelSequenceActor* WakeUpSequenceRef = nullptr;

	/**
	 * The CineCameraActor active on the wake-up sequence's LAST FRAME.
	 * If null, auto-detected from PC->GetViewTarget() when OnStop fires.
	 */
	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "WakeUp|Config",
		meta = (DisplayName = "Last Sequence CineCamera"))
	ACameraActor* LastWakeUpCineCamera = nullptr;

	/**
	 * Optional cinecam to snap to BEFORE the sequence plays.
	 * Leave null if the Camera Cuts track handles the first frame.
	 */
	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "WakeUp|Config",
		meta = (DisplayName = "Pre-Sequence Cinecam"))
	AActor* WakeUpCineCamRef = nullptr;

	// ═════════════════════════════════════════════════════════════════════════
	// CLASS DEFAULTS  (set once in BP_WakeUpDirector Class Defaults)
	// ═════════════════════════════════════════════════════════════════════════

	/**
	 * Actor tag on the SK_Manny Spawnable in the wake-up sequence.
	 *
	 * REQUIRED SETUP (one-time):
	 *   1. In the wake-up LevelSequence, select the SK_Manny track.
	 *   2. Details → Actor Tags → add this tag (e.g. "SKM_WakeUp").
	 *   3. Set "When Finished" to "Keep State" on that track.
	 *
	 * Default: "SKM_WakeUp"
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "WakeUp|Config|Tags",
		meta = (DisplayName = "Proxy Skeleton Tag"))
	FName ProxySkeletonTag = FName("SKM_WakeUp");

	/**
	 * Name of the head bone. Ghost camera travels to this bone's world position.
	 * Standard Manny: "head"
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "WakeUp|Config|Bones",
		meta = (DisplayName = "Head Bone Name"))
	FName HeadBoneName = FName("head");

	/**
	 * Name of the root/feet bone. Player pawn is placed at this bone's world position.
	 * Standard Manny: "root"
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "WakeUp|Config|Bones",
		meta = (DisplayName = "Root Bone Name"))
	FName RootBoneName = FName("root");

	// ═════════════════════════════════════════════════════════════════════════
	// RUNTIME STATE
	// ═════════════════════════════════════════════════════════════════════════

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "WakeUp|Runtime")
	ULevelSequencePlayer* WakeUpSequencePlayer = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "WakeUp|Runtime")
	AAlphaExilemetCharacter* CachedPlayer = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "WakeUp|Runtime")
	APlayerController* CachedPC = nullptr;

	// ═════════════════════════════════════════════════════════════════════════
	// PUBLIC INTERFACE
	// ═════════════════════════════════════════════════════════════════════════

	/**
	 * Called by GM_SimulatorGamemode after the Tutorial→Main transition.
	 * GM BP: [WakeUpDirectorRef → Is Valid] → [InitializeWakeUp]
	 */
	UFUNCTION(BlueprintCallable, Category = "WakeUp")
	void InitializeWakeUp();

protected:
	// ═════════════════════════════════════════════════════════════════════════
	// BLUEPRINT IMPLEMENTABLE EVENTS
	// ═════════════════════════════════════════════════════════════════════════

	/** Push self-reference to GM. Called at end of C++ BeginPlay. */
	UFUNCTION(BlueprintImplementableEvent, Category = "WakeUp|Events")
	void BP_RegisterWithGameMode();

	/**
	 * Called after camera handoff blend completes.
	 * Show HUD, play ambient audio, trigger ship terminal warning, etc.
	 * Survival is already ON when this fires.
	 */
	UFUNCTION(BlueprintImplementableEvent, Category = "WakeUp|Events")
	void BP_OnWakeUpComplete();

private:
	UFUNCTION()
	void OnWakeUpSequenceFinished();

	UFUNCTION()
	void OnWakeUpHandoffComplete();
};
