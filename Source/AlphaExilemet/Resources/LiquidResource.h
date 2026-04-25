#pragma once

#include "CoreMinimal.h"
#include "ResourceBase.h"
#include "LiquidResource.generated.h"

UCLASS()
class ALPHAEXILEMET_API ALiquidResource : public AResourceBase
{
	GENERATED_BODY()
	
public:	
	ALiquidResource();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Liquid")
	UStaticMeshComponent* LiquidMesh;

	UFUNCTION(BlueprintCallable, Category="Liquid")
	float DrainLiquid(float Amount);

	UFUNCTION(BlueprintPure, Category="Liquid")
	FName GetLiquidType() const;
};