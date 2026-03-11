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
	
	// Sphere defining the oxygen regeneration area
	UPROPERTY(EditAnywhere, Category="Base")
	USphereComponent* OxygenSphere;

	// Overlap function declarations
	UFUNCTION()
	void OnOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	UFUNCTION()
	void OnOverlapEnd(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);
	
public:
	// Radius in world units for oxygen regeneration
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Base")
	float OxygenRegenRadius;
	
	// This runs whenever you change a variable in the editor
	virtual void OnConstruction(const FTransform& Transform) override;
};