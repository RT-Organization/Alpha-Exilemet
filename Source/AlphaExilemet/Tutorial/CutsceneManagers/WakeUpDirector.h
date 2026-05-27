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
// AWakeUpDirector  v4 — identical pattern to TutorialDirector v18
//
// - No Actor Tags, no Sequencer Binding Tags.
// - Proxy found via GetBoundObjects() + ProxyMeshNameHint.
// - OnStop deferred by one tick to let Sequencer finish teardown.
// - Black bars cleared by CinematicHandoffComponent.
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
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "WakeUp|Components")
	UCinematicHandoffComponent* CinematicHandoff;

	// ── INSTANCE REFERENCES ───────────────────────────────────────────────────

	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "WakeUp|Config",
		meta = (DisplayName = "Wake Up Sequence"))
	ALevelSequenceActor* WakeUpSequenceRef = nullptr;

	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "WakeUp|Config",
		meta = (DisplayName = "Pre-Sequence Cinecam"))
	AActor* WakeUpCineCamRef = nullptr;

	// ── CLASS DEFAULTS ────────────────────────────────────────────────────────

	/**
	 * Partial name used to find the SK_Manny Spawnable via GetBoundObjects().
	 * No tags required. Leave empty to use the first bound SK actor found.
	 * Default: "SKM_Manny"
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "WakeUp|Config",
		meta = (DisplayName = "Proxy Mesh Name Hint"))
	FString ProxyMeshNameHint = TEXT("SKM_Manny");

	/** Head bone — ghost camera travels here. Standard Manny: "head" */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "WakeUp|Config|Bones",
		meta = (DisplayName = "Head Bone Name"))
	FName HeadBoneName = FName("head");

	/** Root bone — player pawn placed here. Standard Manny: "root" */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "WakeUp|Config|Bones",
		meta = (DisplayName = "Root Bone Name"))
	FName RootBoneName = FName("root");

	// ── RUNTIME STATE ─────────────────────────────────────────────────────────

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "WakeUp|Runtime")
	ULevelSequencePlayer* WakeUpSequencePlayer = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "WakeUp|Runtime")
	AAlphaExilemetCharacter* CachedPlayer = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "WakeUp|Runtime")
	APlayerController* CachedPC = nullptr;

	// ── PUBLIC INTERFACE ──────────────────────────────────────────────────────

	UFUNCTION(BlueprintCallable, Category = "WakeUp")
	void InitializeWakeUp();

protected:
	UFUNCTION(BlueprintImplementableEvent, Category = "WakeUp|Events")
	void BP_RegisterWithGameMode();

	UFUNCTION(BlueprintImplementableEvent, Category = "WakeUp|Events")
	void BP_OnWakeUpComplete();

private:
	UFUNCTION() void OnWakeUpSequenceFinished();
	void             OnWakeUpSequenceFinishedDeferred();
	UFUNCTION() void OnWakeUpHandoffComplete();

	USkeletalMeshComponent* FindProxyMeshInSequence(AActor*& OutProxyActor) const;

	FTimerHandle DeferredSequenceEndHandle;
};
