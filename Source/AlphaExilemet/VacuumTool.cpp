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