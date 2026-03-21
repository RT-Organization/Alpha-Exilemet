#pragma once

#include "CoreMinimal.h"
#include "ToolBase.h"
#include "AlphaExilemetTypes.h"
#include "VacuumTool.generated.h"

UCLASS()
class ALPHAEXILEMET_API AVacuumTool : public AToolBase
{
	GENERATED_BODY()

public:
	AVacuumTool();

	virtual void StartUsing_Implementation() override;
	virtual void StopUsing_Implementation() override;

protected:
	virtual void BeginPlay() override;

public:
	/* ----------------------------- */
	/* STATS              */
	/* ----------------------------- */

	// Track current upgrade levels
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Vacuum|Stats")
	int32 SpeedLevel = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Vacuum|Stats")
	int32 RangeLevel = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Vacuum|Stats")
	int32 CapacityLevel = 0;

	/* ----------------------------- */
	/* PROGRESSION MATH		        */
	/* ----------------------------- */

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Vacuum|Progression")
	FStatProgression SpeedProgression;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Vacuum|Progression")
	FStatProgression RangeProgression;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Vacuum|Progression")
	FStatProgression CapacityProgression;

	virtual void UpgradeStat(FName StatName) override;
	
	/* ----------------------------- */
	/* INVENTORY           */
	/* ----------------------------- */
	
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category="Vacuum|Inventory")
	TMap<FName, int32> HarvestedSlime;
	
	/* ----------------------------- */
	/* SETTINGS            */
	/* ----------------------------- */

	// Deleted BaseVacuumRange and BaseVacuumPower because Progression handles it!

	/* ----------------------------- */
	/* COWORKER GAMEPLAY GETTERS     */
	/* ----------------------------- */

	UFUNCTION(BlueprintPure, Category="Vacuum|Stats")
	float GetVacuumSpeed() const;

	UFUNCTION(BlueprintPure, Category="Vacuum|Stats")
	float GetVacuumRange() const;

	virtual float GetMaxCapacity() const override;
};