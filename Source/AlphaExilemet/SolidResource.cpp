// Fill out your copyright notice in the Description page of Project Settings.


#include "SolidResource.h"

#include "AlphaExilemetTypes.h"
#include "DroppedSolidResource.h"

ASolidResource::ASolidResource()
{
	OreMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("OreMesh"));
	OreMesh->SetupAttachment(RootComponent);

	OreMesh->SetCollisionProfileName(TEXT("BlockAll"));
}

void ASolidResource::SetLastPickaxe(APickaxeTool* Tool)
{
	LastPickaxe = Tool;
}

void ASolidResource::UpdateScale()
{
	float HealthRatio = Health / InitialHealth;
	
	HealthRatio = FMath::Clamp(HealthRatio, 0.0f, 1.0f);
	
	float ScaledValue = FMath::Lerp(0.5f, 1.0f, HealthRatio);

	SetActorScale3D(ScaledValue * InitialScale);
}

void ASolidResource::DepleteResource()
{
	Super::DepleteResource();
	
	const FResourceRow* Row = ResourceID.GetRow<FResourceRow>(TEXT("SolidResource"));
	if (!Row) return;
	
	int32 BaseMaxDrops = Row->MaxDrops;
	int32 LuckBonus = 0;
	if (LastPickaxe)
	{
		LuckBonus = FMath::FloorToInt(LastPickaxe->GetMiningLuck());
	}
	int32 MaxDropsWithLuck = BaseMaxDrops + LuckBonus;
	int32 DropCount = FMath::RandRange(1, MaxDropsWithLuck);
	for (int32 i = 0; i < DropCount; i++)
	{
		SpawnDroppedResource();
	}
	
	SetLastPickaxe(nullptr);
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