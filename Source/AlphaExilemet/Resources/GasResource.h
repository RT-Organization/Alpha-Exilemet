#pragma once

#include "CoreMinimal.h"
#include "ResourceBase.h"
#include "GasResource.generated.h"

class UNiagaraComponent;
class USphereComponent;

UCLASS()
class ALPHAEXILEMET_API AGasResource : public AResourceBase
{
	GENERATED_BODY()
	
public:	
	// The invisible hitbox so the Gas Rod knows what to grab!
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Gas")
	USphereComponent* GasHitbox;

	// The visual gas cloud effect
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Gas")
	UNiagaraComponent* GasVFX;
	
	AGasResource();
};