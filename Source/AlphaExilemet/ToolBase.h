// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Interactable.h"
#include "ToolBase.generated.h"

// --- NEW STRUCT TO FIX UHT ERROR ---
USTRUCT(BlueprintType)
struct FMaterialCache
{
	GENERATED_BODY()

	UPROPERTY()
	TArray<UMaterialInterface*> Materials;
};

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
	/* TOOL INFO          */
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
	
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Tool|Animations")
	UAnimMontage* EquipAnimation;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Tool|Animations")
	UAnimMontage* IdleAnimation;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Tool|Animations")
	UAnimMontage* UseAnimation;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Tool|Animations")
	UAnimMontage* HolsterAnimation;
	
	/* ----------------------------- */
	/* TOOL EVENTS           */
	/* ----------------------------- */

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category="Tool")
	void OnEquip();

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category="Tool")
	void OnUnequip();
	
	// Called when the item is spawned from the shop
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category="Tool|Spawning")
	void MaterializeItem();
	virtual void MaterializeItem_Implementation();
	
	// The material to apply while spawning (e.g., M_Disolve)
	UPROPERTY(EditDefaultsOnly, Category="Tool|Spawning")
	UMaterialInterface* MaterializeMaterial;

	// Internal cache to remember the original textures of every mesh piece
	UPROPERTY()
	TMap<UMeshComponent*, FMaterialCache> CachedMaterials;

	UFUNCTION(BlueprintCallable, Category="Tool|Spawning")
	void StartMaterialize();

	UFUNCTION(BlueprintCallable, Category="Tool|Spawning")
	void UpdateMaterialize(float Alpha);

	UFUNCTION(BlueprintCallable, Category="Tool|Spawning")
	void FinishMaterialize();
	
	/* ----------------------------- */
	/* TOOL INPUT            */
	/* ----------------------------- */
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Tool|Equipment")
	FName HolsterSocketName = "spine_03";
	
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
	virtual void ClearInventory(float RetainedFraction = 0.0f);
	
};