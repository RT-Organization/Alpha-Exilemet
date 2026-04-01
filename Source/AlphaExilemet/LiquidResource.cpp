#include "LiquidResource.h"
#include "TimerManager.h"

ALiquidResource::ALiquidResource()
{
	LiquidMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("LiquidMesh"));
	LiquidMesh->SetupAttachment(RootComponent);
}

/* ----------------------------- */
/* LIQUID LOGIC                  */
/* ----------------------------- */

float ALiquidResource::DrainLiquid(float Amount)
{
	if (bIsDepleted || Amount <= 0.f)
		return 0.f;

	float Actual = FMath::Min(Health, Amount);

	ApplyResourceDamage(Actual);

	return Actual;
}

FName ALiquidResource::GetLiquidType() const
{
	return ResourceID.RowName;
}