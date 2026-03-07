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
};