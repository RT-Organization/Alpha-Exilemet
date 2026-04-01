#include "VacuumTool.h"
#include "LiquidResource.h"

#include "GameFramework/Character.h"
#include "GameFramework/PlayerController.h"
#include "Camera/PlayerCameraManager.h"
#include "TimerManager.h"

AVacuumTool::AVacuumTool()
{
	PrimaryActorTick.bCanEverTick = true;

	CapacityProgression.BaseValue = 100.0f;
	CapacityProgression.AdditivePerLevel = 50.0f;

	SpeedProgression.BaseValue = 15.0f;
	SpeedProgression.AdditivePerLevel = 5.0f;

	RangeProgression.BaseValue = 600.0f;
	RangeProgression.AdditivePerLevel = 100.0f;
}

void AVacuumTool::BeginPlay()
{
	Super::BeginPlay();
}

/* ----------------------------- */
/* INPUT                         */
/* ----------------------------- */

void AVacuumTool::StartUsing_Implementation()
{
	Super::StartUsing_Implementation();
	StartVacuumTimer();
}

void AVacuumTool::StopUsing_Implementation()
{
	Super::StopUsing_Implementation();
	StopVacuumTimer();
}

/* ----------------------------- */
/* TIMER                         */
/* ----------------------------- */

void AVacuumTool::StartVacuumTimer()
{
	GetWorldTimerManager().SetTimer(
		VacuumTimer,
		this,
		&AVacuumTool::PerformVacuumTrace,
		GetAbsorptionInterval(),
		true
	);
}

void AVacuumTool::StopVacuumTimer()
{
	GetWorldTimerManager().ClearTimer(VacuumTimer);
}

/* ----------------------------- */
/* TRACE                         */
/* ----------------------------- */

void AVacuumTool::PerformVacuumTrace()
{
	if (GetCurrentStoredSlime() >= FMath::FloorToInt(GetMaxCapacity()))
	{
		// Optional: trigger "tank full" feedback
		return;
	}

	if (!OwnerCharacter)
	{
		OwnerCharacter = Cast<ACharacter>(GetOwner());
		if (!OwnerCharacter) return;
	}

	APlayerController* PC = Cast<APlayerController>(OwnerCharacter->GetController());
	if (!PC) return;

	APlayerCameraManager* CameraManager = PC->PlayerCameraManager;
	if (!CameraManager) return;

	FVector Start = CameraManager->GetCameraLocation();
	FVector Forward = CameraManager->GetCameraRotation().Vector();
	FVector End = Start + Forward * GetVacuumRange();

	FHitResult Hit;
	FCollisionQueryParams Params;
	Params.AddIgnoredActor(this);
	Params.AddIgnoredActor(OwnerCharacter);

	if (!GetWorld()->LineTraceSingleByChannel(Hit, Start, End, ECC_Visibility, Params))
		return;

	ALiquidResource* Liquid = Cast<ALiquidResource>(Hit.GetActor());
	if (!Liquid) return;
	
	OnLiquidHitting(Liquid);

	float Extracted = Liquid->DrainLiquid(AbsorptionDamagePerTick);

	AbsorbSlime(Liquid->GetLiquidType(), Extracted);
}

/* ----------------------------- */
/* ABSORPTION                    */
/* ----------------------------- */

void AVacuumTool::AbsorbSlime(FName SlimeType, float Amount)
{
	if (Amount <= 0.f) return;

	float MaxCapacity = GetMaxCapacity();
	float Current = GetCurrentStoredSlime();

	float Available = MaxCapacity - Current;
	if (Available <= 0.f) return;

	float Actual = FMath::Min(Amount, Available);

	int32 IntAmount = FMath::FloorToInt(Actual);
	if (IntAmount <= 0) return;

	HarvestedSlime.FindOrAdd(SlimeType) += IntAmount;
}

/* ----------------------------- */
/* INVENTORY                     */
/* ----------------------------- */

float AVacuumTool::GetCurrentStoredSlime() const
{
	int32 Total = 0;

	for (const auto& Pair : HarvestedSlime)
	{
		Total += Pair.Value;
	}

	return (float)Total;
}

float AVacuumTool::GetFillPercent() const
{
	return GetCurrentStoredSlime() / GetMaxCapacity();
}

/* ----------------------------- */
/* STATS                         */
/* ----------------------------- */

void AVacuumTool::UpgradeStat(FName StatName)
{
	Super::UpgradeStat(StatName);

	if (StatName == "Vacuum_Speed") SpeedLevel++;
	else if (StatName == "Vacuum_Capacity") CapacityLevel++;
	else if (StatName == "Vacuum_Distance") RangeLevel++;
}

float AVacuumTool::GetAbsorptionInterval() const
{
	float Rate = SpeedProgression.GetValueAtLevel(SpeedLevel);
	return FMath::Max(0.01f, 1.0f / Rate);
}

float AVacuumTool::GetVacuumRange() const
{
	return RangeProgression.GetValueAtLevel(RangeLevel);
}

float AVacuumTool::GetMaxCapacity() const
{
	return CapacityProgression.GetValueAtLevel(CapacityLevel);
}