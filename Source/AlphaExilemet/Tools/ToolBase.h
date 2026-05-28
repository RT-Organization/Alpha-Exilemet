// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Components/SkeletalMeshComponent.h"
#include "AlphaExilemet/Data/AlphaExilemetTypes.h"
#include "AlphaExilemet/Interfaces/Interactable.h"
#include "ToolBase.generated.h"

class UUserWidget;

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
	/* INTERACTION                   */
	/* ----------------------------- */
	virtual void Interact_Implementation(class AAlphaExilemetCharacter* Interactor) override;
	
	/* ----------------------------- */
	/* TOOL INFO                     */
	/* ----------------------------- */
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Tool")
	EToolType Type;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Tool|UI")
	UTexture2D* Icon;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Tool|UI")
	FText DisplayName;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Tool|UI", meta=(MultiLine="true"))
	FText Description;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Tool")
	UMeshComponent* Mesh;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Tool")
	UStaticMeshComponent* StaticMeshComp;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Tool")
	USkeletalMeshComponent* SkeletalMeshComp;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Tool|Animations")
	UAnimMontage* EquipAnimation;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Tool|Animations")
	UAnimMontage* IdleAnimation;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Tool|Animations")
	UAnimMontage* UseAnimation;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Tool|Animations")
	UAnimMontage* HolsterAnimation;
	
	/* ----------------------------- */
	/* TOOL EVENTS                   */
	/* ----------------------------- */

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category="Tool")
	void OnEquip();

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category="Tool")
	void OnUnequip();
	
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category="Tool|Spawning")
	void MaterializeItem();
	virtual void MaterializeItem_Implementation();
	
	UPROPERTY(EditDefaultsOnly, Category="Tool|Spawning")
	UMaterialInterface* MaterializeMaterial;

	UPROPERTY()
	TMap<UMeshComponent*, FMaterialCache> CachedMaterials;

	UFUNCTION(BlueprintCallable, Category="Tool|Spawning")
	void StartMaterialize();

	UFUNCTION(BlueprintCallable, Category="Tool|Spawning")
	void UpdateMaterialize(float Alpha);

	UFUNCTION(BlueprintCallable, Category="Tool|Spawning")
	void FinishMaterialize();
	
	/* ----------------------------- */
	/* TOOL INPUT                    */
	/* ----------------------------- */
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Tool|Equipment")
	FName HolsterSocketName = "spine_03";
	
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category="Tool")
	void StartUsing();
	virtual void StartUsing_Implementation();
	
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category="Tool")
	void StopUsing();
	virtual void StopUsing_Implementation();
	
	/* ----------------------------- */
	/* TOOL PROGRESSION              */
	/* ----------------------------- */
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Tool|Progression")
	TArray<FName> UpgradeStatNames;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tool|Progression")
	TMap<FName, int32> ToolUpgradeLevels;

	UFUNCTION(BlueprintPure, Category = "Tool|Progression")
	int32 GetToolStatLevel(FName StatName);
	
	UFUNCTION(BlueprintPure, Category="Tool|Stats")
	virtual float GetMaxCapacity() const;

	UFUNCTION(BlueprintCallable, Category = "Tool|Progression")
	virtual void UpgradeStat(FName StatName);

	/* ----------------------------- */
	/* TOOL INVENTORY                */
	/* ----------------------------- */
	virtual void ClearInventory(float RetainedFraction = 0.0f);

	UFUNCTION(BlueprintCallable, Category="Tool|Inventory")
	virtual int32 RemoveResource(FName InResourceID, int32 Amount);

	UFUNCTION(BlueprintPure, Category="Tool|Inventory")
	virtual int32 GetResourceAmount(FName InResourceID) const;

	// Returns the full inventory map — override in each tool subclass.
	// Used by the character to total resources for upgrade cost checks,
	// and by the Inspect Inventory widget to populate its display.
	UFUNCTION(BlueprintCallable, Category="Tool|Inventory")
	virtual TMap<FName, int32> GetAllResources() const;

	/* ----------------------------- */
	/* INSPECTION SYSTEM             */
	/* ----------------------------- */

	// Assign the specific inventory widget for this tool in each tool's Blueprint CDO.
	// The character's StartInspectCurrentTool() will read this and pass it to BP.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Tool|Inspection")
	TSubclassOf<UUserWidget> InventoryWidgetClass;

	/* ----------------------------- */
	/* SAVE & LOAD                   */
	/* ----------------------------- */
	virtual void SaveToolData(class UAlphaExilemetSaveGame* SaveObject);
	virtual void LoadToolData(class UAlphaExilemetSaveGame* SaveObject);
};