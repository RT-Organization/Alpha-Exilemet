// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "ToolBase.h"
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
	
	
	
	/* ----------------------------- */
	/*            STATS              */
	/* ----------------------------- */
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Pickaxe|Stats")
	int32 STR = 1;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Pickaxe|Stats")
	int32 CAP = 1;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Pickaxe|Stats")
	int32 LU = 1;
	
	
	
	/* ----------------------------- */
	/*           INVENTORY           */
	/* ----------------------------- */
	// slot array (each slot: ore and amount)
	
	
	
	/* ----------------------------- */
	/*           MINING              */
	/* ----------------------------- */

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Pickaxe|Mining")
	float MiningRange = 500.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Pickaxe|Mining")
	float MiningInterval = 0.2f;

	FTimerHandle MiningTimer;
	
	
	
	/* ----------------------------- */
	/*        INTERNAL LOGIC         */
	/* ----------------------------- */

	void StartMining();
	
	void StopMining();
	
	void PerformMiningTrace();
	
	void ApplyMiningDamage(AActor* Target);
};
// TODO: Mineral Inventory, Add minerals to inventory on mine complete