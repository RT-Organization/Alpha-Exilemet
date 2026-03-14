#include "VacuumTool.h"

AVacuumTool::AVacuumTool()
{
	PrimaryActorTick.bCanEverTick = true;
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
/*         STAT UPGRADES         */
/* ----------------------------- */

void AVacuumTool::UpgradeStat(FName StatName)
{
	// 1. Call the parent function so ToolBase saves the level internally
	Super::UpgradeStat(StatName);

	// 2. Add your buffs here!
	if (StatName == "Vacuum_Speed")
	{
		SPD += 1;
	}
	else if (StatName == "Vacuum_Capacity")
	{
		CAP += 1;
	}
	else if (StatName == "Vacuum_Distance")
	{
		RNG += 1;
	}
}