// Fill out your copyright notice in the Description page of Project Settings.


#include "SolidResource.h"

#include "DroppedSolidResource.h"

ASolidResource::ASolidResource()
{
	OreMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("OreMesh"));
	OreMesh->SetupAttachment(RootComponent);

	OreMesh->SetCollisionProfileName(TEXT("BlockAll"));
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
}