#include "GasRodTool.h"
#include "AlphaExilemetSaveGame.h"

AGasRodTool::AGasRodTool()
{
	PrimaryActorTick.bCanEverTick = true;
	
	CapacityProgression.BaseValue = 3.0f;
	CapacityProgression.AdditivePerLevel = 1.0f;

	AbsSpeedProgression.BaseValue = 10.0f;
	AbsSpeedProgression.AdditivePerLevel = 2.0f; 

	RangeProgression.BaseValue = 1000.0f;
	RangeProgression.AdditivePerLevel = 200.0f;  
}

void AGasRodTool::BeginPlay()
{
	Super::BeginPlay();
}

void AGasRodTool::StartUsing_Implementation()
{
	Super::StartUsing_Implementation();
	// TODO: Implement casting logic to capture Gas spheres
}

void AGasRodTool::StopUsing_Implementation()
{
	Super::StopUsing_Implementation();
	// TODO: Implement reeling in logic
}

/* ----------------------------- */
/* STAT UPGRADES                 */
/* ----------------------------- */

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