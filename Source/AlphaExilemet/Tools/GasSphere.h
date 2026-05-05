#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "AlphaExilemet/Interfaces/Interactable.h"
#include "GasSphere.generated.h"

class AGasRodTool;
class AGasResource;
class AAlphaExilemetCharacter;

UCLASS()
class ALPHAEXILEMET_API AGasSphere : public AActor, public IInteractable
{
	GENERATED_BODY()
	
public:	
	AGasSphere();

	virtual void Tick(float DeltaTime) override;
	virtual void BeginPlay() override;
	virtual void NotifyActorBeginOverlap(AActor* OtherActor) override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	void InitSphere(AGasRodTool* InOwnerTool, float InDamagePerSecond);

protected:
	
	// =========================================================================
	// COMPONENTS
	// =========================================================================
	
	UPROPERTY(VisibleAnywhere)
	UStaticMeshComponent* Mesh;
	
	// =========================================================================
	// REFERENCES
	// =========================================================================
	
	UPROPERTY()
	AGasRodTool* OwnerTool;
	
	UPROPERTY()
	AGasResource* TargetGas;
	
	UPROPERTY()
	FName GasType;
	
	// =========================================================================
	// GAMEPLAY
	// =========================================================================
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="GasSphere|Gameplay")
	float DamagePerSecond = 10.f;
	
	bool bIsAttached = false;
	bool bIsFull = false;
	
	// =========================================================================
	// ORBIT / ANIMATION
	// =========================================================================
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="GasSphere|Orbit")
	float OrbitSpeed = 4.0f;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="GasSphere|Orbit")
	float OrbitSpeedMultiplier = 3.0f;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="GasSphere|Orbit")
	float SpiralEasePower = 2.0f;
	
	// Runtime
	FVector InitialOffset;
	FVector Velocity;
	float InitialRadius = 0.f;
	float CurrentAngle = 0.f;
	float OrbitDirection = 1.f; // +1 or -1
	
	float TimeToKill = 1.f;
	float ElapsedTime = 0.f;
	
public:
	
	void TryAttachToGas(AGasResource* Gas);
	
	virtual void Interact_Implementation(AAlphaExilemetCharacter* Interactor) override;
};