#include "AlarmController.h"
#include "Components/LightComponent.h"
#include "TimerManager.h"

// ─────────────────────────────────────────────────────────────────────────────
// Constructor / BeginPlay
// ─────────────────────────────────────────────────────────────────────────────

AAlarmController::AAlarmController()
{
	PrimaryActorTick.bCanEverTick = false;
}

void AAlarmController::BeginPlay()
{
	Super::BeginPlay();

	// Make sure lights start OFF — the alarm hasn't been triggered yet.
	// We also cache their original intensities here so StopAlarm can restore them.
	CacheOriginalIntensities();
	SetAllLightsIntensity(0.f);

	// Register self with the GameMode so it can hold AlarmControllerRef.
	BP_RegisterWithGameMode();
}

// ─────────────────────────────────────────────────────────────────────────────
// StartAlarm
// ─────────────────────────────────────────────────────────────────────────────

void AAlarmController::StartAlarm()
{
	if (bAlarmActive) return; // already running — ignore double calls
	bAlarmActive = true;

	UE_LOG(LogTemp, Log, TEXT("AAlarmController::StartAlarm — alarm pulse started."));

	// Start with the light ON so the player sees it immediately.
	PulseOn();

	BP_OnAlarmStarted();
}

// ─────────────────────────────────────────────────────────────────────────────
// StopAlarm
// ─────────────────────────────────────────────────────────────────────────────

void AAlarmController::StopAlarm()
{
	if (!bAlarmActive) return; // nothing to stop
	bAlarmActive      = false;
	bAlarmEverStopped = true;

	// Kill any pending pulse timer.
	GetWorld()->GetTimerManager().ClearTimer(PulseTimerHandle);

	// Restore lights to their original intensities (as set by the designer).
	for (int32 i = 0; i < AlarmActors.Num(); ++i)
	{
		if (!AlarmActors[i]) continue;
		if (ULightComponent* LC = AlarmActors[i]->FindComponentByClass<ULightComponent>())
		{
			LC->SetIntensity(OriginalIntensities.IsValidIndex(i)
				? OriginalIntensities[i]
				: 0.f);
		}
	}

	bLightsCurrentlyOn = false;

	UE_LOG(LogTemp, Log, TEXT("AAlarmController::StopAlarm — alarm stopped, lights restored."));

	BP_OnAlarmStopped();
}

// ─────────────────────────────────────────────────────────────────────────────
// CacheOriginalIntensities
// Called once in BeginPlay before we turn lights off.
// ─────────────────────────────────────────────────────────────────────────────

void AAlarmController::CacheOriginalIntensities()
{
	OriginalIntensities.Empty();
	OriginalIntensities.Reserve(AlarmActors.Num());

	for (AActor* Actor : AlarmActors)
	{
		float Intensity = 0.f;
		if (Actor)
		{
			if (ULightComponent* LC = Actor->FindComponentByClass<ULightComponent>())
				Intensity = LC->Intensity;
		}
		OriginalIntensities.Add(Intensity);
	}
}

// ─────────────────────────────────────────────────────────────────────────────
// Pulse helpers
// ─────────────────────────────────────────────────────────────────────────────

void AAlarmController::PulseOn()
{
	if (!bAlarmActive) return;

	SetAllLightsIntensity(AlarmIntensity);
	bLightsCurrentlyOn = true;

	// Schedule the OFF phase.
	GetWorld()->GetTimerManager().SetTimer(
		PulseTimerHandle,
		this,
		&AAlarmController::PulseOff,
		PulseOnTime,
		false);
}

void AAlarmController::PulseOff()
{
	if (!bAlarmActive) return;

	SetAllLightsIntensity(0.f);
	bLightsCurrentlyOn = false;

	// Schedule the ON phase.
	GetWorld()->GetTimerManager().SetTimer(
		PulseTimerHandle,
		this,
		&AAlarmController::PulseOn,
		PulseOffTime,
		false);
}

// ─────────────────────────────────────────────────────────────────────────────
// SetAllLightsIntensity
// ─────────────────────────────────────────────────────────────────────────────

void AAlarmController::SetAllLightsIntensity(float Intensity)
{
	for (AActor* Actor : AlarmActors)
	{
		if (!Actor) continue;
		if (ULightComponent* LC = Actor->FindComponentByClass<ULightComponent>())
			LC->SetIntensity(Intensity);
	}
}
