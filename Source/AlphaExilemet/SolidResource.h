// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "ResourceBase.h"
#include "SolidResource.generated.h"

/**
 * 
 */
UCLASS()
class ALPHAEXILEMET_API ASolidResource : public AResourceBase
{
	GENERATED_BODY()
	
public:	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Solid")
	UStaticMeshComponent* OreMesh;
	
	// Sets default values for this actor's properties
	ASolidResource();
};
