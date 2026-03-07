// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "Interactable.generated.h"

// This class does not need to be modified.
UINTERFACE(MinimalAPI)
class UInteractable : public UInterface
{
	GENERATED_BODY()
};

class ALPHAEXILEMET_API IInteractable
{
	GENERATED_BODY()

public:
	// BlueprintNativeEvent allows us to trigger this in C++ but code the visual result in Blueprint!
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Interaction")
	void Interact(class AAlphaExilemetCharacter* Interactor);
};
