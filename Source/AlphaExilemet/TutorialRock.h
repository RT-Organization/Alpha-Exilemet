#pragma once

#include "CoreMinimal.h"
#include "AlphaExilemet/Resources/SolidResource.h"
#include "TutorialRock.generated.h"

// ─────────────────────────────────────────────────────────────────────────────
// DELEGATE
// Declared at file scope so BP_TutorialDirector can bind to each rock instance.
// ─────────────────────────────────────────────────────────────────────────────
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnTutorialRockDestroyed, ATutorialRock*, Rock);

/**
 * ATutorialRock
 *
 * A lightweight subclass of ASolidResource used exclusively in L_Tutorial.
 * The existing pickaxe trace hits it via the ASolidResource cast — zero changes
 * needed to PickaxeTool.
 *
 * Key difference from ASolidResource:
 *   - DepleteResource() skips ore drops entirely (calls AResourceBase::DepleteResource,
 *     NOT ASolidResource::DepleteResource).
 *   - Broadcasts OnTutorialRockDestroyed so BP_TutorialDirector can count hits
 *     and trigger the skull-reveal sequence.
 */
UCLASS()
class ALPHAEXILEMET_API ATutorialRock : public ASolidResource
{
	GENERATED_BODY()

public:
	/**
	 * Bind to this in BP_TutorialDirector::BeginPlay (GetAllActorsOfClass loop).
	 * Fires once when this rock's Health drops to zero.
	 */
	UPROPERTY(BlueprintAssignable, Category = "Tutorial|Events")
	FOnTutorialRockDestroyed OnTutorialRockDestroyed;

protected:
	/**
	 * Overrides ASolidResource::DepleteResource().
	 * Calls AResourceBase::DepleteResource() directly (hides actor, disables collision,
	 * starts regen timer) — intentionally skips the ore-drop spawning logic.
	 * Then broadcasts OnTutorialRockDestroyed.
	 */
	virtual void DepleteResource() override;
};
