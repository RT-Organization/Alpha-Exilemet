#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "BaseCamp.generated.h"

class USphereComponent;

UCLASS()
class ALPHAEXILEMET_API ABaseCamp : public AActor
{
	GENERATED_BODY()
	
public:
	ABaseCamp();
	
protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;
	
	// Sphere defining the oxygen regeneration area
	UPROPERTY(EditAnywhere, Category="Base")
	USphereComponent* OxygenSphere;
	
public:
	// Radius in world units for oxygen regeneration
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Base")
	float OxygenRegenRadius;
	
	// Amount of oxygen regained per second inside the base
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Base")
	float OxygenRegenRate;
	
	// This runs whenever you change a variable in the editor
	virtual void OnConstruction(const FTransform& Transform) override;
};