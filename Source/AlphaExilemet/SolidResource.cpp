// Fill out your copyright notice in the Description page of Project Settings.


#include "SolidResource.h"

ASolidResource::ASolidResource()
{
	OreMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("OreMesh"));
	OreMesh->SetupAttachment(RootComponent);

	OreMesh->SetCollisionProfileName(TEXT("BlockAll"));
}