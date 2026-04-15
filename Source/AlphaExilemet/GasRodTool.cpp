#include "GasRodTool.h"
#include "AlphaExilemetSaveGame.h"

AGasRodTool::AGasRodTool()
{
	PrimaryActorTick.bCanEverTick = true;

	// Capacity: slots for Gas spheres — keep low so costs in Gas can stay low too
	CapacityProgression.BaseValue       = 1.0f;
	CapacityProgression.AdditivePerLevel = 1.0f;

	// Abs.Speed: damage per second dealt to the Gas resource while absorbing.
	// Harvest time (s) = Resource.Health / DamagePerSecond
	// GAS0 (50 HP) @ base: 50 / 10 = 5 s  — matches original design intent
	AbsSpeedProgression.BaseValue       = 10.0f;
	AbsSpeedProgression.AdditivePerLevel = 2.0f; 

	// Range: maximum distance to a Gas vent the rod can lock onto
	RangeProgression.BaseValue       = 1000.0f;
	RangeProgression.AdditivePerLevel = 200.0f;  
}

void AGasRodTool::BeginPlay()
{
	Super::BeginPlay();
}

void AGasRodTool::StartUsing_Implementation()
{
	Super::StartUsing_Implementation();
	// TODO: Cast the rod toward the Gas sphere and begin the absorption loop.
	//       On each 1-second tick call: GasSphere->ApplyResourceDamage(GetAbsorptionSpeed())
	//       Stop the loop (and succeed) when ApplyResourceDamage returns true (depleted).
	//       If the player moves out of range or releases input, abort and reset.
}

void AGasRodTool::StopUsing_Implementation()
{
	Super::StopUsing_Implementation();
	// TODO: Cancel any in-flight absorption timer / tween and reset the gas sphere.
}

/* ----------------------------- */
/* STAT GETTERS                  */
/* ----------------------------- */

// Returns damage per second at the current upgrade level.
// Harvest time (s) = Resource.Health / GetAbsorptionSpeed()
float AGasRodTool::GetAbsorptionSpeed() const
{
	int32 Level = ToolUpgradeLevels.FindRef(FName("Rod_AbsSpeed"));
	return AbsSpeedProgression.GetValueAtLevel(Level);
}

float AGasRodTool::GetRodRange() const
{
	int32 Level = ToolUpgradeLevels.FindRef(FName("Rod_Distance"));
	return RangeProgression.GetValueAtLevel(Level);
}

float AGasRodTool::GetMaxCapacity() const
{
	int32 Level = ToolUpgradeLevels.FindRef(FName("Rod_Quantity"));
	return CapacityProgression.GetValueAtLevel(Level);
}

/* ----------------------------- */
/* INVENTORY                     */
/* ----------------------------- */

void AGasRodTool::ClearInventory(float RetainedFraction)
{
	if (RetainedFraction <= 0.0f)
	{
		HarvestedGas.Empty();
		return;
	}

	for (auto It = HarvestedGas.CreateIterator(); It; ++It)
	{
		int32 RetainedAmount = FMath::FloorToInt(It.Value() * RetainedFraction);
		if (RetainedAmount > 0) It.Value() = RetainedAmount;
		else                    It.RemoveCurrent();
	}
}

/* ----------------------------- */
/* SAVE & LOAD                   */
/* ----------------------------- */

void AGasRodTool::SaveToolData(UAlphaExilemetSaveGame* SaveObject)
{
	Super::SaveToolData(SaveObject);
	if (SaveObject) SaveObject->SavedHarvestedGas = HarvestedGas;
}

void AGasRodTool::LoadToolData(UAlphaExilemetSaveGame* SaveObject)
{
	Super::LoadToolData(SaveObject);
	if (SaveObject) HarvestedGas = SaveObject->SavedHarvestedGas;
}