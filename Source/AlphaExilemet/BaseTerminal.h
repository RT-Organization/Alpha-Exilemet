#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "AlphaExilemet/Interfaces/Interactable.h"
#include "BaseTerminal.generated.h"

class UBoxComponent;

// ─────────────────────────────────────────────────────────────────────────────
// ABaseTerminal  v4
//
// Changes from v3:
//
//   WIDGET TIMING FIX:
//     BP_OnTerminalClosingStarted — NEW event, fires IMMEDIATELY when the
//       player presses F to close (before camera blends back).
//       Wire here: Remove ActiveWidget, SET ActiveWidget/ActiveTerminal = None.
//       Widget disappears instantly on keypress. ✓
//
//     BP_OnTerminalClosed — now fires AFTER blend completes (unchanged timing).
//       Wire here ONLY: SetPlayerHUD Visible, Set Input Mode Game Only.
//       Input is NOT restored until camera is fully back. ✓
//
//     BP_OnTerminalViewReady — unchanged (fires after blend-IN, shows widget). ✓
//
//   RESULT: Widget appears after blend-in, disappears on keypress,
//           input restored only after blend-out. Clean separation.
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

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Terminal")
	UBoxComponent* InteractionBox;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terminal")
	bool bUseMeshForInteraction;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Terminal")
	UStaticMeshComponent* TerminalMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Terminal|Camera")
	class UCameraComponent* TerminalCamera;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terminal|Camera")
	float CameraBlendTime = 0.5f;

	// ── INTERACTION LOCK ──────────────────────────────────────────────────────

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Terminal")
	bool bIsInteracting = false;

	// ── CONFIRMATION LOCK ─────────────────────────────────────────────────────

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Terminal|Confirmation")
	bool bRequiresConfirmation = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Terminal|Confirmation")
	bool bCloseBlocked = false;

	// ── INTERACTABLE INTERFACE ────────────────────────────────────────────────

	virtual bool CanBeInteractedWith_Implementation() const override;
	virtual void Interact_Implementation(class AAlphaExilemetCharacter* Interactor) override;

	// ── PUBLIC INTERFACE ──────────────────────────────────────────────────────

	UFUNCTION(BlueprintCallable, Category = "Terminal|Interaction")
	void TryStopTerminalInteraction(class AAlphaExilemetCharacter* Interactor);

	UFUNCTION(BlueprintCallable, Category = "Terminal|Confirmation")
	void BlockTerminalClose();

	UFUNCTION(BlueprintCallable, Category = "Terminal|Confirmation")
	void ConfirmAndCloseTerminal(class AAlphaExilemetCharacter* Interactor);

	UFUNCTION(BlueprintCallable, Category = "Terminal|Interaction")
	void StopTerminalInteraction(class AAlphaExilemetCharacter* Interactor);

	virtual void OnConstruction(const FTransform& Transform) override;

	// ── BLUEPRINT IMPLEMENTABLE EVENTS ────────────────────────────────────────

	/** Camera blend-IN complete — create and show widget here. */
	UFUNCTION(BlueprintImplementableEvent, Category = "Terminal|Events")
	void BP_OnTerminalViewReady(class AAlphaExilemetCharacter* Interactor);

	/**
	 * Fires IMMEDIATELY when the player presses F to close (before camera moves).
	 * Wire here in BP_BaseTerminal:
	 *   → Is Valid (ActiveWidget) → Remove from Parent → SET ActiveWidget = None
	 *   → SET Active Terminal = None (via PlayerRef)
	 * Widget vanishes instantly on keypress.
	 */
	UFUNCTION(BlueprintImplementableEvent, Category = "Terminal|Events")
	void BP_OnTerminalClosingStarted();

	/**
	 * Camera blend-OUT complete — player has full control.
	 * Wire here in BP_BaseTerminal:
	 *   → SetPlayerHUD Macro (Visible)
	 *   → Set Input Mode Game Only
	 * Input is NOT restored until camera fully returns.
	 */
	UFUNCTION(BlueprintImplementableEvent, Category = "Terminal|Events")
	void BP_OnTerminalClosed();

	/** Player tried to close while bCloseBlocked = true. Play denied SFX here. */
	UFUNCTION(BlueprintImplementableEvent, Category = "Terminal|Events")
	void BP_OnClosureBlocked();

	/**
	 * OK confirmed — fires BEFORE camera blends back.
	 * Wire: GET ShipRef → Is Valid → StopAlarm
	 */
	UFUNCTION(BlueprintImplementableEvent, Category = "Terminal|Events")
	void BP_OnConfirmationReceived();
};