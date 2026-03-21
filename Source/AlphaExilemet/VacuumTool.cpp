#include "VacuumTool.h"

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

void AVacuumTool::StartUsing_Implementation()
{
	Super::StartUsing_Implementation();
	// TODO: Implement suction logic (e.g., raycast/sphere trace to pull Slime)
}

void AVacuumTool::StopUsing_Implementation()
{
	Super::StopUsing_Implementation();
	// TODO: Stop suction logic / Stop visual effects
}

/* ----------------------------- */
/* STAT UPGRADES				 */
/* ----------------------------- */

void AVacuumTool::UpgradeStat(FName StatName)
{
	// 1. Call the parent function so ToolBase saves the level internally
	Super::UpgradeStat(StatName);

	// 2. Increment specific levels
	if (StatName == "Vacuum_Speed") SpeedLevel++;
	else if (StatName == "Vacuum_Capacity") CapacityLevel++;
	else if (StatName == "Vacuum_Distance") RangeLevel++;
}

float AVacuumTool::GetVacuumSpeed() const
{
	return SpeedProgression.GetValueAtLevel(SpeedLevel);
}

float AVacuumTool::GetVacuumRange() const
{
	return RangeProgression.GetValueAtLevel(RangeLevel);
}

float AVacuumTool::GetMaxCapacity() const
{
	return CapacityProgression.GetValueAtLevel(CapacityLevel);
}