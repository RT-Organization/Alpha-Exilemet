#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Engine/DataTable.h"
#include "ResourceBase.generated.h"

UCLASS()
class ALPHAEXILEMET_API AResourceBase : public AActor
{
	GENERATED_BODY()
	
public:	
	AResourceBase();

protected:
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
	FDataTableRowHandle ResourceID;

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
	float VeinRegenerationTime = 120.f;
	
	bool bIsDepleted = false;
	
	FTimerHandle RegenTimer;
	
	/* ----------------------------- */
	/*        INTERNAL METHODS       */
	/* ----------------------------- */
	
	virtual void RegenerateResource();
	virtual void DepleteResource();
	virtual void UpdateScale();

public:
	
	// =========================================================================
	// ACCESSORS (NEW)
	// =========================================================================
	
	UFUNCTION(BlueprintPure, Category="Resource")
	float GetHealth() const { return Health; }
	
	UFUNCTION(BlueprintPure, Category="Resource")
	float GetInitialHealth() const { return InitialHealth; }
	
	UFUNCTION(BlueprintPure, Category="Resource")
	bool IsDepleted() const { return bIsDepleted; }
	
	// =========================================================================
	
	UFUNCTION(BlueprintCallable, Category="Resource")
	bool ApplyResourceDamage(float DamageAmount);
	
	UFUNCTION(BlueprintCallable, Category="Resource")
	void SetOutline(bool bEnable);
};