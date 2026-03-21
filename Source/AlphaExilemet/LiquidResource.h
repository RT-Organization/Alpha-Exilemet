#pragma once

#include "CoreMinimal.h"
#include "ResourceBase.h"
#include "LiquidResource.generated.h"

UCLASS()
class ALPHAEXILEMET_API ALiquidResource : public AResourceBase
{
	GENERATED_BODY()
	
public:	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Liquid")
	UStaticMeshComponent* SlimeMesh;
	
	ALiquidResource();
};