#pragma once

#include "CoreMinimal.h"
#include "ToolBase.h"
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

	/* ----------------------------- */
	/* STATS              */
	/* ----------------------------- */

	// Speed Level (0-5)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Vacuum|Stats")
	int32 SPD = 0;

	// Range Level (0-5)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Vacuum|Stats")
	int32 RNG = 0;

	// Capacity Level (0-5)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Vacuum|Stats")
	int32 CAP = 0;

	virtual void UpgradeStat(FName StatName) override;
	
	/* ----------------------------- */
	/* SETTINGS            */
	/* ----------------------------- */

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Vacuum|Settings")
	float BaseVacuumRange = 600.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Vacuum|Settings")
	float BaseVacuumPower = 15.f;
};