// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Interactable.h"
#include "ToolBase.generated.h"

UCLASS()
class ALPHAEXILEMET_API AToolBase : public AActor, public IInteractable
{
	GENERATED_BODY()
	
public:
	AToolBase();

protected:
	virtual void BeginPlay() override;

public:
	/* ----------------------------- */
	/* INTERACTION           */
	/* ----------------------------- */
	virtual void Interact_Implementation(class AAlphaExilemetCharacter* Interactor) override;
	
	/* ----------------------------- */
	/*            TOOL INFO          */
	/* ----------------------------- */
	
	// Icon shown in GUI
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Tool|UI")
	UTexture2D* Icon;
	
	// Tool display name
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Tool|UI")
	FText DisplayName;
	
	// Tool description for the Workbench UI
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Tool|UI", meta=(MultiLine="true"))
	FText Description;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Tool")
	UStaticMeshComponent* Mesh;
	
	/* ----------------------------- */
	/*         TOOL EVENTS           */
	/* ----------------------------- */

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category="Tool")
	void OnEquip();

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category="Tool")
	void OnUnequip();
	
	
	
	/* ----------------------------- */
	/*         TOOL INPUT            */
	/* ----------------------------- */
	
	// Press input
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category="Tool")
	void StartUsing();
	virtual void StartUsing_Implementation();
	
	// Release input
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category="Tool")
	void StopUsing();
	virtual void StopUsing_Implementation();
	
	/* ----------------------------- */
	/* TOOL PROGRESSION      */
	/* ----------------------------- */
	
	// The names of the Data Table rows this specific tool uses (e.g., "Pickaxe_Force")
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Tool|Progression")
	TArray<FName> UpgradeStatNames;
	
	// Maps the specific Stat (e.g., "Pickaxe_Force")
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tool|Progression")
	TMap<FName, int32> ToolUpgradeLevels;

	// Helper function to get this tool's stat level
	UFUNCTION(BlueprintPure, Category = "Tool|Progression")
	int32 GetToolStatLevel(FName StatName);
	
	// Returns the true Max Capacity based on the tool's current upgrades
	UFUNCTION(BlueprintPure, Category="Tool|Stats")
	virtual float GetMaxCapacity() const;

	// Called when the Terminal upgrades a stat
	UFUNCTION(BlueprintCallable, Category = "Tool|Progression")
	virtual void UpgradeStat(FName StatName);

	/* ----------------------------- */
	/* TOOL INVENTORY        */
	/* ----------------------------- */

	
};
