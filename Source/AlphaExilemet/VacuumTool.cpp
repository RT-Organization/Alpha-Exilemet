#include "VacuumTool.h"
#include "LiquidResource.h"

#include "GameFramework/Character.h"
#include "GameFramework/PlayerController.h"
#include "Camera/PlayerCameraManager.h"
#include "TimerManager.h"
#include "AlphaExilemetSaveGame.h"

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

void AVacuumTool::ClearInventory(float RetainedFraction)
{
	if (RetainedFraction <= 0.0f)
	{
		HarvestedSlime.Empty();
		return;
	}

	for (auto It = HarvestedSlime.CreateIterator(); It; ++It)
	{
		int32 RetainedAmount = FMath::FloorToInt(It.Value() * RetainedFraction);
		
		if (RetainedAmount > 0)
		{
			It.Value() = RetainedAmount;
		}
		else
		{
			It.RemoveCurrent();
		}
	}
}

/* ----------------------------- */
/* STATS                         */
/* ----------------------------- */

float AVacuumTool::GetAbsorptionInterval() const
{
	int32 Level = ToolUpgradeLevels.FindRef(FName("Vacuum_Speed"));
	float Rate = SpeedProgression.GetValueAtLevel(Level);
	return FMath::Max(0.01f, 1.0f / Rate);
}

float AVacuumTool::GetVacuumRange() const
{
	int32 Level = ToolUpgradeLevels.FindRef(FName("Vacuum_Distance"));
	return RangeProgression.GetValueAtLevel(Level);
}

float AVacuumTool::GetMaxCapacity() const
{
	int32 Level = ToolUpgradeLevels.FindRef(FName("Vacuum_Capacity"));
	return CapacityProgression.GetValueAtLevel(Level);
}

/* ----------------------------- */
/* SAVE & LOAD                   */
/* ----------------------------- */
void AVacuumTool::SaveToolData(UAlphaExilemetSaveGame* SaveObject)
{
	Super::SaveToolData(SaveObject);
	if (SaveObject) SaveObject->SavedHarvestedSlime = HarvestedSlime;
}

void AVacuumTool::LoadToolData(UAlphaExilemetSaveGame* SaveObject)
{
	Super::LoadToolData(SaveObject);
	if (SaveObject) HarvestedSlime = SaveObject->SavedHarvestedSlime;
}