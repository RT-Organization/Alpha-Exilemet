#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Interactable.h"
#include "BaseTerminal.generated.h"

class UBoxComponent;

UCLASS()
class ALPHAEXILEMET_API ABaseTerminal : public AActor, public IInteractable
{
	GENERATED_BODY()
	
public:
	ABaseTerminal();
	
protected:
	virtual void BeginPlay() override;
	
	// Internal timer handle to know when the blend finishes
	FTimerHandle CameraBlendTimerHandle;

	// Internal function called by the timer
	void OnBlendComplete();

	// Cache the interactor so the timer can pass it to the Blueprint event
	UPROPERTY()
	class AAlphaExilemetCharacter* CurrentInteractor;
	
public:
	// -------------------------------------------------------------------------
	// VARIABLES
	// -------------------------------------------------------------------------
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Terminal")
	UBoxComponent* InteractionBox;
	
	// If true, we use the Mesh for interaction. If false, we use the InteractionBox.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Terminal")
	bool bUseMeshForInteraction;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Terminal")
	UStaticMeshComponent* TerminalMesh;
	
	// -------------------------------------------------------------------------
	// INTERACTION METHODS
	// -------------------------------------------------------------------------
	virtual void Interact_Implementation(class AAlphaExilemetCharacter* Interactor) override;
	
	// -------------------------------------------------------------------------
	// CONTRUCTION METHODS
	// -------------------------------------------------------------------------
	virtual void OnConstruction(const FTransform& Transform) override;
	
	
	// The camera that the player's view will blend to
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Terminal|Camera")
	class UCameraComponent* TerminalCamera;

	// How long the transition takes (in seconds)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Terminal|Camera")
	float CameraBlendTime = 0.5f;

	// Called in BP exactly when the camera finishes moving so you can show the UI
	UFUNCTION(BlueprintImplementableEvent, Category="Terminal|Events")
	void BP_OnTerminalViewReady(class AAlphaExilemetCharacter* Interactor);

	// Call this from Blueprint (e.g., when the player clicks an "Exit" button on the UI)
	UFUNCTION(BlueprintCallable, Category="Terminal|Interaction")
	void StopTerminalInteraction(class AAlphaExilemetCharacter* Interactor);
};