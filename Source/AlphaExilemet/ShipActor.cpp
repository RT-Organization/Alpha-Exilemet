#include "ShipActor.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/LightComponent.h"
#include "TimerManager.h"

// ─────────────────────────────────────────────────────────────────────────────
// Constructor
// ─────────────────────────────────────────────────────────────────────────────

AShipActor::AShipActor()
{
	PrimaryActorTick.bCanEverTick = false;

	// Exterior hull — skeletal for animations (hatch, struts, engine exhaust).
	ExteriorMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("ExteriorMesh"));
	RootComponent = ExteriorMesh;

	// Interior — static mesh parented to the exterior so it moves with the ship.
	InteriorMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("InteriorMesh"));
	InteriorMesh->SetupAttachment(RootComponent);
}

// ─────────────────────────────────────────────────────────────────────────────
// BeginPlay
// ─────────────────────────────────────────────────────────────────────────────

void AShipActor::BeginPlay()
{
	Super::BeginPlay();

	// Cache original light intensities BEFORE we turn them off.
	// This guarantees StopAlarm restores the exact designer-set values.
	CacheOriginalIntensities();

	// Lights start OFF — alarm hasn't been triggered yet.
	SetAllLightsIntensity(0.f);

	// Register with GameMode so GM.ShipRef is valid immediately.
	BP_RegisterWithGameMode();
}

// ─────────────────────────────────────────────────────────────────────────────
// StartAlarm
// ─────────────────────────────────────────────────────────────────────────────

void AShipActor::StartAlarm()
{
	if (bAlarmActive)
	{
		UE_LOG(LogTemp, Warning,
			TEXT("AShipActor [%s]::StartAlarm — already active, ignored."), *GetName());
		return;
	}

	bAlarmActive = true;

	// First pulse is ON so the player sees the light immediately.
	PulseOn();

	BP_OnAlarmStarted();

	UE_LOG(LogTemp, Log, TEXT("AShipActor [%s]: alarm started."), *GetName());
}

// ─────────────────────────────────────────────────────────────────────────────
// StopAlarm
// ─────────────────────────────────────────────────────────────────────────────

void AShipActor::StopAlarm()
{
	if (!bAlarmActive)
	{
		UE_LOG(LogTemp, Warning,
			TEXT("AShipActor [%s]::StopAlarm — alarm was not active, ignored."), *GetName());
		return;
	}

	bAlarmActive = false;

	// Kill the pending pulse timer — no more flashes.
	GetWorld()->GetTimerManager().ClearTimer(PulseTimerHandle);

	// Restore every light to its original designer-set intensity.
	for (int32 i = 0; i < AlarmLights.Num(); ++i)
	{
		if (!AlarmLights[i]) continue;
		if (ULightComponent* LC = AlarmLights[i]->FindComponentByClass<ULightComponent>())
		{
			const float RestoreIntensity = OriginalIntensities.IsValidIndex(i)
				? OriginalIntensities[i]
				: 0.f;
			LC->SetIntensity(RestoreIntensity);
		}
	}

	BP_OnAlarmStopped();

	UE_LOG(LogTemp, Log, TEXT("AShipActor [%s]: alarm stopped, lights restored."), *GetName());
}

// ─────────────────────────────────────────────────────────────────────────────
// CacheOriginalIntensities — called once in BeginPlay
// ─────────────────────────────────────────────────────────────────────────────

void AShipActor::CacheOriginalIntensities()
{
	OriginalIntensities.Empty();
	OriginalIntensities.Reserve(AlarmLights.Num());

	for (AActor* Actor : AlarmLights)
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

void AShipActor::PulseOn()
{
	if (!bAlarmActive) return;

	SetAllLightsIntensity(AlarmIntensity);

	GetWorld()->GetTimerManager().SetTimer(
		PulseTimerHandle,
		this,
		&AShipActor::PulseOff,
		PulseOnTime,
		false);
}

void AShipActor::PulseOff()
{
	if (!bAlarmActive) return;

	SetAllLightsIntensity(0.f);

	GetWorld()->GetTimerManager().SetTimer(
		PulseTimerHandle,
		this,
		&AShipActor::PulseOn,
		PulseOffTime,
		false);
}

void AShipActor::SetAllLightsIntensity(float Intensity)
{
	for (AActor* Actor : AlarmLights)
	{
		if (!Actor) continue;
		if (ULightComponent* LC = Actor->FindComponentByClass<ULightComponent>())
			LC->SetIntensity(Intensity);
	}
}
