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
class UCinematicHandoffComponent;
class ACameraActor;
class USkeletalMeshComponent;

// ─────────────────────────────────────────────────────────────────────────────
// ATutorialDirector  v18
//
// Key changes from v17:
//
//  BUG FIX 1 — CineCamera reference resets to null on save.
//    Root cause: Sequencer Spawnables are transient; they don't exist as
//    persistent level actors. A soft-object reference to them is null at
//    edit-time so the slot always saves as null.
//    Fix: LastTutorialCineCamera removed from instance Details.
//    At runtime, OnIntroSequenceFinished() reads the active view target from
//    PC->GetViewTarget() the frame OnStop fires — this is always the last
//    sequence camera and never requires a manual assignment.
//
//  BUG FIX 2 — Player spawns at PlayerStart (proxy not found by tag).
//    Root cause: Sequencer's right-click → Tags menu sets "Object Binding Tags"
//    on the Sequencer binding, NOT AActor::Tags. GetAllActorsWithTag() searches
//    AActor::Tags and finds nothing.
//    Fix: we iterate IntroSequenceRef->GetBoundObjects() directly — this gives
//    us every actor bound to the sequence at runtime with no tag dependency.
//    We identify the proxy SK by checking for a USkeletalMeshComponent and
//    an optional name hint (ProxyMeshNameHint).
//
//  BUG FIX 3 — Black bars after sequence (bConstrainAspectRatio not cleared).
//    Fix: CinematicHandoffComponent now explicitly clears bConstrainAspectRatio
//    on the FP camera before and after the view-target switch.
//
//  BUG FIX 4 — Can't move after sequence (Sequencer cinematic mode still active).
//    Fix: OnIntroSequenceFinished defers its logic by one tick via a 0-second
//    timer so Sequencer finishes its internal teardown first.
//
//  BUG FIX 5 — Safety timer added to CinematicHandoffComponent.
//    If the handoff never completes for any reason, input is force-restored
//    after SafetyInputRestoreDelay seconds.
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
	// ═════════════════════════════════════════════════════════════════════════
	// COMPONENTS
	// ═════════════════════════════════════════════════════════════════════════

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Tutorial|Components")
	UCinematicHandoffComponent* CinematicHandoff;

	// ═════════════════════════════════════════════════════════════════════════
	// INSTANCE REFERENCES  (assign in the placed actor's Details panel)
	// ═════════════════════════════════════════════════════════════════════════

	/**
	 * The LevelSequenceActor for the intro cutscene.
	 * Drag LS_Tutorial_2 from the Outliner into this slot.
	 */
	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Tutorial|Config",
		meta = (DisplayName = "Intro Sequence"))
	ALevelSequenceActor* IntroSequenceRef = nullptr;

	/**
	 * The persistent (level-placed) ship actor.
	 * Hidden by default. Revealed by C++ when the sequence ends to replace
	 * the disappearing SHIP_1 Spawnable.
	 *
	 * Optional: leave null if no ship needs to persist.
	 */
	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Tutorial|Config",
		meta = (DisplayName = "Persistent Ship Actor"))
	AActor* PersistentShipActor = nullptr;

	/**
	 * Optional cinecam to snap to BEFORE the sequence plays, preventing
	 * a 1-frame first-person flash on the first frame.
	 * Leave null if the Camera Cuts track handles the first frame.
	 */
	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Tutorial|Config",
		meta = (DisplayName = "Pre-Sequence Cinecam"))
	AActor* TutorialCineCamRef = nullptr;

	// ═════════════════════════════════════════════════════════════════════════
	// CLASS DEFAULTS  (set once in BP_TutorialDirector → Class Defaults)
	// ═════════════════════════════════════════════════════════════════════════

	/** BP_Pickaxe class spawned after the handoff. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Tutorial|Config")
	TSubclassOf<AToolBase> TutorialPickaxeClass;

	/**
	 * Partial name used to identify the SKM_Manny proxy among all bound objects.
	 * The C++ checks if the bound actor's name CONTAINS this string
	 * (case-insensitive). Works without any tag setup in Sequencer.
	 *
	 * Examples that work:
	 *   "SKM_Manny"   — matches SKM_Manny_0, SKM_Manny_1, etc.
	 *   "Manny"       — matches any actor whose name contains "Manny"
	 *   ""            — disabled; first bound SK mesh found is used
	 *
	 * Set in BP_TutorialDirector Class Defaults → Tutorial|Config.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Tutorial|Config",
		meta = (DisplayName = "Proxy Mesh Name Hint"))
	FString ProxyMeshNameHint = TEXT("SKM_Manny");

	/**
	 * Partial name used to identify the SHIP_1 Spawnable among bound objects.
	 * Used to copy the ship's final transform to PersistentShipActor.
	 * Leave empty to skip transform copy (PersistentShipActor stays in place).
	 *
	 * Examples: "SHIP_1", "CutsceneShip", "Ship"
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Tutorial|Config",
		meta = (DisplayName = "Cutscene Ship Name Hint"))
	FString CutsceneShipNameHint = TEXT("SHIP_1");

	/**
	 * Name of the head bone in the proxy SK.
	 * Ghost camera travels to this bone's world position.
	 * Standard UE Manny: "head"
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Tutorial|Config|Bones",
		meta = (DisplayName = "Head Bone Name"))
	FName HeadBoneName = FName("head");

	/**
	 * Name of the root/feet bone in the proxy SK.
	 * BP_Player is placed at this bone's world position.
	 * Standard UE Manny: "root"
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Tutorial|Config|Bones",
		meta = (DisplayName = "Root Bone Name"))
	FName RootBoneName = FName("root");

	// ═════════════════════════════════════════════════════════════════════════
	// RUNTIME STATE
	// ═════════════════════════════════════════════════════════════════════════

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Tutorial|Runtime")
	ULevelSequencePlayer* IntroSequencePlayer = nullptr;

	/**
	 * Player's world transform saved after the handoff completes.
	 * Stores BOTH position AND facing direction.
	 * The OxygenSphere boundary guard teleports the player back here
	 * (position + rotation both restored — fixes the old "wrong facing" bug).
	 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Tutorial|Runtime")
	FTransform CraterStartTransform;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Tutorial|Runtime")
	AAlphaExilemetCharacter* CachedPlayer = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Tutorial|Runtime")
	APlayerController* CachedPC = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Tutorial|Skull")
	ASkullProp* SkullRef = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Tutorial|Skull")
	bool bSkullInteractionActive = false;

	// ═════════════════════════════════════════════════════════════════════════
	// PUBLIC INTERFACE
	// ═════════════════════════════════════════════════════════════════════════

	UFUNCTION(BlueprintCallable, Category = "Tutorial")
	void InitializeTutorial();

	UFUNCTION(BlueprintCallable, Category = "Tutorial|Skull")
	void OnSkullInteracted();

	UFUNCTION(BlueprintCallable, Category = "Tutorial")
	void ClearTutorialPickaxe();

protected:
	UFUNCTION(BlueprintImplementableEvent, Category = "Tutorial|Events")
	void BP_RegisterWithGameMode();

	UFUNCTION(BlueprintImplementableEvent, Category = "Tutorial|Events")
	void BP_HideHUD();

	UFUNCTION(BlueprintImplementableEvent, Category = "Tutorial|Events")
	void BP_OnIntroFinished();

	UFUNCTION(BlueprintImplementableEvent, Category = "Tutorial|Skull")
	void BP_PlayMagicSpellSound();

	UFUNCTION(BlueprintImplementableEvent, Category = "Tutorial|Skull")
	void BP_ShowInstantBlack();

	UFUNCTION(BlueprintImplementableEvent, Category = "Tutorial|Skull")
	void BP_PlayExplosionSequence();

private:
	void HidePlayerForCutscene();

	UFUNCTION() void OnIntroSequenceFinished();

	// Deferred one-tick after OnStop to let Sequencer finish teardown.
	void OnIntroSequenceFinishedDeferred();

	UFUNCTION() void OnCinematicHandoffComplete();

	// Bound-objects proxy search (replaces GetAllActorsWithTag).
	USkeletalMeshComponent* FindProxyMeshInSequence(AActor*& OutProxyActor) const;
	AActor*                 FindShipInSequence() const;

	UFUNCTION()
	void OnOxygenSphereEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);

	void ExecuteTeleportToCrater();
	void OnTeleportReadyToMove();
	void OnTeleportComplete();

	void OnSpellDurationComplete();
	void OnSkullPausedBeforeBlack();
	void OnPostBlackDelay();
	void OnLevelSwapReady();

	void SuppressPlayerMoveInput();
	void RestorePlayerMoveInput();
	void LockPlayerInputFull();

	AActor* FindBaseCamp() const;

	FTimerHandle DeferredSequenceEndHandle;
	FTimerHandle TeleportFadeOutHandle;
	FTimerHandle TeleportFadeInHandle;
	FTimerHandle SpellDurationHandle;
	FTimerHandle SkullPauseHandle;
	FTimerHandle PostBlackSoundHandle;
	FTimerHandle LevelSwapHandle;

	UPROPERTY() AToolBase* SpawnedTutorialPickaxe = nullptr;
};
