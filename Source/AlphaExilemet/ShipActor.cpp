#include "ShipActor.h"
#include "BaseCamp.h"
#include "AlphaExilemetCharacter.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/LightComponent.h"
#include "Animation/AnimInstance.h"
#include "TimerManager.h"
#include "Components/SphereComponent.h"
#include "Kismet/GameplayStatics.h"

AShipActor::AShipActor()
{
	PrimaryActorTick.bCanEverTick = false;

	ExteriorMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("ExteriorMesh"));
	RootComponent = ExteriorMesh;

	InteriorMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("InteriorMesh"));
	InteriorMesh->SetupAttachment(RootComponent);
}

void AShipActor::BeginPlay()
{
	Super::BeginPlay();

	// Collect lights, cache their ORIGINAL state (intensity + color).
	// Do NOT turn them off — they are interior lights that stay on normally.
	CollectAlarmLightsByTag();
	CacheOriginalLightState();

	// Bind montage-ended callback once.
	if (UAnimInstance* AnimInst = ExteriorMesh->GetAnimInstance())
	{
		AnimInst->OnMontageEnded.AddDynamic(this, &AShipActor::OnMontageEnded);
		UE_LOG(LogTemp, Log,
			TEXT("AShipActor [%s]: montage callback bound."), *GetName());
	}
	else
	{
		UE_LOG(LogTemp, Warning,
			TEXT("AShipActor [%s]: no AnimInstance — assign ABP_Ship to ExteriorMesh."),
			*GetName());
	}

	FindAndBindBaseCamp();
	BP_RegisterWithGameMode();
}

// ─────────────────────────────────────────────────────────────────────────────
// ALARM
// ─────────────────────────────────────────────────────────────────────────────

void AShipActor::StartAlarm()
{
	if (bAlarmActive) return;
	bAlarmActive = true;

	// Set all lights to alarm red immediately, then start pulsing.
	SetAllLightsState(AlarmIntensity, AlarmColor);
	SchedulePulseOff();

	BP_OnAlarmStarted();
	UE_LOG(LogTemp, Log, TEXT("AShipActor [%s]: alarm started."), *GetName());
}

void AShipActor::StopAlarm()
{
	if (!bAlarmActive) return;
	bAlarmActive = false;

	GetWorld()->GetTimerManager().ClearTimer(PulseTimerHandle);

	// Restore every light to its original intensity AND color.
	for (int32 i = 0; i < AlarmLights.Num(); ++i)
	{
		if (!AlarmLights[i]) continue;
		AlarmLights[i]->SetIntensity(
			OriginalIntensities.IsValidIndex(i) ? OriginalIntensities[i] : 0.f);
		AlarmLights[i]->SetLightColor(
			OriginalColors.IsValidIndex(i) ? OriginalColors[i] : FLinearColor::White);
	}

	BP_OnAlarmStopped();
	UE_LOG(LogTemp, Log, TEXT("AShipActor [%s]: alarm stopped, lights restored."), *GetName());
}

// ─────────────────────────────────────────────────────────────────────────────
// ANIMATION
// ─────────────────────────────────────────────────────────────────────────────

void AShipActor::OnPlayerEnteredCamp()
{
	bShouldBeOpen = true;
	TryUpdateAnimation();
}

void AShipActor::OnPlayerExitedCamp()
{
	bShouldBeOpen = false;
	TryUpdateAnimation();
}

void AShipActor::TryUpdateAnimation()
{
	if (bIsAnimationPlaying) return;
	if (bShouldBeOpen == bCurrentlyOpen) return;

	bIsAnimationPlaying = true;

	if (bShouldBeOpen)
	{
		UE_LOG(LogTemp, Log, TEXT("AShipActor [%s]: → OPEN"), *GetName());
		PlayMontage(OpenMontage);
	}
	else
	{
		UE_LOG(LogTemp, Log, TEXT("AShipActor [%s]: → CLOSE"), *GetName());
		PlayMontage(CloseMontage);
	}
}

void AShipActor::PlayMontage(UAnimMontage* Montage)
{
	if (!Montage)
	{
		UE_LOG(LogTemp, Warning,
			TEXT("AShipActor [%s]: montage null — set OpenMontage/CloseMontage in Class Defaults."),
			*GetName());
		bIsAnimationPlaying = false;
		bCurrentlyOpen = bShouldBeOpen;
		if (bCurrentlyOpen) BP_OnShipOpened(); else BP_OnShipClosed();
		return;
	}

	UAnimInstance* AnimInst = ExteriorMesh->GetAnimInstance();
	if (!AnimInst)
	{
		UE_LOG(LogTemp, Error,
			TEXT("AShipActor [%s]: no AnimInstance — ExteriorMesh needs ABP_Ship."),
			*GetName());
		bIsAnimationPlaying = false;
		return;
	}

	AnimInst->Montage_Play(Montage, 1.f);
}

void AShipActor::OnMontageEnded(UAnimMontage* Montage, bool bInterrupted)
{
	if (Montage != OpenMontage && Montage != CloseMontage) return;

	bCurrentlyOpen      = bShouldBeOpen;
	bIsAnimationPlaying = false;

	UE_LOG(LogTemp, Log,
		TEXT("AShipActor [%s]: montage ended (interrupted=%d). Open=%d"),
		*GetName(), bInterrupted, bCurrentlyOpen);

	if (bCurrentlyOpen) BP_OnShipOpened(); else BP_OnShipClosed();
	TryUpdateAnimation();
}

// ─────────────────────────────────────────────────────────────────────────────
// CAMP BINDING
// ─────────────────────────────────────────────────────────────────────────────

void AShipActor::FindAndBindBaseCamp()
{
	CachedBaseCamp = Cast<ABaseCamp>(
		UGameplayStatics::GetActorOfClass(this, ABaseCamp::StaticClass()));

	if (!CachedBaseCamp)
	{
		UE_LOG(LogTemp, Warning,
			TEXT("AShipActor [%s]: BaseCamp not found."), *GetName());
		return;
	}

	CachedBaseCamp->OnPlayerEnteredCamp.AddDynamic(
		this, &AShipActor::HandlePlayerEnteredCamp);
	CachedBaseCamp->OnPlayerExitedCamp.AddDynamic(
		this, &AShipActor::HandlePlayerExitedCamp);

	// Handle spawn-inside-bubble case.
	if (CachedBaseCamp->OxygenSphere)
	{
		TArray<AActor*> Overlapping;
		CachedBaseCamp->OxygenSphere->GetOverlappingActors(
			Overlapping, AAlphaExilemetCharacter::StaticClass());

		if (Overlapping.Num() > 0)
		{
			UE_LOG(LogTemp, Log,
				TEXT("AShipActor [%s]: player inside bubble at spawn — opening."), *GetName());
			bShouldBeOpen = true;
			TryUpdateAnimation();
		}
	}

	UE_LOG(LogTemp, Log,
		TEXT("AShipActor [%s]: bound to BaseCamp."), *GetName());
}

void AShipActor::HandlePlayerEnteredCamp() { OnPlayerEnteredCamp(); }
void AShipActor::HandlePlayerExitedCamp()  { OnPlayerExitedCamp();  }

// ─────────────────────────────────────────────────────────────────────────────
// LIGHT HELPERS
// ─────────────────────────────────────────────────────────────────────────────

void AShipActor::CollectAlarmLightsByTag()
{
	AlarmLights.Empty();
	TArray<ULightComponent*> All;
	GetComponents<ULightComponent>(All);
	for (ULightComponent* LC : All)
		if (LC && LC->ComponentTags.Contains(AlarmLightTag))
			AlarmLights.Add(LC);

	UE_LOG(LogTemp, Log,
		TEXT("AShipActor [%s]: %d alarm light(s) with tag '%s'."),
		*GetName(), AlarmLights.Num(), *AlarmLightTag.ToString());
}

void AShipActor::CacheOriginalLightState()
{
	OriginalIntensities.Empty();
	OriginalColors.Empty();

	for (ULightComponent* LC : AlarmLights)
	{
		OriginalIntensities.Add(LC ? LC->Intensity : 0.f);
		// GetLightColor returns FLinearColor
		OriginalColors.Add(LC ? LC->GetLightColor() : FLinearColor::White);
	}
}

void AShipActor::SetAllLightsState(float Intensity, FLinearColor Color)
{
	for (ULightComponent* LC : AlarmLights)
	{
		if (!LC) continue;
		LC->SetIntensity(Intensity);
		LC->SetLightColor(Color);
	}
}

void AShipActor::SchedulePulseOff()
{
	GetWorld()->GetTimerManager().SetTimer(
		PulseTimerHandle, this, &AShipActor::PulseOff, PulseOnTime, false);
}

void AShipActor::PulseOff()
{
	if (!bAlarmActive) return;
	// OFF state: intensity 0, color doesn't matter
	SetAllLightsState(0.f, AlarmColor);
	GetWorld()->GetTimerManager().SetTimer(
		PulseTimerHandle, this, &AShipActor::PulseOn, PulseOffTime, false);
}

void AShipActor::PulseOn()
{
	if (!bAlarmActive) return;
	SetAllLightsState(AlarmIntensity, AlarmColor);
	GetWorld()->GetTimerManager().SetTimer(
		PulseTimerHandle, this, &AShipActor::PulseOff, PulseOnTime, false);
}