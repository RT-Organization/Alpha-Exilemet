#include "BaseTerminal.h"
#include "Components/BoxComponent.h"
#include "AlphaExilemetCharacter.h"

ABaseTerminal::ABaseTerminal()
{
	PrimaryActorTick.bCanEverTick = false;
	
	InteractionBox = CreateDefaultSubobject<UBoxComponent>(TEXT("InteractionBox"));
	
	// We use BlockAllDynamic so the Raycast (LineTrace) can actually hit the box!
	InteractionBox->SetCollisionProfileName(TEXT("BlockAllDynamic")); 
	
	RootComponent = InteractionBox;
}

void ABaseTerminal::BeginPlay()
{
	Super::BeginPlay();
}

// This function is automatically called by the Interface when the Raycast hits it
void ABaseTerminal::Interact_Implementation(AAlphaExilemetCharacter* Interactor)
{
	OnInteract(Interactor);
}