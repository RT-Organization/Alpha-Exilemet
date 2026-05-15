#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "PortalBase.generated.h"

class AAlphaExilemetCharacter;
class UBoxComponent;
class UStaticMeshComponent;
class UNiagaraComponent;

/**
 * APortalBase
 *
 * Drop one of these in the Main level for every portal biome.
 * Set PortalLevelName to the streaming sub-level (e.g. "Portal_Desert").
 *
 * ──────────────────────────────────────────────────────────────────────
 * BLUEPRINT EXTENSION POINTS (implement in the child BP, not in C++)
 * ──────────────────────────────────────────────────────────────────────
 *
 *  BP_OnPortalSetup(EnteringPlayer)
 *    • Runs just BEFORE the level starts streaming.
 *    • Survival is already disabled by the time this fires.
 *    • Use it to: equip a specific tool, apply a status effect, set a
 *      challenge-specific flag, play a one-shot VO, etc.
 *    • Leave empty if the portal needs no special setup.
 *
 * ──────────────────────────────────────────────────────────────────────
 * CHALLENGE COMPLETION (for the other programmer)
 * ──────────────────────────────────────────────────────────────────────
 *
 *  When the challenge is done, call:
 *    StreamingSubsystem → CompletePortalChallenge()
 *
 *  That function re-enables survival, broadcasts OnPortalExitStarted so
 *  the GameMode BP can show the loading screen, and then the GameMode BP
 *  calls ExitPortal() after a short fade delay.
 *
 * ──────────────────────────────────────────────────────────────────────
 * LEVEL STREAMING NOTE
 * ──────────────────────────────────────────────────────────────────────
 *  Main is NEVER unloaded. Portals are loaded ON TOP of Main.
 *  On exit, only the portal sub-level is unloaded.
 */
UCLASS()
class ALPHAEXILEMET_API APortalBase : public AActor
{
	GENERATED_BODY()

public:
	APortalBase();

protected:
	virtual void BeginPlay() override;

	// =========================================================================
	// COMPONENTS
	// =========================================================================

	/**
	 * The arch mesh. Assign the static mesh in the Blueprint CDO.
	 * Collision is disabled on the mesh — the TriggerBox handles interaction.
	 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Portal|Components")
	UStaticMeshComponent* PortalMesh;

	/**
	 * Walk-through box collider placed at the interior of the arch.
	 * Move and resize it freely in the Viewport until the UX feels right.
	 * Default extent: X=40, Y=80, Z=90  (cm) — a sensible start for a
	 * human-sized arch opening.  Adjust per mesh.
	 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Portal|Components")
	UBoxComponent* TriggerBox;

	/**
	 * Niagara VFX component placed at the arch centre.
	 * Assign the Niagara System asset in the Blueprint CDO.
	 * Activated automatically on BeginPlay.
	 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Portal|Components")
	UNiagaraComponent* PortalVFX;

public:
	// =========================================================================
	// CONFIGURATION  (set in Blueprint CDO or per-instance in the editor)
	// =========================================================================

	/**
	 * Name of the streaming sub-level to load when the player steps through.
	 * Must match the sub-level asset name exactly (e.g. "Portal_Desert").
	 * Leave empty → portal is inert.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Portal|Config")
	FName PortalLevelName;

	/**
	 * Yaw (degrees) added to the player's rotation when they return.
	 * 180° = player faces back toward the arch they exited — recommended.
	 * Set 0 to keep the same rotation they had on entry.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Portal|Config")
	float ReturnYawOffset = 180.0f;

	/**
	 * Seconds the trigger is suppressed after a successful activation.
	 * Prevents double-fires while the loading screen is coming up.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Portal|Config")
	float TriggerCooldown = 3.0f;

	// =========================================================================
	// RUNTIME STATE
	// =========================================================================

	/** Set false from BP to lock a portal (e.g. not yet unlocked). */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Portal|State")
	bool bPortalActive = true;

	// =========================================================================
	// PUBLIC FUNCTIONS
	// =========================================================================

	/** Enable or disable the portal at runtime (also disables overlap events). */
	UFUNCTION(BlueprintCallable, Category = "Portal")
	void SetPortalActive(bool bActive);

	/**
	 * Manually trigger entry — normally called automatically by the box trigger.
	 * Exposed so cutscenes / debug commands can force an entry.
	 */
	UFUNCTION(BlueprintCallable, Category = "Portal")
	void EnterPortalLevel(AAlphaExilemetCharacter* Player);

	// =========================================================================
	// BLUEPRINT EXTENSION POINTS
	// =========================================================================

	/**
	 * IMPLEMENT IN BLUEPRINT (optional).
	 *
	 * Fires just before the streaming call, after survival is disabled.
	 * Use it to configure per-portal challenge state:
	 *   • Equip the pickaxe / specific tool
	 *   • Set a challenge-progress variable on the GameInstance
	 *   • Play a VO / ambient sound
	 *   • Spawn a challenge actor
	 */
	UFUNCTION(BlueprintImplementableEvent, Category = "Portal|Events")
	void BP_OnPortalSetup(AAlphaExilemetCharacter* EnteringPlayer);

private:
	// =========================================================================
	// INTERNALS
	// =========================================================================

	bool bOnCooldown = false;
	FTimerHandle CooldownHandle;

	UFUNCTION()
	void OnOverlapBegin(UPrimitiveComponent* OverlappedComp,
	                    AActor*             OtherActor,
	                    UPrimitiveComponent* OtherComp,
	                    int32               OtherBodyIndex,
	                    bool                bFromSweep,
	                    const FHitResult&   SweepResult);

	void ClearCooldown();
};