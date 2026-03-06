#include "AlphaExilemetCharacter.h"
// Make sure to include your tool's header file here eventually!
// #include "ToolBase.h" 

// Sets default values
AAlphaExilemetCharacter::AAlphaExilemetCharacter()
{
	// Set this character to call Tick() every frame.
	PrimaryActorTick.bCanEverTick = true;

	// Initialize Base Stat Levels
	OxygenLevel = 0;
	HealthLevel = 0;
	AgilityLevel = 0;
	CapacityLevel = 0;

	// Initialize Runtime Variables
	MaxHealth = 100.0f;
	Health = MaxHealth;
    
	MaxOxygen = 100.0f;
	Oxygen = MaxOxygen;

	Currency = 0.0f;

	// Initialize Equipment
	//CurrentTool = nullptr;
}

// Called when the game starts or when spawned
void AAlphaExilemetCharacter::BeginPlay()
{
	Super::BeginPlay();
	
	// Here you can add logic to scale MaxHealth and MaxOxygen 
	// based on the HealthLevel and OxygenLevel at the start of the game!
}

// Called every frame
void AAlphaExilemetCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// Oxygen drain logic can potentially go here, or in a Timer for better performance.
}

// Called to bind functionality to input
void AAlphaExilemetCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
}

/* EQUIP METHODS
void AAlphaExilemetCharacter::Equip(AToolBase* NewTool)
{
	// If we already have a tool, unequip it first
	if (CurrentTool)
	{
		Unequip();
	}

	if (NewTool)
	{
		CurrentTool = NewTool;
		// Add attachment logic here (e.g., attach tool mesh to character hand socket)
		// CurrentTool->AttachToComponent(GetMesh(), FAttachmentTransformRules::SnapToTargetNotIncludingScale, FName("RightHandSocket"));
	}
}

void AAlphaExilemetCharacter::Unequip()
{
	if (CurrentTool)
	{
		// Add unequip logic here (e.g., detach, hide, or destroy the tool)
        
		CurrentTool = nullptr;
	}
}
*/