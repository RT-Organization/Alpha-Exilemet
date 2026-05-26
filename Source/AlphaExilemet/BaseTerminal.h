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
//   - Close-lock system added.
//     When bRequiresConfirmation = true (set in subclass Class Defaults),
//     the terminal CANNOT be closed until ConfirmAndCloseTerminal() is called.
//
//     BP_OnTerminalViewReady → WBP_ShipTerminal::CheckIfNewGame finds a new game
//     → calls BlockTerminalClose() on the terminal → shows WB_ShipRepairWarning.
//
//     OK button on WB_ShipRepairWarning → calls ConfirmAndCloseTerminal() on the
//     terminal → alarm stops → camera blends back → BP_OnTerminalClosed fires.
//
//     If the player tries to close before confirming, TryStopTerminalInteraction()
//     does NOT close and calls BP_OnClosureBlocked() so you can play a denied SFX
//     or shake the UI.
//
//   - BP_OnTerminalClosed() added.
//     Fires in RestoreInput() (after the blend-out finishes, player has full
//     control again). Use this in WBP_ShipTerminal to Remove from Parent.
//
//   - AlarmControllerClass / AlarmControllerRef removed from here intentionally.
//     The terminal does NOT know about alarms directly — it calls
//     BP_OnConfirmationReceived() so the Blueprint subclass (BP_ShipTerminal)
//     can grab the alarm controller from the GM and call StopAlarm().
//     This keeps the base class clean and alarm-agnostic.
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

	/**
	 * True while the camera is blending in or out.
	 * CanBeInteractedWith() returns false during this time.
	 * Set true on interaction start, false on RestoreInput.
	 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Terminal")
	bool bIsInteracting = false;

	// ── CONFIRMATION LOCK ─────────────────────────────────────────────────────

	/**
	 * Set this to TRUE in the Blueprint subclass (e.g. BP_ShipTerminal) Class Defaults
	 * if this terminal requires a confirmation step before the player can close it.
	 *
	 * When true:
	 *   - TryStopTerminalInteraction() will BLOCK closing and call BP_OnClosureBlocked().
	 *   - Only ConfirmAndCloseTerminal() bypasses the lock and closes the terminal.
	 *   - BlockTerminalClose() resets the per-session flag (call when the warning appears).
	 *
	 * When false (default):
	 *   - TryStopTerminalInteraction() behaves exactly like StopTerminalInteraction().
	 *   - No change to existing terminals.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Terminal|Confirmation")
	bool bRequiresConfirmation = false;

	/**
	 * Runtime flag — true while the terminal is waiting for the player to confirm.
	 * Reset each time the terminal is opened (in OnBlendComplete).
	 * Cleared by ConfirmAndCloseTerminal().
	 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Terminal|Confirmation")
	bool bCloseBlocked = false;

	// ── INTERACTABLE INTERFACE ────────────────────────────────────────────────

	virtual bool CanBeInteractedWith_Implementation() const override;
	virtual void Interact_Implementation(class AAlphaExilemetCharacter* Interactor) override;

	// ── PUBLIC INTERFACE ──────────────────────────────────────────────────────

	/**
	 * Call from WBP_ShipTerminal when the confirmation warning appears.
	 * Sets bCloseBlocked = true so the back button is locked.
	 */
	UFUNCTION(BlueprintCallable, Category = "Terminal|Confirmation")
	void BlockTerminalClose();

	/**
	 * The SAFE close function — use this everywhere instead of StopTerminalInteraction.
	 *
	 * - If bRequiresConfirmation = false  →  closes normally (same as before).
	 * - If bRequiresConfirmation = true AND bCloseBlocked = true
	 *       →  does NOT close; calls BP_OnClosureBlocked() (play denied SFX here).
	 * - If bRequiresConfirmation = true AND bCloseBlocked = false
	 *       →  closes normally (confirmation was already given earlier this session).
	 *
	 * Wire this to your back/close button in WBP_ShipTerminal.
	 */
	UFUNCTION(BlueprintCallable, Category = "Terminal|Interaction")
	void TryStopTerminalInteraction(class AAlphaExilemetCharacter* Interactor);

	/**
	 * Called by WB_ShipRepairWarning OK button.
	 * Clears the close block, fires BP_OnConfirmationReceived (stop alarm there),
	 * then begins the camera blend back to the player.
	 *
	 * Wire: WB_ShipRepairWarning → On Clicked (OK Button)
	 *         → Get Owning Actor → Cast To ABaseTerminal
	 *         → ConfirmAndCloseTerminal(Interactor)
	 */
	UFUNCTION(BlueprintCallable, Category = "Terminal|Confirmation")
	void ConfirmAndCloseTerminal(class AAlphaExilemetCharacter* Interactor);

	/**
	 * Direct close — bypasses all locks.
	 * Keep this for programmatic closes (level transition cleanup, death, etc.)
	 * Do NOT wire this to UI buttons — use TryStopTerminalInteraction instead.
	 */
	UFUNCTION(BlueprintCallable, Category = "Terminal|Interaction")
	void StopTerminalInteraction(class AAlphaExilemetCharacter* Interactor);

	virtual void OnConstruction(const FTransform& Transform) override;

	// ── BLUEPRINT EVENTS ──────────────────────────────────────────────────────

	/** Camera blend-in done — show your UI here. */
	UFUNCTION(BlueprintImplementableEvent, Category = "Terminal|Events")
	void BP_OnTerminalViewReady(class AAlphaExilemetCharacter* Interactor);

	/**
	 * Player tried to close the terminal while bCloseBlocked = true.
	 * Play a denied sound or shake the UI here.
	 * Example: Play Sound 2D (SW_UI_Denied) or Play Animation (ShakeAnim).
	 */
	UFUNCTION(BlueprintImplementableEvent, Category = "Terminal|Events")
	void BP_OnClosureBlocked();

	/**
	 * Fires when ConfirmAndCloseTerminal() is called — BEFORE the camera blends back.
	 * Use this to: Stop the alarm, remove WB_ShipRepairWarning from parent, etc.
	 *
	 * Example BP implementation:
	 *   [Get Game Mode → Cast → Get AlarmControllerRef → Is Valid]
	 *     → True → [StopAlarm (Target = AlarmControllerRef)]
	 */
	UFUNCTION(BlueprintImplementableEvent, Category = "Terminal|Events")
	void BP_OnConfirmationReceived();

	/**
	 * Fires in RestoreInput() — camera has fully returned, player has full control.
	 * Use this to Remove from Parent on the terminal UI widget.
	 *
	 * Example BP implementation:
	 *   [Remove from Parent (Target = TerminalWidgetRef)]
	 */
	UFUNCTION(BlueprintImplementableEvent, Category = "Terminal|Events")
	void BP_OnTerminalClosed();
};
