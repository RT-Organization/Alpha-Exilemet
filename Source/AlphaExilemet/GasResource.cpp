#include "GasResource.h"
#include "NiagaraComponent.h"
#include "Components/SphereComponent.h"

AGasResource::AGasResource()
{
	// 1. Setup the invisible hitbox
	GasHitbox = CreateDefaultSubobject<USphereComponent>(TEXT("GasHitbox"));
	GasHitbox->SetupAttachment(RootComponent);
	GasHitbox->InitSphereRadius(100.f);
	GasHitbox->SetCollisionProfileName(TEXT("OverlapAllDynamic"));

	// 2. Setup the visual effect
	GasVFX = CreateDefaultSubobject<UNiagaraComponent>(TEXT("GasVFX"));
	GasVFX->SetupAttachment(GasHitbox);
}