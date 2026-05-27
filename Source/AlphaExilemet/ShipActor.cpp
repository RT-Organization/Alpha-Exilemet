#include "ShipActor.h"
#include "BaseCamp.h"
#include "AlphaExilemetCharacter.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/LightComponent.h"
#include "Components/SphereComponent.h"
#include "Animation/AnimInstance.h"
#include "TimerManager.h"
#include "Kismet/GameplayStatics.h"

// ─────────────────────────────────────────────────────────────────────────────
// Constructor
// ─────────────────────────────────────────────────────────────────────────────

AShipActor::AShipActor()
{
	PrimaryActorTick.bCanEverTick = false;

	ExteriorMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("ExteriorMesh"));
	RootComponent = ExteriorMesh;

	InteriorMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("InteriorMesh"));
	InteriorMesh->SetupAttachment(RootComponent);
}

// ─────────────────────────────────────────────────────────────────────────────
// BeginPlay
// ─────────────────────────────────────────────────────────────────────────────

void AShipActor::BeginPlay()
{
	Super::BeginPlay();

	CollectAlarmLightsByTag();
	CacheOriginalIntensities();
	SetAllLightsIntensity(0.f);

	// Bind the montage-ended callback on the ExteriorMesh AnimInstance once.
	// C++ owns the callback — BP never needs to wire Bind/Unbind manually.
	if (UAnimInstance* AnimInst = ExteriorMesh->GetAnimInstance())
	{
		AnimInst->OnMontageEnded.AddDynamic(this, &AShipActor::OnMontageEnded);
		UE_LOG(LogTemp, Log,
			TEXT("AShipActor [%s]: montage-ended callback bound."), *GetName());
	}
	else
	{
		UE_LOG(LogTemp, Warning,
			TEXT("AShipActor [%s]: ExteriorMesh has no AnimInstance at BeginPlay. "
			     "Make sure ExteriorMesh has an Animation Blueprint assigned that "
			     "contains a DefaultSlot node in its AnimGraph."), *GetName());
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
	PulseOn();
	BP_OnAlarmStarted();
	UE_LOG(LogTemp, Log, TEXT("AShipActor [%s]: alarm started."), *GetName());
}

void AShipActor::StopAlarm()
{
	if (!bAlarmActive) return;
	bAlarmActive = false;
	GetWorld()->GetTimerManager().ClearTimer(PulseTimerHandle);
	for (int32 i = 0; i < AlarmLights.Num(); ++i)
		if (AlarmLights[i])
			AlarmLights[i]->SetIntensity(
				OriginalIntensities.IsValidIndex(i) ? OriginalIntensities[i] : 0.f);
	BP_OnAlarmStopped();
	UE_LOG(LogTemp, Log, TEXT("AShipActor [%s]: alarm stopped."), *GetName());
}

// ─────────────────────────────────────────────────────────────────────────────
// ANIMATION — public interface
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

// ─────────────────────────────────────────────────────────────────────────────
// ANIMATION — state machine
// ─────────────────────────────────────────────────────────────────────────────

void AShipActor::TryUpdateAnimation()
{
	if (bIsAnimationPlaying) return;          // wait for current montage to end
	if (bShouldBeOpen == bCurrentlyOpen) return; // already in correct state

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
		// No montage assigned — skip animation but still update state so the
		// system doesn't get stuck. Fire the appropriate BP event immediately.
		UE_LOG(LogTemp, Warning,
			TEXT("AShipActor [%s]: montage is null — "
			     "assign OpenMontage/CloseMontage in Class Defaults."), *GetName());

		bIsAnimationPlaying = false;
		bCurrentlyOpen = bShouldBeOpen;
		if (bCurrentlyOpen) BP_OnShipOpened(); else BP_OnShipClosed();
		return;
	}

	UAnimInstance* AnimInst = ExteriorMesh->GetAnimInstance();
	if (!AnimInst)
	{
		UE_LOG(LogTemp, Error,
			TEXT("AShipActor [%s]: no AnimInstance — "
			     "ExteriorMesh needs an Animation Blueprint with a DefaultSlot node."),
			*GetName());
		bIsAnimationPlaying = false;
		return;
	}

	AnimInst->Montage_Play(Montage, 1.f);
}

// ─────────────────────────────────────────────────────────────────────────────
// ANIMATION — montage-ended callback (bound once in BeginPlay)
// ─────────────────────────────────────────────────────────────────────────────

void AShipActor::OnMontageEnded(UAnimMontage* Montage, bool bInterrupted)
{
	// Only react to our own montages — ignore anything from other systems.
	if (Montage != OpenMontage && Montage != CloseMontage) return;

	// If interrupted we still update state — the ship is wherever the
	// geometry ended up. TryUpdateAnimation will immediately start the
	// correct follow-up animation if desired state changed mid-play.
	bCurrentlyOpen      = bShouldBeOpen;
	bIsAnimationPlaying = false;

	UE_LOG(LogTemp, Log,
		TEXT("AShipActor [%s]: montage ended (interrupted=%d). "
		     "CurrentlyOpen=%d, ShouldBeOpen=%d"),
		*GetName(), bInterrupted, bCurrentlyOpen, bShouldBeOpen);

	// Fire BP notification so BP can enable/disable ramp collision, SFX, etc.
	if (bCurrentlyOpen) BP_OnShipOpened(); else BP_OnShipClosed();

	// Re-evaluate — handles the "player left during open animation" case.
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
			TEXT("AShipActor [%s]: BaseCamp not found — no proximity animation."),
			*GetName());
		return;
	}

	CachedBaseCamp->OnPlayerEnteredCamp.AddDynamic(
		this, &AShipActor::HandlePlayerEnteredCamp);
	CachedBaseCamp->OnPlayerExitedCamp.AddDynamic(
		this, &AShipActor::HandlePlayerExitedCamp);

	// ── INITIAL STATE CHECK ───────────────────────────────────────────────────
	// BaseCamp may have already broadcast OnPlayerEnteredCamp before we bound.
	// Query the sphere directly — this fixes the "spawn inside bubble" case.
	if (CachedBaseCamp->OxygenSphere)
	{
		TArray<AActor*> Overlapping;
		CachedBaseCamp->OxygenSphere->GetOverlappingActors(
			Overlapping, AAlphaExilemetCharacter::StaticClass());

		if (Overlapping.Num() > 0)
		{
			UE_LOG(LogTemp, Log,
				TEXT("AShipActor [%s]: player inside bubble at spawn — opening ship."),
				*GetName());
			bShouldBeOpen = true;
			TryUpdateAnimation();
		}
	}

	UE_LOG(LogTemp, Log,
		TEXT("AShipActor [%s]: bound to BaseCamp delegates."), *GetName());
}

void AShipActor::HandlePlayerEnteredCamp() { OnPlayerEnteredCamp(); }
void AShipActor::HandlePlayerExitedCamp()  { OnPlayerExitedCamp();  }

// ─────────────────────────────────────────────────────────────────────────────
// ALARM — internal helpers
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
		TEXT("AShipActor [%s]: %d alarm light(s) found with tag '%s'."),
		*GetName(), AlarmLights.Num(), *AlarmLightTag.ToString());
}

void AShipActor::CacheOriginalIntensities()
{
	OriginalIntensities.Empty();
	for (ULightComponent* LC : AlarmLights)
		OriginalIntensities.Add(LC ? LC->Intensity : 0.f);
}

void AShipActor::PulseOn()
{
	if (!bAlarmActive) return;
	SetAllLightsIntensity(AlarmIntensity);
	GetWorld()->GetTimerManager().SetTimer(
		PulseTimerHandle, this, &AShipActor::PulseOff, PulseOnTime, false);
}

void AShipActor::PulseOff()
{
	if (!bAlarmActive) return;
	SetAllLightsIntensity(0.f);
	GetWorld()->GetTimerManager().SetTimer(
		PulseTimerHandle, this, &AShipActor::PulseOn, PulseOffTime, false);
}

void AShipActor::SetAllLightsIntensity(float Intensity)
{
	for (ULightComponent* LC : AlarmLights)
		if (LC) LC->SetIntensity(Intensity);
}