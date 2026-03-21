#include "LiquidResource.h"

ALiquidResource::ALiquidResource()
{
	SlimeMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("SlimeMesh"));
	SlimeMesh->SetupAttachment(RootComponent);
	
}