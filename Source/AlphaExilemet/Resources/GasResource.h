#pragma once

#include "CoreMinimal.h"
#include "ResourceBase.h"
#include "GasResource.generated.h"

class UNiagaraComponent;
class USphereComponent;
class AGasSphere;

UCLASS()
class ALPHAEXILEMET_API AGasResource : public AResourceBase
{
	GENERATED_BODY()
	
public:	
	AGasResource();
	
	virtual void BeginPlay() override;

	UFUNCTION(BlueprintPure, Category="Gas")
	FName GetGasType() const;

	virtual void Tick(float DeltaTime) override;

	// =========================================================================
	// COMPONENTS
	// =========================================================================
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Gas")
	USphereComponent* GasHitbox;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Gas")
	UNiagaraComponent* GasVFX;

	// =========================================================================
	// RESERVATION SYSTEM
	// =========================================================================

protected:

	UPROPERTY()
	AGasSphere* CurrentSphere = nullptr;

public:

	bool TryReserve(AGasSphere* Sphere);
	void ReleaseReservation();

protected:

	// =========================================================================
	// SCALE TWEEN
	// =========================================================================

	FVector TargetScale;
	float ScaleInterpSpeed = 3.0f;

	virtual void UpdateScale() override;
	virtual void RegenerateResource() override;
	virtual void DepleteResource() override;
};