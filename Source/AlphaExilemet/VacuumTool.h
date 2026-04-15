#pragma once

#include "CoreMinimal.h"
#include "ToolBase.h"
#include "AlphaExilemetTypes.h"
#include "LiquidResource.h"
#include "VacuumTool.generated.h"

class ACharacter;

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
	/* PROGRESSION                   */
	/* ----------------------------- */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Vacuum|Progression")
	FStatProgression SpeedProgression;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Vacuum|Progression")
	FStatProgression RangeProgression;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Vacuum|Progression")
	FStatProgression CapacityProgression;

	/* ----------------------------- */
	/* VACUUM CONSTANTS              */
	/* ----------------------------- */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Vacuum|Constants")
	float TickInterval = 0.1f;

	/* ----------------------------- */
	/* INVENTORY                     */
	/* ----------------------------- */

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category="Vacuum|Inventory")
	TMap<FName, int32> HarvestedSlime;
	
	virtual void ClearInventory(float RetainedFraction = 0.0f) override;

	/* ----------------------------- */
	/* VACUUM INTERNALS              */
	/* ----------------------------- */

	FTimerHandle VacuumTimer;
	
	/* ----------------------------- */
	/* EVENTS                        */
	/* ----------------------------- */

	UFUNCTION(BlueprintImplementableEvent, Category="Vacuum|Events")
	void OnLiquidHitting(ALiquidResource* Liquid);
	
	/* ----------------------------- */
	/* STAT GETTERS                  */
	/* ----------------------------- */
	
	UFUNCTION(BlueprintPure, Category="Vacuum|Stats")
	float GetAbsorptionInterval() const;
	
	UFUNCTION(BlueprintPure, Category="Vacuum|Stats")
	float GetAbsorptionDamagePerTick() const;

	UFUNCTION(BlueprintPure, Category="Vacuum|Stats")
	float GetVacuumRange() const;

	virtual float GetMaxCapacity() const override;

	UFUNCTION(BlueprintPure, Category="Vacuum|Inventory")
	float GetCurrentStoredSlime() const;

	UFUNCTION(BlueprintPure, Category="Vacuum|Inventory")
	float GetFillPercent() const;
	
	/* ----------------------------- */
	/* SAVE & LOAD                   */
	/* ----------------------------- */
	virtual void SaveToolData(class UAlphaExilemetSaveGame* SaveObject) override;
	virtual void LoadToolData(class UAlphaExilemetSaveGame* SaveObject) override;

protected:
	/* ----------------------------- */
	/* INTERNAL                      */
	/* ----------------------------- */

	UPROPERTY()
	ACharacter* OwnerCharacter;

	void StartVacuumTimer();
	void StopVacuumTimer();

	UFUNCTION(BlueprintCallable, Category="Vacuum|Harvesting")
	void PerformVacuumTrace();

	void AbsorbSlime(FName SlimeType, float Amount);
};