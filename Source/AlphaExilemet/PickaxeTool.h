// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "ToolBase.h"
#include "AlphaExilemetTypes.h"
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
	/* STATS                         */
	/* ----------------------------- */
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Pickaxe|Stats")
	int32 StrengthLevel = 0;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Pickaxe|Stats")
	int32 CapacityLevel = 0;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Pickaxe|Stats")
	int32 LuckLevel = 0;

	/* ----------------------------- */
	/* PROGRESSION MATH			     */
	/* ----------------------------- */

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Pickaxe|Progression")
	FStatProgression StrengthProgression;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Pickaxe|Progression")
	FStatProgression CapacityProgression;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Pickaxe|Progression")
	FStatProgression LuckProgression;
	
	virtual void UpgradeStat(FName StatName) override;
	
	/* ----------------------------- */
	/* INVENTORY                     */
	/* ----------------------------- */
	
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category="Pickaxe|Inventory")
	TArray<FName> HarvestedOres;
	
	/* ----------------------------- */
	/* MINING                        */
	/* ----------------------------- */
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Pickaxe|Mining")
	float MiningRange = 500.f;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Pickaxe|Mining")
	float MiningInterval = 0.2f;
	
	FTimerHandle MiningTimer;

	/* ----------------------------- */
	/* COWORKER GAMEPLAY GETTERS     */
	/* ----------------------------- */
	
	UFUNCTION(BlueprintPure, Category="Pickaxe|Stats")
	float GetMiningStrength() const;

	UFUNCTION(BlueprintPure, Category="Pickaxe|Stats")
	float GetMiningLuck() const;

	virtual float GetMaxCapacity() const override;
	
protected:
	/* ----------------------------- */
	/* INTERNAL LOGIC				 */
	/* ----------------------------- */
	
	UPROPERTY()
	ACharacter* OwnerCharacter;

	void StartMiningTimer();
	void StopMiningTimer();
	
	void PerformMiningTrace();
	void ApplyMiningDamage(AActor* Target);
};