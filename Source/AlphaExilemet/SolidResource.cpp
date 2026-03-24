// Fill out your copyright notice in the Description page of Project Settings.


#include "SolidResource.h"

#include "DroppedSolidResource.h"

ASolidResource::ASolidResource()
{
	OreMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("OreMesh"));
	OreMesh->SetupAttachment(RootComponent);

	OreMesh->SetCollisionProfileName(TEXT("BlockAll"));
}

void ASolidResource::UpdateScale()
{
	float HealthRatio = Health / InitialHealth;
	
	HealthRatio = FMath::Clamp(HealthRatio, 0.0f, 1.0f);
	
	float ScaledValue = FMath::Lerp(0.5f, 1.0f, HealthRatio);

	SetActorScale3D(FVector(ScaledValue));
}

void ASolidResource::DepleteResource()
{
	Super::DepleteResource();
	
	SpawnDroppedResource();
}

void ASolidResource::SpawnDroppedResource()
{
	UStaticMesh* MeshAsset = OreMesh->GetStaticMesh();
	if (!MeshAsset) return;

	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride =
		ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

	ADroppedSolidResource* Dropped = GetWorld()->SpawnActor<ADroppedSolidResource>(
		ADroppedSolidResource::StaticClass(),
		GetActorLocation(),
		FRotator::ZeroRotator,
		Params
	);
	
	if (!Dropped) return;

	Dropped->InitDroppedResource(MeshAsset, ResourceID);
	Dropped->SetActorScale3D(GetActorScale3D());
	
	UStaticMeshComponent* MeshComp = Dropped->FindComponentByClass<UStaticMeshComponent>();
	if (MeshComp && MeshComp->IsSimulatingPhysics())
	{
		FVector Direction;
		
		if (DropImpulseAngle >= 360.f)
		{
			Direction = FMath::VRand();
		}
		else if (DropImpulseAngle <= 0.f)
		{
			Direction = FVector::UpVector;
		}
		else
		{
			float HalfAngleRad = FMath::DegreesToRadians(DropImpulseAngle);
			Direction = FMath::VRandCone(FVector::UpVector, HalfAngleRad);
		}
		
		FVector Impulse = Direction * DropImpulseStrength;
		
		MeshComp->AddImpulse(Impulse, NAME_None, true);
	}
}