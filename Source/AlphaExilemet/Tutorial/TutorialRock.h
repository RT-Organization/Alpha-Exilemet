#pragma once

#include "CoreMinimal.h"
#include "AlphaExilemet/Resources/SolidResource.h"
#include "TutorialRock.generated.h"

class UNiagaraSystem;

// ─────────────────────────────────────────────────────────────────────────────
// DELEGATE
// ─────────────────────────────────────────────────────────────────────────────
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnTutorialRockDestroyed, ATutorialRock*, Rock);

/**
 * ATutorialRock  v2
 *
 * Changes from v1:
 *   - Added DestructionVFX (UNiagaraSystem*) — assign NS_RockBreak (or similar)
 *     in BP_TutorialRock Class Defaults. Spawns at the rock's world location
 *     when health reaches zero. No extra Blueprint nodes needed.
 *   - DestructionVFXScale controls the emitter scale (default 1.0).
 */
UCLASS()
class ALPHAEXILEMET_API ATutorialRock : public ASolidResource
{
	GENERATED_BODY()

public:
	// ── VFX ──────────────────────────────────────────────────────────────────

	/**
	 * Niagara particle system spawned at the rock's location when depleted.
	 * Assign in BP_TutorialRock → Class Defaults → Tutorial|VFX.
	 * Leave null to skip VFX entirely (no error).
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Tutorial|VFX")
	UNiagaraSystem* DestructionVFX = nullptr;

	/** Uniform scale applied to the spawned Niagara emitter. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Tutorial|VFX")
	float DestructionVFXScale = 1.0f;

	// ── EVENTS ───────────────────────────────────────────────────────────────

	/**
	 * Bind in BP_TutorialDirector::BeginPlay (GetAllActorsOfClass loop).
	 * Fires once when this rock's health drops to zero.
	 */
	UPROPERTY(BlueprintAssignable, Category = "Tutorial|Events")
	FOnTutorialRockDestroyed OnTutorialRockDestroyed;

protected:
	/**
	 * Overrides ASolidResource::DepleteResource().
	 *   1. Spawns DestructionVFX at the rock's world location (if assigned).
	 *   2. Calls AResourceBase::DepleteResource() — hides actor, disables
	 *      collision, starts regen timer. Skips ore-drop logic intentionally.
	 *   3. Broadcasts OnTutorialRockDestroyed.
	 */
	virtual void DepleteResource() override;
};
