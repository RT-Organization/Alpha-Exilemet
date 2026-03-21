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
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Pickaxe|Stats")
	int32 STR = 0;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Pickaxe|Stats")
	int32 CAP = 0;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Pickaxe|Stats")
	int32 LU = 0;
	
	virtual void UpgradeStat(FName StatName) override;
	
	
	/* ----------------------------- */
	/* INVENTORY                     */
	/* ----------------------------- */
	
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category="Pickaxe|Inventory")
	TArray<FName> HarvestedOres;
	
	
	/* ----------------------------- */
	/*           MINING              */
	/* ----------------------------- */
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Pickaxe|Mining")
	float BaseMiningDamage = 20.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Pickaxe|Mining")
	float StrengthScaling = 8.f;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Pickaxe|Mining")
	float MiningRange = 500.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Pickaxe|Mining")
	float MiningInterval = 0.2f;
	
	FTimerHandle MiningTimer;
	
	
	
	/* ----------------------------- */
	/*        INTERNAL LOGIC         */
	/* ----------------------------- */
	
	void StartMiningTimer();
	void StopMiningTimer();
	
	UFUNCTION(BlueprintCallable, Category="Pickaxe|Mining")
	void PerformMiningTrace();

	UFUNCTION(BlueprintCallable, Category="Pickaxe|Mining")
	void ApplyMiningDamage(AActor* Target);
};
// TODO: Mineral Inventory, Add minerals to inventory on mine complete