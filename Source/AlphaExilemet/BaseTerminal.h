#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "AlphaExilemet/Interfaces/Interactable.h"
#include "BaseTerminal.generated.h"

class UBoxComponent;

// ─────────────────────────────────────────────────────────────────────────────
// ABaseTerminal  v3
//
// Changes from v2:
//
//   CLOSE-LOCK SYSTEM:
//     bCloseBlocked (bool) — set to true by BlockTerminalClose().
//     TryStopTerminalInteraction() — the new close entry point called by BP_Player.
//       • If bCloseBlocked → fires BP_OnClosureBlocked (denied SFX etc.), does nothing else.
//       • If not blocked   → calls StopTerminalInteraction normally.
//     BlockTerminalClose()         — called by WBP_ShipTerminal after showing warning.
//     ConfirmAndCloseTerminal()    — called by WB_ShipRepairWarning OK button.
//       • Clears bCloseBlocked.
//       • Fires BP_OnConfirmationReceived (stop alarm in BP_ShipTerminal).
//       • Starts the camera blend back (StopTerminalInteraction).
//
//   BP_OnTerminalClosed (new BlueprintImplementableEvent):
//     Fires in RestoreInput() — camera has FULLY returned, player has control.
//     Implement in BP_BaseTerminal EventGraph to:
//       • Remove Active Widget from parent.
//       • SET Active Terminal = None on BP_Player.
//       • Show Player HUD.
//       • Set Input Mode Game Only.
//     This REPLACES the old CloseTerminal custom event logic entirely.
//
//   BP_Player INTERACT changes (see guide):
//     OLD: Is Valid(ActiveTerminal) → True → CloseTerminal (custom event on terminal)
//     NEW: Is Valid(ActiveTerminal) → True → TryStopTerminalInteraction(Self)
//     The CloseTerminal custom event in BP_BaseTerminal can be DELETED.
//
// WHAT DOES NOT CHANGE (backward compat):
//   • Interact_Implementation — identical.
//   • StopTerminalInteraction — still exists and is the real camera-blend function.
//   • BP_OnTerminalViewReady — identical.
//   • OpenTerminalPanel function — no change needed (but see guide for TerminalRef).
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
	// ═════════════════════════════════════════════════════════════════════════
	// COMPONENTS
	// ═════════════════════════════════════════════════════════════════════════

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

	// ═════════════════════════════════════════════════════════════════════════
	// INTERACTION LOCK
	// ═════════════════════════════════════════════════════════════════════════

	/**
	 * True while the camera is blending in or out.
	 * CanBeInteractedWith() returns false → interaction prompt hidden.
	 * Set true on Interact_Implementation, false on RestoreInput.
	 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Terminal")
	bool bIsInteracting = false;

	// ═════════════════════════════════════════════════════════════════════════
	// CONFIRMATION LOCK
	// ═════════════════════════════════════════════════════════════════════════

	/**
	 * Set TRUE in BP_ShipTerminal Class Defaults (the only terminal that
	 * requires a confirmation before closing).
	 * All other terminal subclasses leave this false — they close normally.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Terminal|Confirmation")
	bool bRequiresConfirmation = false;

	/**
	 * Runtime flag — true while the terminal is waiting for the player to
	 * click OK on the warning widget.
	 * Set by BlockTerminalClose().
	 * Cleared by ConfirmAndCloseTerminal().
	 * Reset to false every time the camera blend completes (OnBlendComplete)
	 * so a fresh open always starts unlocked.
	 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Terminal|Confirmation")
	bool bCloseBlocked = false;

	// ═════════════════════════════════════════════════════════════════════════
	// INTERACTABLE INTERFACE
	// ═════════════════════════════════════════════════════════════════════════

	virtual bool CanBeInteractedWith_Implementation() const override;
	virtual void Interact_Implementation(class AAlphaExilemetCharacter* Interactor) override;

	// ═════════════════════════════════════════════════════════════════════════
	// PUBLIC INTERFACE
	// ═════════════════════════════════════════════════════════════════════════

	/**
	 * THE ONLY close function that should be called from BP_Player Interact.
	 *
	 * If bRequiresConfirmation = false  →  closes normally (unchanged behavior).
	 * If bRequiresConfirmation = true AND bCloseBlocked = true
	 *     →  does NOT close; fires BP_OnClosureBlocked.
	 * If bRequiresConfirmation = true AND bCloseBlocked = false
	 *     →  closes normally (confirmation was already given).
	 *
	 * BP_Player wiring (replace old CloseTerminal call):
	 *   Is Valid (ActiveTerminal)
	 *     → True → TryStopTerminalInteraction (Target = ActiveTerminal, Interactor = Self)
	 */
	UFUNCTION(BlueprintCallable, Category = "Terminal|Interaction")
	void TryStopTerminalInteraction(class AAlphaExilemetCharacter* Interactor);

	/**
	 * Called by WBP_ShipTerminal::CheckIfNewGame after showing the warning widget.
	 * Locks the terminal so TryStopTerminalInteraction is blocked.
	 *
	 * WBP_ShipTerminal wiring (in CheckIfNewGame, after Add to Viewport):
	 *   GET TerminalRef → Cast to ABaseTerminal → BlockTerminalClose
	 */
	UFUNCTION(BlueprintCallable, Category = "Terminal|Confirmation")
	void BlockTerminalClose();

	/**
	 * Called by WB_ShipRepairWarning OK button.
	 * Order of operations (all in one frame, before the blend timer):
	 *   1. Clears bCloseBlocked.
	 *   2. Fires BP_OnConfirmationReceived (stop alarm in BP_ShipTerminal).
	 *   3. Starts the camera blend back to the player.
	 *
	 * WB_ShipRepairWarning OK button wiring:
	 *   → Save Player Data (Game Instance)
	 *   → Remove from Parent (self — warning widget only)
	 *   → Get Owning Player Pawn → Cast to BP_Player → GET ActiveTerminal
	 *   → Cast to ABaseTerminal
	 *   → Get Owning Player Pawn → Cast to AAlphaExilemetCharacter (= Interactor)
	 *   → ConfirmAndCloseTerminal (Target = terminal, Interactor = character)
	 */
	UFUNCTION(BlueprintCallable, Category = "Terminal|Confirmation")
	void ConfirmAndCloseTerminal(class AAlphaExilemetCharacter* Interactor);

	/**
	 * Direct / programmatic close — bypasses ALL locks.
	 * Use ONLY for: level transitions, player death, emergency cleanup.
	 * Do NOT wire this to any UI button.
	 */
	UFUNCTION(BlueprintCallable, Category = "Terminal|Interaction")
	void StopTerminalInteraction(class AAlphaExilemetCharacter* Interactor);

	virtual void OnConstruction(const FTransform& Transform) override;

	// ═════════════════════════════════════════════════════════════════════════
	// BLUEPRINT IMPLEMENTABLE EVENTS
	// ═════════════════════════════════════════════════════════════════════════

	/**
	 * Camera blend-in complete — show terminal widget here.
	 * Already wired in BP_BaseTerminal (calls OpenTerminalPanel).
	 * No change needed.
	 */
	UFUNCTION(BlueprintImplementableEvent, Category = "Terminal|Events")
	void BP_OnTerminalViewReady(class AAlphaExilemetCharacter* Interactor);

	/**
	 * Player tried to close while bCloseBlocked = true.
	 * Implement in BP_ShipTerminal:
	 *   → Play Sound 2D (SW_Invalid_Act)   ← you already have this
	 */
	UFUNCTION(BlueprintImplementableEvent, Category = "Terminal|Events")
	void BP_OnClosureBlocked();

	/**
	 * OK button was confirmed — fires BEFORE the camera starts blending back.
	 * Implement in BP_ShipTerminal:
	 *   → Get Game Mode → Cast → GET ShipRef → Is Valid → StopAlarm
	 * You already have this wired correctly (Image 6). No change needed.
	 */
	UFUNCTION(BlueprintImplementableEvent, Category = "Terminal|Events")
	void BP_OnConfirmationReceived();

	/**
	 * Camera blend-out COMPLETE — player has full control again.
	 * This fires at the SAFE moment to remove the terminal widget from viewport.
	 *
	 * Implement in BP_BaseTerminal EventGraph (REPLACES the old CloseTerminal
	 * custom event body):
	 *   → Is Valid (ActiveWidget)
	 *       → True → Remove from Parent (Target = ActiveWidget)
	 *                SET ActiveWidget = None
	 *   → SET Active Terminal = None     (on BP_Player via PlayerRef)
	 *   → SetPlayerHUD (In Visibility = Visible)   ← your existing Macro
	 *   → Set Input Mode Game Only
	 *       (Get Player Controller → Player Controller pin)
	 */
	UFUNCTION(BlueprintImplementableEvent, Category = "Terminal|Events")
	void BP_OnTerminalClosed();
};
