#include "GasRodTool.h"

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
/* STAT UPGRADES         */
/* ----------------------------- */

void AGasRodTool::UpgradeStat(FName StatName)
{
	// 1. Call the parent function so ToolBase saves the level internally
	Super::UpgradeStat(StatName);

	// 2. Increment specific levels
	if (StatName == "Rod_AbsSpeed") AbsSpeedLevel++;
	else if (StatName == "Rod_Distance") RangeLevel++;
	else if (StatName == "Rod_Quantity") CapacityLevel++;
}

float AGasRodTool::GetAbsorptionSpeed() const
{
	return AbsSpeedProgression.GetValueAtLevel(AbsSpeedLevel);
}

float AGasRodTool::GetRodRange() const
{
	return RangeProgression.GetValueAtLevel(RangeLevel);
}

float AGasRodTool::GetMaxCapacity() const
{
	return CapacityProgression.GetValueAtLevel(CapacityLevel);
}

/* ----------------------------- */
/* INVENTORY			         */
/* ----------------------------- */
void AGasRodTool::ClearInventory()
{
	HarvestedGas.Empty();
}