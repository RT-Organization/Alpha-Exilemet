#include "AlphaExilemetCharacter.h"
#include "ToolBase.h"
#include "Camera/CameraComponent.h"
#include "DrawDebugHelpers.h"
#include "Interactable.h"
#include "TimerManager.h"

// Sets default values
AAlphaExilemetCharacter::AAlphaExilemetCharacter()
{
	PrimaryActorTick.bCanEverTick = true;

	// Create a CameraComponent	
	FirstPersonCameraComponent = CreateDefaultSubobject<UCameraComponent>(TEXT("FirstPersonCamera"));
	FirstPersonCameraComponent->SetupAttachment(GetMesh(), FName("head"));
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

	// Initialize Survival Variables
	bIsInSafeZone = false;
	OxygenDrainRate = 2.0f;
	OxygenRegenRate = 10.0f;
	SuffocationDamageRate = 5.0f;
}

void AAlphaExilemetCharacter::BeginPlay()
{
	Super::BeginPlay();

	// Start a repeating timer that ticks every 1.0 seconds to handle survival logic
	GetWorld()->GetTimerManager().SetTimer(SurvivalTimerHandle, this, &AAlphaExilemetCharacter::HandleSurvivalStats, 1.0f, true);

	// Broadcast initial UI stats
	OnHealthChanged.Broadcast(Health, MaxHealth);
	OnOxygenChanged.Broadcast(Oxygen, MaxOxygen);
}

void AAlphaExilemetCharacter::HandleSurvivalStats()
{
	if (bIsInSafeZone)
	{
		// Regenerate Oxygen while in Base Camp
		if (Oxygen < MaxOxygen)
		{
			Oxygen = FMath::Clamp(Oxygen + OxygenRegenRate, 0.0f, MaxOxygen);
			OnOxygenChanged.Broadcast(Oxygen, MaxOxygen);
		}
	}
	else
	{
		// Drain Oxygen while exploring
		if (Oxygen > 0.0f)
		{
			Oxygen = FMath::Clamp(Oxygen - OxygenDrainRate, 0.0f, MaxOxygen);
			OnOxygenChanged.Broadcast(Oxygen, MaxOxygen);
		}
		else
		{
			// Suffocate when Oxygen is empty
			Health = FMath::Clamp(Health - SuffocationDamageRate, 0.0f, MaxHealth);
			OnHealthChanged.Broadcast(Health, MaxHealth);

			if (Health <= 0.0f)
			{
				UE_LOG(LogTemp, Warning, TEXT("Player Died of Suffocation!"));
				// TODO: Implement Death/Respawn logic here
			}
		}
	}
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

		// Broadcast to UI that a new tool was equipped
		OnToolEquipped.Broadcast(NewTool);
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