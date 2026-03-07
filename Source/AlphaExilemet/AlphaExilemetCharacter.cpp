#include "AlphaExilemetCharacter.h"
#include "ToolBase.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Interactable.h"

// Sets default values
AAlphaExilemetCharacter::AAlphaExilemetCharacter()
{
	PrimaryActorTick.bCanEverTick = true;

	// Create a CameraComponent	
	FirstPersonCameraComponent = CreateDefaultSubobject<UCameraComponent>(TEXT("FirstPersonCamera"));
	FirstPersonCameraComponent->SetupAttachment(CastChecked<USceneComponent, UCapsuleComponent>(GetCapsuleComponent()));
	FirstPersonCameraComponent->SetRelativeLocation(FVector(0, 0, 0)); // Position the camera
	FirstPersonCameraComponent->bUsePawnControlRotation = true;

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
	
	CurrentTool = nullptr;
	
	// Initialize Extra Variables
	InteractionDistance = 300.0f;
}

void AAlphaExilemetCharacter::BeginPlay()
{
	Super::BeginPlay();
}

void AAlphaExilemetCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

void AAlphaExilemetCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
}

// -------------------------------------------------------------------------
// EQUIP
// -------------------------------------------------------------------------
void AAlphaExilemetCharacter::Equip(AToolBase* NewTool)
{
	if (CurrentTool)
	{
		Unequip();
	}

	if (NewTool)
	{
		CurrentTool = NewTool;
	}
}

void AAlphaExilemetCharacter::Unequip()
{
	if (CurrentTool)
	{
		CurrentTool = nullptr;
	}
}

// -------------------------------------------------------------------------
// INTERACT
// -------------------------------------------------------------------------
void AAlphaExilemetCharacter::TryInteract()
{
	FVector StartLoc = FirstPersonCameraComponent->GetComponentLocation();
	FVector ForwardVector = FirstPersonCameraComponent->GetForwardVector();
	FVector EndLoc = StartLoc + (ForwardVector * InteractionDistance);

	FHitResult HitResult;
	FCollisionQueryParams CollisionParams;
	CollisionParams.AddIgnoredActor(this); // Don't hit the player

	// Shoot the Raycast
	bool bHit = GetWorld()->LineTraceSingleByChannel(HitResult, StartLoc, EndLoc, ECC_Visibility, CollisionParams);

	if (bHit && HitResult.GetActor())
	{
		AActor* HitActor = HitResult.GetActor();
		
		if (HitActor->Implements<UInteractable>())
		{
			IInteractable::Execute_Interact(HitActor, this);
		}
	}
}