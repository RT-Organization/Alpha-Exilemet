#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "AlphaExilemet/Interfaces/Interactable.h"
#include "BaseTerminal.generated.h"

class UBoxComponent;

// ─────────────────────────────────────────────────────────────────────────────
// ABaseTerminal  v2
//
// Changes from v1:
//   - bIsInteracting flag added.
//     Set true when Interact_Implementation fires (camera blend starts).
//     Set false when RestoreInput fires (camera fully returned to player).
//     Prevents input-spam interaction while the camera is animating.
//
//   - CanBeInteractedWith_Implementation() overridden → returns !bIsInteracting.
//     The character's Tick() checks this, so the prompt disappears during the
//     blend and the player cannot trigger a second interaction mid-animation.
// ─────────────────────────────────────────────────────────────────────────────

UCLASS()
class ALPHAEXILEMET_API ABaseTerminal : public AActor, public IInteractable
{
	GENERATED_BODY()
	
public:
	ABaseTerminal();
	
protected:
	virtual void BeginPlay() override;
	
	FTimerHandle CameraBlendTimerHandle;
	FTimerHandle StopBlendTimerHandle;

	void OnBlendComplete();
	void RestoreInput();

	UPROPERTY()
	class AAlphaExilemetCharacter* CurrentInteractor;
	
public:
	// ── COMPONENTS ────────────────────────────────────────────────────────────

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Terminal")
	UBoxComponent* InteractionBox;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Terminal")
	bool bUseMeshForInteraction;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Terminal")
	UStaticMeshComponent* TerminalMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Terminal|Camera")
	class UCameraComponent* TerminalCamera;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Terminal|Camera")
	float CameraBlendTime = 0.5f;

	// ── INTERACTION LOCK ──────────────────────────────────────────────────────

	/**
	 * True while the camera is blending in or out.
	 * CanBeInteractedWith() returns false during this time, hiding the
	 * interaction prompt and blocking double-interaction bugs.
	 *
	 * Set true by Interact_Implementation (blend-in starts).
	 * Set false by RestoreInput (blend-out finishes, player has full control).
	 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Terminal")
	bool bIsInteracting = false;

	// ── INTERACTABLE INTERFACE ────────────────────────────────────────────────

	/** Returns false while bIsInteracting — hides prompt during camera blend. */
	virtual bool CanBeInteractedWith_Implementation() const override;

	virtual void Interact_Implementation(class AAlphaExilemetCharacter* Interactor) override;

	// ── BLUEPRINT EVENTS ──────────────────────────────────────────────────────

	/** Called when camera finishes blending to the terminal view. Show UI here. */
	UFUNCTION(BlueprintImplementableEvent, Category="Terminal|Events")
	void BP_OnTerminalViewReady(class AAlphaExilemetCharacter* Interactor);

	// ── PUBLIC INTERFACE ──────────────────────────────────────────────────────

	/**
	 * Call from BP when the player closes the terminal UI.
	 * Starts the camera blend back to the player and re-enables input
	 * after the blend finishes.
	 */
	UFUNCTION(BlueprintCallable, Category="Terminal|Interaction")
	void StopTerminalInteraction(class AAlphaExilemetCharacter* Interactor);

	virtual void OnConstruction(const FTransform& Transform) override;
};
