#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "AlphaExilemetTypes.h"
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

	// -------------------------------------------------------------------------
	// SHIP REPAIR PROGRESSION
	// -------------------------------------------------------------------------
	
	// Maps the Ship System to its current repair level
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Base|Ship Repairs")
	TMap<EShipSystem, int32> ShipRepairLevels;

	// Safely gets the current level of a specific system
	UFUNCTION(BlueprintPure, Category = "Base|Ship Repairs")
	int32 GetShipSystemLevel(EShipSystem SystemID);

	// Increments the level of a specific system
	UFUNCTION(BlueprintCallable, Category = "Base|Ship Repairs")
	void UpgradeShipSystem(EShipSystem SystemID);
};