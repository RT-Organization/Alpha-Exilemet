// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ToolBase.generated.h"

UCLASS()
class ALPHAEXILEMET_API AToolBase : public AActor
{
	GENERATED_BODY()
	
public:
	AToolBase();

protected:
	virtual void BeginPlay() override;

public:
	/* ----------------------------- */
	/*            TOOL INFO          */
	/* ----------------------------- */
	
	// Icon shown in GUI
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Tool|UI")
	UTexture2D* Icon;
	
	// Tool display name
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Tool|UI")
	FText DisplayName;
	
	
	
	/* ----------------------------- */
	/*         TOOL EVENTS           */
	/* ----------------------------- */

	UFUNCTION(BlueprintImplementableEvent, Category="Tool")
	void OnEquip();

	UFUNCTION(BlueprintImplementableEvent, Category="Tool")
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
};
