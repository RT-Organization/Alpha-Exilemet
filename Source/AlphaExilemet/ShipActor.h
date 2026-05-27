#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ShipActor.generated.h"

class ABaseTerminal;

// ─────────────────────────────────────────────────────────────────────────────
// AShipActor  v1
//
// THE single actor that represents the player's ship in L_Main.
// Responsibilities (one per concern — nothing bleeds across boundaries):
//
//   VISUAL    — Exterior skeletal mesh + Interior static mesh.
//   ALARM     — Pulse alarm lights on/off via timer.
//   TERMINALS — Holds up to 5 terminal actor references for editor assignment.
//               The ship does NOT spawn terminals — they are pre-placed in the
//               level and assigned in this actor's Instance Details panel.
//               This lets the designer position each terminal freely.
//
// WHAT THIS CLASS DOES NOT DO:
//   - It does NOT know about widgets or UI.
//   - It does NOT call anything on terminals (terminals are just grouped here).
//   - It does NOT handle player input.
//
// HOW ALARM COMMUNICATION WORKS (no coupling):
//   Ship ← StartAlarm()     called by BP_WakeUpDirector::BP_OnWakeUpComplete
//   Ship ← StopAlarm()      called by BP_ShipTerminal::BP_OnConfirmationReceived
//   Ship fires BP_OnAlarmStarted / BP_OnAlarmStopped for SFX / VFX in BP.
//
// SETUP:
//   1. Create BP_Ship child Blueprint of AShipActor.
//   2. Set ExteriorMesh and InteriorMesh assets in Class Defaults.
//   3. Place BP_Ship in L_Main.
//   4. In the placed instance Details → Ship | Terminals: assign each
//      BP_ShipTerminal actor that is already placed in the level.
//   5. In the placed instance Details → Alarm | Lights: assign every
//      PointLight / SpotLight actor you want to flash during the alarm.
//   6. Implement BP_RegisterWithGameMode to store Self in GM.ShipRef.
// ─────────────────────────────────────────────────────────────────────────────

UCLASS(Abstract, Blueprintable)
class ALPHAEXILEMET_API AShipActor : public AActor
{
	GENERATED_BODY()

public:
	AShipActor();

protected:
	virtual void BeginPlay() override;

public:
	// ═════════════════════════════════════════════════════════════════════════
	// VISUAL COMPONENTS
	// ═════════════════════════════════════════════════════════════════════════

	/**
	 * The ship's exterior hull — a Skeletal Mesh so it can be animated
	 * (landing struts, hatch opens, engine exhaust, etc.)
	 * Visible from outside the ship.
	 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Ship|Mesh")
	USkeletalMeshComponent* ExteriorMesh;

	/**
	 * The ship's interior — a Static Mesh (walls, floors, ceiling, consoles).
	 * Stays rigid; no animation needed.
	 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Ship|Mesh")
	UStaticMeshComponent* InteriorMesh;

	// ═════════════════════════════════════════════════════════════════════════
	// TERMINAL REFERENCES
	// ═════════════════════════════════════════════════════════════════════════

	/**
	 * The terminals physically located inside this ship.
	 * Assign in the PLACED INSTANCE Details panel (up to 5 is typical).
	 * The terminals are independent actors — this array is purely for grouping.
	 * The ship does NOT call anything on them; they manage themselves.
	 *
	 * Designer workflow:
	 *   1. Place BP_ShipTerminal actors inside the ship mesh in the level.
	 *   2. Select this ship actor.
	 *   3. Details → Ship | Terminals → add each terminal actor here.
	 */
	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Ship|Terminals",
		meta = (DisplayName = "Ship Terminals"))
	TArray<ABaseTerminal*> Terminals;

	// ═════════════════════════════════════════════════════════════════════════
	// ALARM — LIGHT ACTORS
	// ═════════════════════════════════════════════════════════════════════════

	/**
	 * Every alarm light actor (PointLight, SpotLight, RectLight…) inside the
	 * ship that should pulse during the alarm.
	 * Assign in the PLACED INSTANCE Details panel.
	 * The actor just needs a ULightComponent — type doesn't matter.
	 */
	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Alarm|Lights",
		meta = (DisplayName = "Alarm Light Actors"))
	TArray<AActor*> AlarmLights;

	// ═════════════════════════════════════════════════════════════════════════
	// ALARM — CONFIGURATION
	// ═════════════════════════════════════════════════════════════════════════

	/**
	 * Brightness the lights reach while ON during each pulse.
	 * Units: lux (same as UE PointLight Intensity).
	 * Tune this to match your light actors' normal scene brightness.
	 * Default 8000 is strong but readable; increase for dramatic effect.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Alarm|Config")
	float AlarmIntensity = 8000.f;

	/**
	 * Seconds the lights stay ON per pulse.
	 * Shorter = more urgent alarm.  Default: 0.25s.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Alarm|Config")
	float PulseOnTime = 0.25f;

	/**
	 * Seconds the lights stay OFF per pulse.
	 * Asymmetric (on < off) gives a snappier alarm feel.  Default: 0.45s.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Alarm|Config")
	float PulseOffTime = 0.45f;

	// ═════════════════════════════════════════════════════════════════════════
	// ALARM — STATE (read-only from BP)
	// ═════════════════════════════════════════════════════════════════════════

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Alarm|State")
	bool bAlarmActive = false;

	// ═════════════════════════════════════════════════════════════════════════
	// ALARM — PUBLIC INTERFACE
	// ═════════════════════════════════════════════════════════════════════════

	/**
	 * Start the alarm pulse.
	 * Call from BP_WakeUpDirector → Event BP_OnWakeUpComplete.
	 *   [Get Game Mode → Cast → GET ShipRef → Is Valid → StartAlarm]
	 * Safe to call multiple times — does nothing if already active.
	 */
	UFUNCTION(BlueprintCallable, Category = "Alarm")
	void StartAlarm();

	/**
	 * Stop the alarm pulse and restore lights to their original intensities.
	 * Call from BP_ShipTerminal → Event BP_OnConfirmationReceived.
	 *   [Get Game Mode → Cast → GET ShipRef → Is Valid → StopAlarm]
	 * Safe to call when alarm is already stopped.
	 */
	UFUNCTION(BlueprintCallable, Category = "Alarm")
	void StopAlarm();

protected:
	// ═════════════════════════════════════════════════════════════════════════
	// BLUEPRINT IMPLEMENTABLE EVENTS
	// ═════════════════════════════════════════════════════════════════════════

	/**
	 * Called at END of C++ BeginPlay.
	 * Implement in BP_Ship:
	 *   [Get Game Mode → Cast To GM_SimulatorGamemode → SET ShipRef = Self]
	 */
	UFUNCTION(BlueprintImplementableEvent, Category = "Ship|Events")
	void BP_RegisterWithGameMode();

	/**
	 * Fires right after lights start pulsing.
	 * Use for: Play alarm SFX loop, Niagara particle effects, HUD alert, etc.
	 */
	UFUNCTION(BlueprintImplementableEvent, Category = "Alarm|Events")
	void BP_OnAlarmStarted();

	/**
	 * Fires right after lights stop pulsing and are restored.
	 * Use for: Stop SFX, stop Niagara, clear HUD alert, etc.
	 */
	UFUNCTION(BlueprintImplementableEvent, Category = "Alarm|Events")
	void BP_OnAlarmStopped();

private:
	// ═════════════════════════════════════════════════════════════════════════
	// INTERNAL — PULSE LOGIC
	// ═════════════════════════════════════════════════════════════════════════

	FTimerHandle PulseTimerHandle;

	// Cached original intensities — restored exactly when StopAlarm is called.
	// Index matches AlarmLights[].
	TArray<float> OriginalIntensities;

	void CacheOriginalIntensities();
	void PulseOn();
	void PulseOff();
	void SetAllLightsIntensity(float Intensity);
};
