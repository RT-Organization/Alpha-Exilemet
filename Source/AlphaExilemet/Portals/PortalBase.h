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
 * IMPORTANT — TRIGGER BOX SIZE
 * ──────────────────────────────────────────────────────────────────────
 * The TriggerBox is attached to PortalMesh. If you scale the portal actor
 * in the editor (e.g. 7.5x), the TriggerBox scale compounds.
 *
 * ALWAYS check the TriggerBox in the Blueprint Viewport:
 *   1. Select TriggerBox in the Components panel.
 *   2. Make sure its WORLD SIZE is roughly 80×200×220 cm
 *      (wide enough for the player capsule to pass through reliably).
 *   3. If the portal actor has a large scale, reduce the TriggerBox scale
 *      to compensate, or set TriggerBoxExtent to a smaller value in CDO.
 *
 * Default TriggerBoxExtent = 50×130×130 cm — tune per portal in BP CDO.
 *
 * ──────────────────────────────────────────────────────────────────────
 * BLUEPRINT EXTENSION POINTS
 * ──────────────────────────────────────────────────────────────────────
 *
 *  BP_OnPortalSetup(EnteringPlayer)
 *    Fires just BEFORE the level starts streaming, survival already off.
 *    Use it to equip tools, set challenge flags, play VO, etc.
 *    Leave empty if this portal needs no special setup.
 *
 * ──────────────────────────────────────────────────────────────────────
 * CHALLENGE COMPLETION
 * ──────────────────────────────────────────────────────────────────────
 *
 *  When done, call from any BP:
 *    StreamingSubsystem → CompletePortalChallenge()
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

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Portal|Components")
	UStaticMeshComponent* PortalMesh;

	/**
	 * Walk-through trigger box.
	 * RESIZE THIS in the Blueprint Viewport after assigning the mesh.
	 * Select TriggerBox → Details → Shape → Box Extent.
	 * Make sure it covers the arch opening in world space.
	 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Portal|Components")
	UBoxComponent* TriggerBox;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Portal|Components")
	UNiagaraComponent* PortalVFX;

public:
	// =========================================================================
	// CONFIGURATION
	// =========================================================================

	/** Sub-level name to load on entry. Must match the asset name exactly. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Portal|Config")
	FName PortalLevelName;

	/**
	 * Yaw added to the player's rotation on return.
	 * 180 = player faces back toward the portal arch. Recommended default.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Portal|Config")
	float ReturnYawOffset = 180.0f;

	/**
	 * Seconds the trigger is locked after one activation.
	 * Prevents double-fires during the loading screen fade-in.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Portal|Config")
	float TriggerCooldown = 3.0f;

	/**
	 * Half-extents (cm) of the TriggerBox set in the constructor.
	 * Override in the Blueprint CDO if the default doesn't fit your arch.
	 *
	 * NOTE: If the portal actor itself is scaled in the editor, these extents
	 * are affected by the parent scale. In that case, shrink the extents here
	 * to compensate, OR keep actor scale at 1 and scale the mesh component
	 * instead (preferred).
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Portal|Config")
	FVector TriggerBoxExtent = FVector(50.f, 130.f, 130.f);

	// =========================================================================
	// RUNTIME STATE
	// =========================================================================

	/** Set false from BP to lock a portal (not yet unlocked). */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Portal|State")
	bool bPortalActive = true;

	// =========================================================================
	// PUBLIC FUNCTIONS
	// =========================================================================

	/** Enable or disable the portal at runtime. */
	UFUNCTION(BlueprintCallable, Category = "Portal")
	void SetPortalActive(bool bActive);

	/**
	 * Manually trigger entry (also called automatically by the TriggerBox).
	 * Exposed for cutscenes or debug commands.
	 */
	UFUNCTION(BlueprintCallable, Category = "Portal")
	void EnterPortalLevel(AAlphaExilemetCharacter* Player);

	// =========================================================================
	// BLUEPRINT EXTENSION
	// =========================================================================

	/** Override in Blueprint to set up per-portal challenge state. */
	UFUNCTION(BlueprintImplementableEvent, Category = "Portal|Events")
	void BP_OnPortalSetup(AAlphaExilemetCharacter* EnteringPlayer);

private:
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