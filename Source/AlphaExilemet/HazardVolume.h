#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "HazardVolume.generated.h"

class UBoxComponent;
class USphereComponent;
class UStaticMeshComponent;
class USceneComponent;
class AAlphaExilemetCharacter;
class ABaseCamp;

// The Dropdown Menu for the Editor
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

	// --- COMPONENTS ---
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Hazard|Components")
	USceneComponent* DefaultRoot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Hazard|Components")
	UBoxComponent* HazardZone;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Hazard|Components")
	USphereComponent* HazardSphere;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Hazard|Components")
	UStaticMeshComponent* HazardMesh;

	// --- HAZARD SETTINGS ---
	// Choose the shape of the hazard area from the dropdown
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hazard|Settings")
	EHazardShape HazardShape;

	// How much damage this deals per second
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hazard|Settings")
	float DamagePerSecond;

	// 1.0 is normal speed. 0.5 is half speed.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hazard|Settings")
	float SpeedMultiplier;

protected:
	// --- RUNTIME VARIABLES ---
	UPROPERTY()
	AAlphaExilemetCharacter* OverlappingPlayer;

	UPROPERTY()
	ABaseCamp* BaseCampRef;

	// --- OVERLAP EVENTS ---
	UFUNCTION()
	void OnOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	UFUNCTION()
	void OnOverlapEnd(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);
};