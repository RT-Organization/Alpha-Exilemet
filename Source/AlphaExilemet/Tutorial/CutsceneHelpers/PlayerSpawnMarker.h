#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "PlayerSpawnMarker.generated.h"

class UArrowComponent;
class UBillboardComponent;

/**
 * APlayerSpawnMarker
 *
 * A lightweight editor-only marker that defines WHERE and in which direction
 * the BP_Player should appear at the end of a cutscene.
 *
 * ──────────────────────────────────────────────────────────────────────
 * WHY THIS EXISTS
 * ──────────────────────────────────────────────────────────────────────
 * Animator Spawnable SKs frequently have incorrect pivot points, making it
 * impossible to derive the correct player foot position from the proxy's
 * root transform. This marker is placed manually by the level designer at
 * the exact foot position of the character at the last cutscene frame.
 *
 * ──────────────────────────────────────────────────────────────────────
 * PLACEMENT WORKFLOW
 * ──────────────────────────────────────────────────────────────────────
 *   1. Open the sequence in the Sequencer panel.
 *   2. Scrub to the very last frame (red playhead to end).
 *   3. In the viewport, identify where the SK character's FEET are
 *      (not the root bone — the actual foot contact point with the ground).
 *   4. Place BP_PlayerSpawnMarker at that world position.
 *   5. Rotate the marker's forward arrow (+X) toward where the player
 *      should look on the first frame of gameplay.
 *   6. Assign this marker to:
 *        TutorialDirector → "Tutorial Player Spawn Marker"   (Tutorial)
 *        WakeUpDirector   → "Wake Up Player Spawn Marker"    (Main)
 *
 * ──────────────────────────────────────────────────────────────────────
 * RUNTIME BEHAVIOR
 * ──────────────────────────────────────────────────────────────────────
 * The actor is fully invisible at runtime. It has no collision, no mesh,
 * and no tick. Only the green arrow and billboard are visible in the editor.
 */
UCLASS(HideCategories = (Collision, Rendering, Replication, Actor))
class ALPHAEXILEMET_API APlayerSpawnMarker : public AActor
{
	GENERATED_BODY()

public:
	APlayerSpawnMarker();

protected:
	virtual void BeginPlay() override;

public:
	// ── EDITOR VISUALS ────────────────────────────────────────────────────────

	UPROPERTY(VisibleAnywhere, Category = "SpawnMarker|Visuals")
	UArrowComponent* Arrow;

	UPROPERTY(VisibleAnywhere, Category = "SpawnMarker|Visuals")
	UBillboardComponent* Billboard;

	// ── PUBLIC API ────────────────────────────────────────────────────────────

	/**
	 * Returns the full FTransform (Location + Rotation + Scale=1) to use
	 * for the player pawn when UCinematicHandoffComponent::BeginHandoff is called.
	 *
	 * The rotation's forward vector (+X) should face where the player looks
	 * on the first frame of gameplay after the cutscene.
	 */
	UFUNCTION(BlueprintPure, Category = "SpawnMarker")
	FTransform GetSpawnTransform() const;
};
