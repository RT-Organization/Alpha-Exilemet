#pragma once

#include "CoreMinimal.h"
#include "ToolBase.h"
#include "AlphaExilemet/Data/AlphaExilemetTypes.h"
#include "GasRodTool.generated.h"

class AGasSphere;

/**
 * AGasRodTool
 *
 * MENTAL MODEL:
 *   The rod holds physical GAS SPHERES. Each sphere is binary: empty or full.
 *   - Empty spheres can be FIRED at a gas cloud → the sphere harvests the gas
 *     over time, then drops back to the ground.
 *   - The player picks up the now-FULL sphere and returns it to the rod inventory.
 *   - MaxCapacity = total sphere slots in the rod (empty + full combined).
 *
 * INVENTORY LAYOUT:
 *   EmptySphereCount   → how many un-launched empty spheres the rod currently holds
 *   HarvestedGas       → TMap<FName, int32>  = full spheres (gas type → sphere count)
 *
 * RULES:
 *   - Only one sphere can harvest a gas at a time
 *   - A sphere is full ONLY if it kills the gas
 *   - Full spheres drop physically and must be picked up
 *
 * PROGRESSION:
 *   AbsSpeed → increases DPS (damage per second)
 *   Range    → increases throw force
 *   Capacity → increases total sphere slots
 */
UCLASS()
class ALPHAEXILEMET_API AGasRodTool : public AToolBase
{
	GENERATED_BODY()

public:
	AGasRodTool();

	virtual void StartUsing_Implementation() override;
	virtual void StopUsing_Implementation() override;

protected:
	virtual void BeginPlay() override;

public:
	// =========================================================================
	// Components
	// =========================================================================
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="GasRod|Components")
	USceneComponent* SphereSpawnOffset;
	
	// =========================================================================
	// PROGRESSION
	// =========================================================================
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="GasRod|Progression")
	FStatProgression AbsSpeedProgression;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="GasRod|Progression")
	FStatProgression RangeProgression;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="GasRod|Progression")
	FStatProgression CapacityProgression;

	// =========================================================================
	// INVENTORY
	// =========================================================================

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category="GasRod|Inventory")
	TMap<FName, int32> HarvestedGas;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category="GasRod|Inventory")
	int32 EmptySphereCount = 0;

	// =========================================================================
	// SPHERES
	// =========================================================================

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="GasRod|Sphere")
	TSubclassOf<AGasSphere> SphereClass;

	UPROPERTY()
	TArray<TWeakObjectPtr<AGasSphere>> ActiveSpheres;

	UFUNCTION(BlueprintCallable)
	void RecallAllSpheres();

	// =========================================================================
	// INVENTORY OPERATIONS
	// =========================================================================

	UFUNCTION(BlueprintCallable, Category="GasRod|Inventory")
	int32 AddEmptySpheresToRod(int32 Quantity = 1);

	UFUNCTION(BlueprintCallable, Category="GasRod|Inventory")
	bool TakeEmptySphere();

	UFUNCTION(BlueprintCallable, Category="GasRod|Inventory")
	bool ReturnFullSphere(FName GasType);
	
	virtual void UpgradeStat(FName StatName) override;
	
	// =========================================================================
	// INVENTORY QUERIES
	// =========================================================================

	virtual float GetMaxCapacity() const override;

	UFUNCTION(BlueprintPure, Category="GasRod|Inventory")
	int32 GetTotalSpheresInRod() const;

	UFUNCTION(BlueprintPure, Category="GasRod|Inventory")
	bool CanFireSphere() const;

	UFUNCTION(BlueprintPure, Category="GasRod|Inventory")
	bool IsRodFull() const;

	virtual void ClearInventory(float RetainedFraction = 0.0f) override;
	virtual int32 RemoveResource(FName InResourceID, int32 Amount) override;
	virtual int32 GetResourceAmount(FName InResourceID) const override;
	virtual TMap<FName, int32> GetAllResources() const override;

	// =========================================================================
	// STATS
	// =========================================================================
	
	UFUNCTION(BlueprintPure, Category="GasRod|Stats")
	float GetAbsorptionSpeed() const;

	UFUNCTION(BlueprintPure, Category="GasRod|Stats")
	float GetRodRange() const;

	// =========================================================================
	// SAVE & LOAD
	// =========================================================================
	virtual void SaveToolData(class UAlphaExilemetSaveGame* SaveObject) override;
	virtual void LoadToolData(class UAlphaExilemetSaveGame* SaveObject) override;
};