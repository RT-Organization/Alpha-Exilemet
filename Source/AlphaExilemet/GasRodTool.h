#pragma once

#include "CoreMinimal.h"
#include "ToolBase.h"
#include "AlphaExilemetTypes.h"
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
	
	/* ----------------------------- */
	/* GAMEPLAY GETTERS              */
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