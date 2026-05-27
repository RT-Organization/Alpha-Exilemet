// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "DroppedSolidResource.h"
#include "AlphaExilemet/Tools/PickaxeTool.h"
#include "ResourceBase.h"
#include "Sound/SoundBase.h"
#include "SolidResource.generated.h"

/**
 * Solid resource actor (ore, stone, crystal, etc.).
 */
UCLASS()
class ALPHAEXILEMET_API ASolidResource : public AResourceBase
{
	GENERATED_BODY()
	
public:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Solid")
	UStaticMeshComponent* OreMesh;
	
	/** Sounds played every time the resource is successfully hit. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Audio")
	TArray<USoundBase*> MiningHitSounds;
	
	/** Random pitch variation for hit sound. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Audio")
	FVector2D MiningHitPitchRange = FVector2D(0.95f, 1.05f);
	
	/** Random volume variation for hit sound. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Audio")
	FVector2D MiningHitVolumeRange = FVector2D(0.95f, 1.05f);
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Drop")
	float DropImpulseStrength = 300.f;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Drop",
		meta = (ClampMin = "0.0", ClampMax = "360.0"))
	float DropImpulseAngle = 45.f;
	
	// Sets default values for this actor
	ASolidResource();
	
	UFUNCTION(BlueprintCallable, Category = "Solid|Mining")
	void SetLastPickaxe(APickaxeTool* Tool);
	
	/**
	 * Override base damage function so we can play mining sounds
	 * whenever the node is struck.
	 */
	virtual bool ApplyResourceDamage(float DamageAmount) override;
	
protected:
	UPROPERTY()
	APickaxeTool* LastPickaxe;
	
	virtual void UpdateScale() override;
	virtual void DepleteResource() override;
	
	void SpawnDroppedResource();
	void PlayMiningHitSound() const;
};