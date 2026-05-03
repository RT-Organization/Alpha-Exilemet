#include "GasSphere.h"
#include "GasRodTool.h"
#include "AlphaExilemet/Resources/GasResource.h"
#include "AlphaExilemet/AlphaExilemetCharacter.h"
#include "Components/StaticMeshComponent.h"

AGasSphere::AGasSphere()
{
	PrimaryActorTick.bCanEverTick = true;

	Mesh = CreateDefaultSubobject<UStaticMeshComponent>("Mesh");
	SetRootComponent(Mesh);

	Mesh->SetSimulatePhysics(true);
	Mesh->SetCollisionProfileName(TEXT("PhysicsActor"));
}

void AGasSphere::BeginPlay()
{
	Super::BeginPlay();
}

void AGasSphere::InitSphere(AGasRodTool* InOwnerTool, float InDamagePerSecond)
{
	OwnerTool = InOwnerTool;
	DamagePerSecond = InDamagePerSecond;
}

void AGasSphere::TryAttachToGas(AGasResource* Gas)
{
	if (!Gas || bIsAttached || bIsFull) return;
	if (!Gas->TryReserve(this)) return;

	TargetGas = Gas;
	GasType = Gas->GetGasType();

	bIsAttached = true;

	Mesh->SetSimulatePhysics(false);
	SetActorLocation(Gas->GetActorLocation());
}

void AGasSphere::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (!bIsAttached || bIsFull || !TargetGas) return;

	bool bKilled = TargetGas->ApplyResourceDamage(DamagePerSecond * DeltaTime);

	if (bKilled)
	{
		bIsFull = true;

		if (TargetGas)
		{
			TargetGas->ReleaseReservation();
		}

		bIsAttached = false;
		TargetGas = nullptr;

		Mesh->SetSimulatePhysics(true);
	}
}

void AGasSphere::Interact_Implementation(AAlphaExilemetCharacter* Interactor)
{
	if (!bIsFull || !Interactor || !Interactor->CurrentTool) return;

	AGasRodTool* Rod = Cast<AGasRodTool>(Interactor->CurrentTool);
	if (!Rod) return;

	bool bAdded = Rod->ReturnFullSphere(GasType);

	if (bAdded)
	{
		Destroy();
	}
}

void AGasSphere::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);

	if (TargetGas)
	{
		TargetGas->ReleaseReservation();
	}
}