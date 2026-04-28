#pragma once

#include "CoreMinimal.h"
#include "ToolBase.h"
#include "AlphaExilemet/Data/AlphaExilemetTypes.h"
#include "GasRodTool.generated.h"

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
	/* ----------------------------- */
	/* PROGRESSION MATH              */
	/* ----------------------------- */
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="GasRod|Progression")
	FStatProgression AbsSpeedProgression;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="GasRod|Progression")
	FStatProgression RangeProgression;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="GasRod|Progression")
	FStatProgression CapacityProgression;
	
	/* ----------------------------- */
	/* INVENTORY                     */
	/* ----------------------------- */
	
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category="GasRod|Inventory")
	TMap<FName, int32> HarvestedGas;
	
	virtual void ClearInventory(float RetainedFraction = 0.0f) override;
	
	UFUNCTION(BlueprintCallable, Category="GasRod|Inventory")
	bool TryAddGas(const FDataTableRowHandle& ResourceID, int32 Quantity = 1);
	
	UFUNCTION(BlueprintPure, Category="GasRod|Inventory")
	int32 GetCurrentTotalSpheres() const;

	UFUNCTION(BlueprintPure, Category="GasRod|Inventory")
	float GetFillPercent() const;

	virtual int32 RemoveResource(FName InResourceID, int32 Amount) override;
	virtual int32 GetResourceAmount(FName InResourceID) const override;

	// Returns the full gas inventory. Used by the upgrade cost check and the inspect widget.
	virtual TMap<FName, int32> GetAllResources() const override;
	
	/* ----------------------------- */
	/* STAT GETTERS                  */
	/* ----------------------------- */
	
	UFUNCTION(BlueprintPure, Category="GasRod|Stats")
	float GetAbsorptionSpeed() const;

	UFUNCTION(BlueprintPure, Category="GasRod|Stats")
	float GetRodRange() const;

	virtual float GetMaxCapacity() const override;
	
	/* ----------------------------- */
	/* SAVE & LOAD                   */
	/* ----------------------------- */
	virtual void SaveToolData(class UAlphaExilemetSaveGame* SaveObject) override;
	virtual void LoadToolData(class UAlphaExilemetSaveGame* SaveObject) override;
};