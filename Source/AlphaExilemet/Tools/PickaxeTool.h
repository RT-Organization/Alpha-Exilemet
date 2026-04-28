#pragma once

#include "CoreMinimal.h"
#include "ToolBase.h"
#include "AlphaExilemet/Data/AlphaExilemetTypes.h"
#include "PickaxeTool.generated.h"

class ACharacter;

UCLASS()
class ALPHAEXILEMET_API APickaxeTool : public AToolBase
{
	GENERATED_BODY()
	
public:
	APickaxeTool();
	
	virtual void StartUsing_Implementation() override;
	virtual void StopUsing_Implementation() override;
	
protected:
	virtual void BeginPlay() override;
	
public:
	/* ----------------------------- */
	/* PROGRESSION MATH              */
	/* ----------------------------- */

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Pickaxe|Progression")
	FStatProgression StrengthProgression;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Pickaxe|Progression")
	FStatProgression CapacityProgression;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Pickaxe|Progression")
	FStatProgression LuckProgression;
	
	/* ----------------------------- */
	/* INVENTORY                     */
	/* ----------------------------- */
	
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category="Pickaxe|Inventory")
	TMap<FName, int32> HarvestedOres;
	
	virtual void ClearInventory(float RetainedFraction = 0.0f) override;

	// Returns the full ore inventory. Used by the upgrade cost check and the inspect widget.
	virtual TMap<FName, int32> GetAllResources() const override;
	
	/* ----------------------------- */
	/* MINING                        */
	/* ----------------------------- */
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Pickaxe|Mining")
	float MiningRange = 500.f;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Pickaxe|Mining")
	float MiningInterval = 0.2f;
	
	FTimerHandle MiningTimer;

	/* ----------------------------- */
	/* GAMEPLAY GETTERS              */
	/* ----------------------------- */
	
	UFUNCTION(BlueprintPure, Category="Pickaxe|Stats")
	float GetMiningStrength() const;

	UFUNCTION(BlueprintPure, Category="Pickaxe|Stats")
	float GetMiningLuck() const;

	virtual float GetMaxCapacity() const override;
	
	UFUNCTION(BlueprintCallable, Category="Pickaxe|Inventory")
	bool TryAddOre(const FDataTableRowHandle& ResourceID, int32 Quantity = 1);
	
	UFUNCTION(BlueprintImplementableEvent, Category="Tool|Harvest")
	void InventoryChanged();
	
	virtual int32 RemoveResource(FName InResourceID, int32 Amount) override;
	virtual int32 GetResourceAmount(FName InResourceID) const override;
	
	/* ----------------------------- */
	/* SAVE & LOAD                   */
	/* ----------------------------- */
	virtual void SaveToolData(class UAlphaExilemetSaveGame* SaveObject) override;
	virtual void LoadToolData(class UAlphaExilemetSaveGame* SaveObject) override;
	
protected:
	/* ----------------------------- */
	/* INTERNAL LOGIC                */
	/* ----------------------------- */
	
	UPROPERTY()
	ACharacter* OwnerCharacter;

	void StartMiningTimer();
	void StopMiningTimer();
	
	UFUNCTION(BlueprintCallable, Category="Pickaxe|Harvesting")
	void PerformMiningTrace();
	UFUNCTION(BlueprintCallable, Category="Pickaxe|Harvesting")
	void ApplyMiningDamage(AActor* Target);
};
