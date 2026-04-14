// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "DroppedSolidResource.h"
#include "PickaxeTool.h"
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
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Drop")
	float DropImpulseStrength = 300.f;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Drop", meta=(ClampMin="0.0", ClampMax="360.0"))
	float DropImpulseAngle = 45.f;
	
	// Sets default values for this actor's properties
	ASolidResource();
	
	UFUNCTION(BlueprintCallable, Category="Solid|Mining")
	void SetLastPickaxe(APickaxeTool* Tool);
protected:
	UPROPERTY()
	class APickaxeTool* LastPickaxe;
	
	virtual void UpdateScale() override;
	
	virtual void DepleteResource() override;
	
	void SpawnDroppedResource();
};
