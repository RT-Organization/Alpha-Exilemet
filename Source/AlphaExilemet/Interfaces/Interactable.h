#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "Interactable.generated.h"

UINTERFACE(MinimalAPI)
class UInteractable : public UInterface
{
	GENERATED_BODY()
};

// ─────────────────────────────────────────────────────────────────────────────
// IInteractable  v2
//
// Changes from v1:
//   - CanBeInteractedWith() added (BlueprintNativeEvent, returns bool).
//
// Two-function contract:
//   CanBeInteractedWith() — checked EVERY TICK by the character line trace.
//                           Returns false to hide the interaction prompt and
//                           prevent Interact() from being called.
//                           Default C++ implementation returns true (opt-in).
//
//   Interact()            — called when the player presses the interact key
//                           AND CanBeInteractedWith() returns true.
//
// USAGE EXAMPLES:
//   ASkullProp:     return bCanInteract;  (false until rise animation finishes)
//   ABaseTerminal:  return !bIsInteracting; (false during camera blend)
//   AResourceBase:  default true (no override needed)
// ─────────────────────────────────────────────────────────────────────────────

class ALPHAEXILEMET_API IInteractable
{
	GENERATED_BODY()

public:
	/**
	 * Returns true if this actor is currently interactable.
	 * Checked every tick by AAlphaExilemetCharacter::Tick() during the line trace.
	 * When false:
	 *   - bIsLookingAtInteractable stays false → interaction prompt is hidden.
	 *   - TryInteract() does nothing even if the player presses the key.
	 *
	 * Default C++ implementation returns true so existing interactables that
	 * don't need gating work with no changes.
	 */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Interaction")
	bool CanBeInteractedWith() const;
	virtual bool CanBeInteractedWith_Implementation() const { return true; }

	/**
	 * Called when the player interacts with this actor.
	 * Only called when CanBeInteractedWith() returns true.
	 */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Interaction")
	void Interact(class AAlphaExilemetCharacter* Interactor);
};
