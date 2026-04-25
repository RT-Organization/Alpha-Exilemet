#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "AlphaExilemet/Data/AlphaExilemetTypes.h"
#include "BaseCamp.generated.h"

class USphereComponent;

UCLASS()
class ALPHAEXILEMET_API ABaseCamp : public AActor
{
	GENERATED_BODY()
	
public:
	ABaseCamp();
	
protected:
	// -------------------------------------------------------------------------
	// ENGINE OVERRIDES
	// -------------------------------------------------------------------------
	virtual void BeginPlay() override;
	virtual void OnConstruction(const FTransform& Transform) override;

public:
	// -------------------------------------------------------------------------
	// COMPONENTS
	// -------------------------------------------------------------------------
	// Sphere defining the oxygen regeneration area
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Base|Components")
	USphereComponent* OxygenSphere;

	// Radius in world units for oxygen regeneration
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Base|Components")
	float OxygenRegenRadius;

	// -------------------------------------------------------------------------
	// SHIP REPAIR PROGRESSION
	// -------------------------------------------------------------------------
	// Maps the Ship System to its current repair level
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Base|Ship Repairs")
	TMap<EShipSystem, int32> ShipRepairLevels;

	// Progression Structs for easy Editor tweaking
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Base|Ship Repairs|Progression")
	FStatProgression ScrubberProgression;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Base|Ship Repairs|Progression")
	FStatProgression ScannerProgression;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Base|Ship Repairs|Progression")
	FStatProgression DampenerProgression;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Base|Ship Repairs|Progression")
	FStatProgression RetrieverProgression;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Base|Ship Repairs|Progression")
	FStatProgression RefinerProgression;

	// Core Upgrade Methods
	UFUNCTION(BlueprintPure, Category = "Base|Ship Repairs")
	int32 GetShipSystemLevel(EShipSystem SystemID);

	UFUNCTION(BlueprintCallable, Category = "Base|Ship Repairs")
	void UpgradeShipSystem(EShipSystem SystemID);

	UFUNCTION(BlueprintCallable, Category = "Base|Ship Repairs")
	void ApplyShipUpgrades();
	
	// Called to sync the visual Forcefield with the physical sphere
	UFUNCTION(BlueprintImplementableEvent, Category = "Base|Ship Repairs")
	void BP_UpdateForcefieldRadius(float NewRadius);
	
	// -------------------------------------------------------------------------
	// SHOP PROGRESSION / UNLOCKS
	// -------------------------------------------------------------------------
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Base|Shop Unlocks")
	TArray<EToolType> UnlockedTools;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Base|Shop Unlocks")
	TArray<ESpecialItem> UnlockedSpecialItems;

	UFUNCTION(BlueprintPure, Category = "Base|Shop Unlocks")
	bool IsToolUnlocked(EToolType ToolID);

	UFUNCTION(BlueprintPure, Category = "Base|Shop Unlocks")
	bool IsSpecialItemUnlocked(ESpecialItem ItemID);

	UFUNCTION(BlueprintCallable, Category = "Base|Shop Unlocks")
	void UnlockTool(EToolType ToolID);

	UFUNCTION(BlueprintCallable, Category = "Base|Shop Unlocks")
	void UnlockSpecialItem(ESpecialItem ItemID);

protected:
	// -------------------------------------------------------------------------
	// EVENT HANDLERS
	// -------------------------------------------------------------------------
	UFUNCTION()
	void OnOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	UFUNCTION()
	void OnOverlapEnd(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);
};