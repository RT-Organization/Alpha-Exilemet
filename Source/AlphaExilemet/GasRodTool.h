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
	/* STATS						 */
	/* ----------------------------- */

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="GasRod|Stats")
	int32 AbsSpeedLevel = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="GasRod|Stats")
	int32 RangeLevel = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="GasRod|Stats")
	int32 CapacityLevel = 0;

	/* ----------------------------- */
	/* PROGRESSION MATH				 */
	/* ----------------------------- */

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="GasRod|Progression")
	FStatProgression AbsSpeedProgression;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="GasRod|Progression")
	FStatProgression RangeProgression;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="GasRod|Progression")
	FStatProgression CapacityProgression;

	virtual void UpgradeStat(FName StatName) override;
	
	/* ----------------------------- */
	/* INVENTORY					 */
	/* ----------------------------- */
	
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category="GasRod|Inventory")
	TMap<FName, int32> HarvestedGas;
	
	virtual void ClearInventory() override;
	
	/* ----------------------------- */
	/* COWORKER GAMEPLAY GETTERS     */
	/* ----------------------------- */

	UFUNCTION(BlueprintPure, Category="GasRod|Stats")
	float GetAbsorptionSpeed() const;

	UFUNCTION(BlueprintPure, Category="GasRod|Stats")
	float GetRodRange() const;

	virtual float GetMaxCapacity() const override;
};