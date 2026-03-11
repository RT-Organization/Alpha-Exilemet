#include "AlphaExilemetCharacter.h"
#include "ToolBase.h"
#include "Camera/CameraComponent.h"
#include "DrawDebugHelpers.h"
#include "Interactable.h"

// Sets default values
AAlphaExilemetCharacter::AAlphaExilemetCharacter()
{
	PrimaryActorTick.bCanEverTick = true; //

	// Create a CameraComponent	
	FirstPersonCameraComponent = CreateDefaultSubobject<UCameraComponent>(TEXT("FirstPersonCamera"));

	// THE FIX:
	// 1. We change CapsuleComponent to GetMesh()
	// 2. We provide the exact FName of the socket in your mesh (e.g., "head")
	FirstPersonCameraComponent->SetupAttachment(GetMesh(), FName("head"));

	// Set relative location to 0 so it snaps to the socket location
	FirstPersonCameraComponent->SetRelativeLocation(FVector(0.f, 0.f, 0.f));
	FirstPersonCameraComponent->SetRelativeRotation(FRotator(0.f, 0.f, 0.f));

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

	// --- INTERACTION PROMPT LOGIC ---
	bIsLookingAtInteractable = false; 

	FVector StartLoc = FirstPersonCameraComponent->GetComponentLocation();
	FVector ForwardVector = FirstPersonCameraComponent->GetForwardVector();
	FVector EndLoc = StartLoc + (ForwardVector * InteractionDistance);

	FHitResult HitResult;
	FCollisionQueryParams CollisionParams;
	CollisionParams.AddIgnoredActor(this); 
	
	bool bHit = GetWorld()->LineTraceSingleByChannel(HitResult, StartLoc, EndLoc, ECC_Visibility, CollisionParams);

	if (bHit && HitResult.GetActor())
	{
		if (HitResult.GetActor()->Implements<UInteractable>())
		{
			bIsLookingAtInteractable = true;
		}
	}
}

void AAlphaExilemetCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
}

// -------------------------------------------------------------------------
// EQUIP
// -------------------------------------------------------------------------
void AAlphaExilemetCharacter::Equip_Implementation(AToolBase* NewTool)
{
	if (CurrentTool)
	{
		Unequip();
	}
	
	if (NewTool)
	{
		CurrentTool = NewTool;
		NewTool->SetOwner(this);
		NewTool->OnEquip();
	}
}

void AAlphaExilemetCharacter::Unequip_Implementation()
{
	if (CurrentTool)
	{
		CurrentTool->OnUnequip();
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
	DrawDebugLine(GetWorld(), StartLoc, EndLoc, FColor::Red, false, 2.0f);
	
	if (bHit && HitResult.GetActor())
	{
		AActor* HitActor = HitResult.GetActor();
		
		if (HitActor->Implements<UInteractable>())
		{
			IInteractable::Execute_Interact(HitActor, this);
		}
	}
}