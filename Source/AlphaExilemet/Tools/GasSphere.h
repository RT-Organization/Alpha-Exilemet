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

	UPROPERTY(VisibleAnywhere)
	UStaticMeshComponent* Mesh;

	UPROPERTY()
	AGasRodTool* OwnerTool;

	UPROPERTY()
	AGasResource* TargetGas;

	UPROPERTY()
	FName GasType;

	float DamagePerSecond = 10.f;

	bool bIsAttached = false;
	bool bIsFull = false;

public:

	void TryAttachToGas(AGasResource* Gas);

	virtual void Interact_Implementation(AAlphaExilemetCharacter* Interactor) override;
};