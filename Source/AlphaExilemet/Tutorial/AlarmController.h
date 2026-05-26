#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "AlarmController.generated.h"

// ─────────────────────────────────────────────────────────────────────────────
// AAlarmController  v1
//
// Placed once in L_Main. Drag your ship alarm light actors into AlarmActors[].
// The controller pulses every light's ULightComponent between 0 and
// AlarmIntensity on a timer, creating the classic alarm flash effect.
//
// FLOW:
//   WakeUp handoff complete  →  BP_OnWakeUpComplete  →  GM.AlarmControllerRef.StartAlarm()
//   WB_ShipRepairWarning OK  →  BaseTerminal.ConfirmAndCloseTerminal()
//                            →  GM.AlarmControllerRef.StopAlarm()
//
// HOW TO SET UP IN EDITOR:
//   1. Place BP_AlarmController in L_Main.
//   2. In the placed instance Details panel → Alarm | Lights, add every alarm
//      light actor you want to pulse (PointLight, SpotLight, etc.)
//   3. Tune AlarmIntensity, PulseOnTime, PulseOffTime in Class Defaults.
//   4. In BP_AlarmController, implement BP_RegisterWithGameMode:
//        Get Game Mode → Cast To GM_SimulatorGamemode → SET AlarmControllerRef = Self
// ─────────────────────────────────────────────────────────────────────────────

UCLASS(Abstract, Blueprintable)
class ALPHAEXILEMET_API AAlarmController : public AActor
{
	GENERATED_BODY()

public:
	AAlarmController();

protected:
	virtual void BeginPlay() override;

public:
	// ── CONFIGURATION ─────────────────────────────────────────────────────────

	/**
	 * Every light actor that should pulse during the alarm.
	 * Assign in the PLACED INSTANCE Details panel (EditInstanceOnly).
	 * Accepts any actor that has a ULightComponent (PointLight, SpotLight, etc.)
	 */
	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Alarm|Lights",
		meta = (DisplayName = "Alarm Light Actors"))
	TArray<AActor*> AlarmActors;

	/**
	 * Intensity the lights reach when they are ON during the pulse.
	 * Match this to the brightness of the red alarm lights in your scene.
	 * Default: 8000 (a strong but not screen-blowing value for indoor lights).
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Alarm|Config")
	float AlarmIntensity = 8000.f;

	/**
	 * How long the light stays ON each pulse (seconds).
	 * Shorter = more frantic alarm. Default: 0.25s.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Alarm|Config")
	float PulseOnTime = 0.25f;

	/**
	 * How long the light stays OFF each pulse (seconds).
	 * Asymmetric on/off gives a more realistic alarm feel. Default: 0.45s.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Alarm|Config")
	float PulseOffTime = 0.45f;

	// ── STATE ─────────────────────────────────────────────────────────────────

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Alarm|State")
	bool bAlarmActive = false;

	// ── PUBLIC INTERFACE ──────────────────────────────────────────────────────

	/**
	 * Start the alarm pulse.
	 * Call from BP_WakeUpDirector::BP_OnWakeUpComplete via GM.AlarmControllerRef.
	 * Safe to call multiple times — ignored if already active.
	 */
	UFUNCTION(BlueprintCallable, Category = "Alarm")
	void StartAlarm();

	/**
	 * Stop the alarm pulse and restore lights to their original intensity.
	 * Call when WB_ShipRepairWarning OK is confirmed (via BaseTerminal).
	 * Safe to call when alarm is not active.
	 */
	UFUNCTION(BlueprintCallable, Category = "Alarm")
	void StopAlarm();

	/** True if the alarm has been stopped at least once (tutorial step complete). */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Alarm|State")
	bool bAlarmEverStopped = false;

protected:
	// ── BLUEPRINT EVENTS ──────────────────────────────────────────────────────

	/**
	 * Implement in BP_AlarmController:
	 *   [Get Game Mode → Cast To GM_SimulatorGamemode → SET AlarmControllerRef = Self]
	 */
	UFUNCTION(BlueprintImplementableEvent, Category = "Alarm|Events")
	void BP_RegisterWithGameMode();

	/**
	 * Optional: play alarm SFX loop, trigger Niagara effects, etc.
	 * Called right after the lights begin pulsing.
	 */
	UFUNCTION(BlueprintImplementableEvent, Category = "Alarm|Events")
	void BP_OnAlarmStarted();

	/**
	 * Optional: stop SFX, stop Niagara effects, etc.
	 * Called right after the lights stop pulsing and are restored.
	 */
	UFUNCTION(BlueprintImplementableEvent, Category = "Alarm|Events")
	void BP_OnAlarmStopped();

private:
	// ── INTERNAL ──────────────────────────────────────────────────────────────

	FTimerHandle PulseTimerHandle;
	bool         bLightsCurrentlyOn = false;

	// Stores the original intensity of each light so StopAlarm can restore it.
	// Index matches AlarmActors[].
	TArray<float> OriginalIntensities;

	void CacheOriginalIntensities();
	void PulseOn();
	void PulseOff();
	void ScheduleNext();
	void SetAllLightsIntensity(float Intensity);
};
