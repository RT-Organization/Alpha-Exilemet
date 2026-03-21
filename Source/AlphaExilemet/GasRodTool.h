#pragma once

#include "CoreMinimal.h"
#include "ToolBase.h"
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

	/* ----------------------------- */
	/* STATS              */
	/* ----------------------------- */

	// Absorption Speed Level (0-5)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="GasRod|Stats")
	int32 ABS = 0;

	// Range Level (0-5)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="GasRod|Stats")
	int32 RNG = 0;

	// Quantity / Max Spheres Level (0-5)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="GasRod|Stats")
	int32 MAX = 0;

	virtual void UpgradeStat(FName StatName) override;
	
	/* ----------------------------- */
	/* INVENTORY					 */
	/* ----------------------------- */
	
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category="GasRod|Inventory")
	TArray<FName> HarvestedGas;
	
	/* ----------------------------- */
	/* SETTINGS            */
	/* ----------------------------- */

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="GasRod|Settings")
	float BaseRodRange = 800.f;
};