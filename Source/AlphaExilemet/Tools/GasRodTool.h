#pragma once

#include "CoreMinimal.h"
#include "ToolBase.h"
#include "AlphaExilemet/Data/AlphaExilemetTypes.h"
#include "GasRodTool.generated.h"

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
 * UPGRADE COST SYSTEM:
 *   GetTotalResourceAmount("RedGas") on the character will call GetResourceAmount()
 *   which reads from HarvestedGas — so the cost system works without changes.
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
	// PROGRESSION MATH
	// =========================================================================
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="GasRod|Progression")
	FStatProgression AbsSpeedProgression;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="GasRod|Progression")
	FStatProgression RangeProgression;
	
	/** Controls the total number of sphere slots (empty + full combined). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="GasRod|Progression")
	FStatProgression CapacityProgression;

	// =========================================================================
	// INVENTORY
	// =========================================================================

	/**
	 * Full spheres currently stored in the rod.
	 * Key = gas resource row name (e.g. "RedGas").
	 * Value = number of full spheres of that gas type.
	 * Used by GetTotalResourceAmount for upgrade cost checks.
	 */
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category="GasRod|Inventory")
	TMap<FName, int32> HarvestedGas;

	/**
	 * Number of empty (unfired) spheres currently inside the rod.
	 * These are available to be launched at a gas cloud.
	 */
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category="GasRod|Inventory")
	int32 EmptySphereCount = 0;

	// -------------------------------------------------------------------------
	// SPHERE OPERATIONS (prepare for gas harvesting mechanic)
	// -------------------------------------------------------------------------

	/**
	 * Add empty spheres to the rod (e.g. player buys new spheres at the shop,
	 * or starting inventory). Respects MaxCapacity.
	 * Returns how many were actually added (may be less if rod is near full).
	 */
	UFUNCTION(BlueprintCallable, Category="GasRod|Inventory")
	int32 AddEmptySpheresToRod(int32 Quantity = 1);

	/**
	 * Remove one empty sphere from the rod to fire it.
	 * Returns true if a sphere was available and removed, false if rod has no empty spheres.
	 */
	UFUNCTION(BlueprintCallable, Category="GasRod|Inventory")
	bool TakeEmptySphere();

	/**
	 * Call when the player picks up a sphere that finished harvesting a gas cloud
	 * and inserts it back into the rod.
	 *
	 * @param GasType  Row name of the harvested gas resource (e.g. "RedGas")
	 * @return true if the sphere was accepted (rod not full), false otherwise.
	 */
	UFUNCTION(BlueprintCallable, Category="GasRod|Inventory")
	bool ReturnFullSphere(FName GasType);

	// -------------------------------------------------------------------------
	// INVENTORY QUERIES
	// -------------------------------------------------------------------------

	/** Total sphere slots available in this rod (at current upgrade level). */
	virtual float GetMaxCapacity() const override;

	/** Total spheres currently held (empty + full). */
	UFUNCTION(BlueprintPure, Category="GasRod|Inventory")
	int32 GetTotalSpheresInRod() const;

	/** True if at least one empty sphere is available to fire. */
	UFUNCTION(BlueprintPure, Category="GasRod|Inventory")
	bool CanFireSphere() const;

	/** True if the rod is at maximum sphere capacity. */
	UFUNCTION(BlueprintPure, Category="GasRod|Inventory")
	bool IsRodFull() const;

	virtual void ClearInventory(float RetainedFraction = 0.0f) override;
	virtual int32 RemoveResource(FName InResourceID, int32 Amount) override;
	virtual int32 GetResourceAmount(FName InResourceID) const override;
	virtual TMap<FName, int32> GetAllResources() const override;

	// =========================================================================
	// STAT GETTERS
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
