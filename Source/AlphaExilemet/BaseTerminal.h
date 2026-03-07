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
	// Volume that acts as the physical bounds for the Raycast to hit
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Terminal")
	UBoxComponent* InteractionBox;
	
	// This replaces your old Interact function. It is the Interface requirement!
	virtual void Interact_Implementation(class AAlphaExilemetCharacter* Interactor) override;
	
	// Event for Blueprint implementation (opens UI panel)
	UFUNCTION(BlueprintImplementableEvent, Category="Terminal")
	void OnInteract(class AAlphaExilemetCharacter* Interactor);
};