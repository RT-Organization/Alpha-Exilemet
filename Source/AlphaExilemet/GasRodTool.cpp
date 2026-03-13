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