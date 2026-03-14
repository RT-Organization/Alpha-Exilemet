#include "GasRodTool.h"

AGasRodTool::AGasRodTool()
{
	PrimaryActorTick.bCanEverTick = true;
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
/*         STAT UPGRADES         */
/* ----------------------------- */

void AGasRodTool::UpgradeStat(FName StatName)
{
	// 1. Call the parent function so ToolBase saves the level internally
	Super::UpgradeStat(StatName);

	// 2. Add your buffs here!
	if (StatName == "Rod_AbsSpeed")
	{
		ABS += 1;
	}
	else if (StatName == "Rod_Distance")
	{
		RNG += 1;
	}
	else if (StatName == "Rod_Quantity")
	{
		MAX += 1;
	}
}