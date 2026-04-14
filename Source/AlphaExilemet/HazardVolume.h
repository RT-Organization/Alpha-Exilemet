#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "HazardVolume.generated.h"

class UBoxComponent;
class USphereComponent;
class UStaticMeshComponent;
class USceneComponent;
class UPrimitiveComponent;
class AAlphaExilemetCharacter;
class ABaseCamp;

UENUM(BlueprintType)
enum class EHazardShape : uint8
{
	Box            UMETA(DisplayName = "Box Volume"),
	Sphere         UMETA(DisplayName = "Sphere Volume"),
	CustomMesh     UMETA(DisplayName = "Custom Mesh (Must have collision)")
};

UCLASS()
class ALPHAEXILEMET_API AHazardVolume : public AActor
{
	GENERATED_BODY()
	
public:	
	AHazardVolume();

protected:
	virtual void BeginPlay() override;
	virtual void OnConstruction(const FTransform& Transform) override;

public:	
	virtual void Tick(float DeltaTime) override;

	// -------------------------------------------------------------------------
	// COMPONENTS
	// -------------------------------------------------------------------------
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Hazard|Components")
	USceneComponent* DefaultRoot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Hazard|Components")
	UBoxComponent* HazardZone;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Hazard|Components")
	USphereComponent* HazardSphere;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Hazard|Components")
	UStaticMeshComponent* HazardMesh;

	// -------------------------------------------------------------------------
	// HAZARD SETTINGS (DAMAGE & SLOW)
	// -------------------------------------------------------------------------
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hazard|Settings")
	EHazardShape HazardShape;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hazard|Settings")
	float DamagePerSecond;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hazard|Settings")
	float SpeedMultiplier;
	
	// -------------------------------------------------------------------------
	// HAZARD SETTINGS (ICE PHYSICS)
	// -------------------------------------------------------------------------
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hazard|Settings|Ice")
	bool bIsSlippery;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hazard|Settings|Ice", meta = (EditCondition = "bIsSlippery"))
	float SlipperyFriction;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hazard|Settings|Ice", meta = (EditCondition = "bIsSlippery"))
	float SlipperyBraking;

protected:
	// -------------------------------------------------------------------------
	// RUNTIME CACHE
	// -------------------------------------------------------------------------
	UPROPERTY()
	AAlphaExilemetCharacter* OverlappingPlayer;

	UPROPERTY()
	ABaseCamp* BaseCampRef;

	// -------------------------------------------------------------------------
	// EVENT HANDLERS
	// -------------------------------------------------------------------------
	UFUNCTION()
	void OnOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	UFUNCTION()
	void OnOverlapEnd(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);

	// -------------------------------------------------------------------------
	// HAZARD SETTINGS (AUDIO)
	// -------------------------------------------------------------------------
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hazard|Settings")
	TEnumAsByte<EPhysicalSurface> HazardSurfaceType;
};