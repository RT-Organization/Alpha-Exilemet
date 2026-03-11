// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ResourceBase.generated.h"

UCLASS()
class ALPHAEXILEMET_API AResourceBase : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AResourceBase();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;
	
	
	
	/* ----------------------------- */
	/*          COMPONENTS           */
	/* ----------------------------- */
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Resource")
	USceneComponent* Root;
	
	
	
	/* ----------------------------- */
	/*            STATS              */
	/* ----------------------------- */
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Resource")
	float Health = 100.f;
	
	UPROPERTY(BlueprintReadOnly, Category="Resource")
	float InitialHealth;
	
	UPROPERTY(BlueprintReadOnly, Category="Resource|Transform")
	FVector InitialScale;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Resource")
	float CurrencyValuePerUnit = 20.f;
	
	
	
	/* ----------------------------- */
	/*        REGENERATION           */
	/* ----------------------------- */
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Resource")
	float VeinRegenerationTime = 10.f;
	
	bool bIsDepleted = false;
	
	FTimerHandle RegenTimer;
	
	
	
	/* ----------------------------- */
	/*        INTERNAL METHODS       */
	/* ----------------------------- */
	
	void RegenerateResource();
	void DepleteResource();
	
	void UpdateScale();
public:	
	UFUNCTION(BlueprintCallable, Category="Resource")
	void ApplyResourceDamage(float DamageAmount);
	
};
