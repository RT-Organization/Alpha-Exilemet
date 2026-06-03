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

	CollectAlarmLightsByTag();
	CacheOriginalLightState();

	if (!ExteriorMesh->GetAnimInstance())
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
	SetAllLightsState(AlarmIntensity, AlarmColor);
	SchedulePulseOff();
	BP_OnAlarmStarted();
}

void AShipActor::StopAlarm()
{
	if (!bAlarmActive) return;
	bAlarmActive = false;
	GetWorld()->GetTimerManager().ClearTimer(PulseTimerHandle);

	for (int32 i = 0; i < AlarmLights.Num(); ++i)
	{
		if (!AlarmLights[i]) continue;
		AlarmLights[i]->SetIntensity(OriginalIntensities.IsValidIndex(i) ? OriginalIntensities[i] : 0.f);
		AlarmLights[i]->SetLightColor(OriginalColors.IsValidIndex(i) ? OriginalColors[i] : FLinearColor::White);
		AlarmLights[i]->MarkRenderStateDirty();
	}

	BP_OnAlarmStopped();
}

// ─────────────────────────────────────────────────────────────────────────────
// ANIMATION
// ─────────────────────────────────────────────────────────────────────────────

void AShipActor::OnPlayerEnteredCamp()
{
	UE_LOG(LogTemp, Warning, TEXT("AShipActor [%s]: OnPlayerEnteredCamp"), *GetName());
	bShouldBeOpen = true;
	TryUpdateAnimation();
}

void AShipActor::OnPlayerExitedCamp()
{
	UE_LOG(LogTemp, Warning, TEXT("AShipActor [%s]: OnPlayerExitedCamp"), *GetName());
	bShouldBeOpen = false;
	TryUpdateAnimation();
}

void AShipActor::TryUpdateAnimation()
{
	if (bIsAnimationPlaying) return;
	if (bShouldBeOpen == bCurrentlyOpen) return;

	bIsAnimationPlaying = true;

	if (bShouldBeOpen)
		PlayMontage(OpenMontage);
	else
		PlayMontage(CloseMontage);
}

void AShipActor::PlayMontage(UAnimMontage* Montage)
{
	UE_LOG(LogTemp, Warning, TEXT("AShipActor [%s]: PlayMontage — %s"),
		*GetName(), Montage ? *Montage->GetName() : TEXT("NULL"));

	if (!Montage)
	{
		bIsAnimationPlaying = false;
		bCurrentlyOpen = bShouldBeOpen;
		if (bCurrentlyOpen) BP_OnShipOpened(); else BP_OnShipClosed();
		return;
	}

	UAnimInstance* AnimInst = ExteriorMesh->GetAnimInstance();
	if (!AnimInst)
	{
		bIsAnimationPlaying = false;
		return;
	}

	float Duration = AnimInst->Montage_Play(Montage, 1.f);
	UE_LOG(LogTemp, Warning, TEXT("AShipActor [%s]: Montage_Play duration=%.3f"), *GetName(), Duration);

	if (Duration <= 0.f)
	{
		UE_LOG(LogTemp, Error, TEXT("AShipActor [%s]: Montage_Play returned 0 — check slot DefaultSlot/DefaultGroup."), *GetName());
		bIsAnimationPlaying = false;
		bCurrentlyOpen = bShouldBeOpen;
		if (bCurrentlyOpen) BP_OnShipOpened(); else BP_OnShipClosed();
		return;
	}

	// Auto Blend Out is DISABLED on the montage asset, so OnMontageEnded will
	// never fire naturally. Instead we schedule a timer to fire one frame
	// before the end so we can update state and fire BP events cleanly.
	// The montage just holds its last frame forever after that — no snap back.
	const float FireAt = FMath::Max(Duration - 0.05f, Duration * 0.99f);
	GetWorld()->GetTimerManager().SetTimer(
		MontageEndTimerHandle, this, &AShipActor::OnMontageReachedEnd, FireAt, false);

	UE_LOG(LogTemp, Warning, TEXT("AShipActor [%s]: end timer set for %.3fs"), *GetName(), FireAt);
}

bool AShipActor::IsOurMontage(UAnimMontage* Montage) const
{
	if (!Montage) return false;
	if (Montage == OpenMontage || Montage == CloseMontage) return true;
	const FString Name = Montage->GetName();
	if (OpenMontage  && Name == OpenMontage->GetName())  return true;
	if (CloseMontage && Name == CloseMontage->GetName()) return true;
	return false;
}

void AShipActor::OnMontageReachedEnd()
{
	UE_LOG(LogTemp, Warning, TEXT("AShipActor [%s]: OnMontageReachedEnd — bShouldBeOpen=%d"),
		*GetName(), bShouldBeOpen);

	bCurrentlyOpen      = bShouldBeOpen;
	bIsAnimationPlaying = false;

	if (bCurrentlyOpen) BP_OnShipOpened(); else BP_OnShipClosed();

	// If the desired state changed while animating, kick off the next animation
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
		UE_LOG(LogTemp, Warning, TEXT("AShipActor [%s]: BaseCamp not found."), *GetName());
		return;
	}

	CachedBaseCamp->OnPlayerEnteredCamp.AddDynamic(this, &AShipActor::HandlePlayerEnteredCamp);
	CachedBaseCamp->OnPlayerExitedCamp.AddDynamic(this,  &AShipActor::HandlePlayerExitedCamp);

	// Delay 0.1s so physics overlaps are populated before checking
	GetWorld()->GetTimerManager().SetTimer(
		SpawnCheckTimerHandle, this, &AShipActor::CheckInitialOverlap, 0.1f, false);

	UE_LOG(LogTemp, Log, TEXT("AShipActor [%s]: bound to BaseCamp."), *GetName());
}

void AShipActor::CheckInitialOverlap()
{
	if (!CachedBaseCamp || !CachedBaseCamp->OxygenSphere) return;

	TArray<AActor*> Overlapping;
	CachedBaseCamp->OxygenSphere->GetOverlappingActors(Overlapping, AAlphaExilemetCharacter::StaticClass());

	UE_LOG(LogTemp, Warning, TEXT("AShipActor [%s]: CheckInitialOverlap — %d inside bubble."),
		*GetName(), Overlapping.Num());

	if (Overlapping.Num() > 0)
	{
		bShouldBeOpen = true;
		TryUpdateAnimation();
	}
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

	UE_LOG(LogTemp, Log, TEXT("AShipActor [%s]: %d alarm light(s) with tag '%s'."),
		*GetName(), AlarmLights.Num(), *AlarmLightTag.ToString());
}

void AShipActor::CacheOriginalLightState()
{
	OriginalIntensities.Empty();
	OriginalColors.Empty();
	for (ULightComponent* LC : AlarmLights)
	{
		OriginalIntensities.Add(LC ? LC->Intensity : 0.f);
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
		LC->MarkRenderStateDirty();
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