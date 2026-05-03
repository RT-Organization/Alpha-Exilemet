#include "GasResource.h"
#include "AlphaExilemet/Tools/GasSphere.h"
#include "NiagaraComponent.h"
#include "Components/SphereComponent.h"

AGasResource::AGasResource()
{
	PrimaryActorTick.bCanEverTick = true;

	GasHitbox = CreateDefaultSubobject<USphereComponent>(TEXT("GasHitbox"));
	GasHitbox->SetupAttachment(RootComponent);
	GasHitbox->InitSphereRadius(100.f);
	GasHitbox->SetCollisionProfileName(TEXT("OverlapAllDynamic"));

	GasVFX = CreateDefaultSubobject<UNiagaraComponent>(TEXT("GasVFX"));
	GasVFX->SetupAttachment(GasHitbox);
}

FName AGasResource::GetGasType() const
{
	return ResourceID.RowName;
}

// =========================================================================
// RESERVATION
// =========================================================================

bool AGasResource::TryReserve(AGasSphere* Sphere)
{
	if (!Sphere) return false;

	if (CurrentSphere && CurrentSphere != Sphere)
	{
		return false;
	}

	CurrentSphere = Sphere;
	return true;
}

void AGasResource::ReleaseReservation()
{
	CurrentSphere = nullptr;
}

// =========================================================================
// SCALE (TWEENED)
// =========================================================================

void AGasResource::UpdateScale()
{
	float HealthRatio = Health / InitialHealth;
	HealthRatio = FMath::Clamp(HealthRatio, 0.1f, 1.f);

	TargetScale = InitialScale * HealthRatio;
}

void AGasResource::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	FVector Current = GetActorScale3D();
	FVector NewScale = FMath::VInterpTo(Current, TargetScale, DeltaTime, ScaleInterpSpeed);

	SetActorScale3D(NewScale);
}

// =========================================================================
// DEPLETION
// =========================================================================

void AGasResource::DepleteResource()
{
	Super::DepleteResource();

	ReleaseReservation();

	if (GasVFX)
	{
		GasVFX->Deactivate();
	}
}