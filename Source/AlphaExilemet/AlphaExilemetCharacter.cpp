#include "AlphaExilemetCharacter.h"
#include "ToolBase.h"
#include "Camera/CameraComponent.h"
#include "DrawDebugHelpers.h"
#include "Interactable.h"
#include "TimerManager.h"
#include "GameFramework/CharacterMovementComponent.h"

AAlphaExilemetCharacter::AAlphaExilemetCharacter()
{
	PrimaryActorTick.bCanEverTick = true;

	FirstPersonCameraComponent = CreateDefaultSubobject<UCameraComponent>(TEXT("FirstPersonCamera"));
	FirstPersonCameraComponent->SetupAttachment(GetMesh(), FName("head"));
	FirstPersonCameraComponent->SetRelativeLocation(FVector(0.f, 0.f, 0.f));
	FirstPersonCameraComponent->SetRelativeRotation(FRotator(0.f, 0.f, 0.f));
	FirstPersonCameraComponent->bUsePawnControlRotation = true;

	// Start at Level 0
	OxygenLevel = 0;
	HealthLevel = 0;
	AgilityLevel = 0;

	// -------------------------------------------------------------------------
	// DEFAULT PROGRESSION CONFIGURATION
	// -------------------------------------------------------------------------
	// Health: Starts at 100, adds exactly 25 per level (100 -> 125 -> 150)
	HealthProgression.BaseValue = 100.0f;
	HealthProgression.AdditivePerLevel = 25.0f;
	HealthProgression.MultiplierPerLevel = 1.0f; // 1.0 = doesn't multiply

	// Oxygen Drain: Starts at 2.0. We want it to go down, so we multiply by 0.85 per level.
	// (Level 0: 2.0 -> Level 1: 1.7 -> Level 2: 1.44)
	OxygenDrainProgression.BaseValue = 2.0f;
	OxygenDrainProgression.AdditivePerLevel = 0.0f;
	OxygenDrainProgression.MultiplierPerLevel = 0.85f; 

	// Agility: Starts at 600 speed, multiplies by 1.1 (+10% speed) per level
	AgilityProgression.BaseValue = 600.0f;
	AgilityProgression.AdditivePerLevel = 0.0f;
	AgilityProgression.MultiplierPerLevel = 1.1f;

	// -------------------------------------------------------------------------
	// DEFAULT STATS
	// -------------------------------------------------------------------------
	MaxOxygen = 100.0f;
	OxygenRegenRate = 10.0f;
	SuffocationDamageRate = 5.0f;
    
	MaxHealth = HealthProgression.BaseValue;
	Health = MaxHealth;
	Oxygen = MaxOxygen;

	Currency = 0.0f;
	CurrentTool = nullptr;
	InteractionDistance = 300.0f;

	bIsInSafeZone = false;
	OxygenDrainRate = OxygenDrainProgression.BaseValue;
}

void AAlphaExilemetCharacter::BeginPlay()
{
	Super::BeginPlay();
	
	// Apply stats immediately upon spawning
	RecalculateStats();
}

void AAlphaExilemetCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// -------------------------------------------------------------------------
	// COMPLETELY SMOOTH SURVIVAL LOGIC
	// -------------------------------------------------------------------------
	if (bIsInSafeZone)
	{
		if (Oxygen < MaxOxygen)
		{
			Oxygen = FMath::Clamp(Oxygen + (OxygenRegenRate * DeltaTime), 0.0f, MaxOxygen);
			OnOxygenChanged.Broadcast(Oxygen, MaxOxygen);
		}
	}
	else
	{
		if (Oxygen > 0.0f)
		{
			Oxygen = FMath::Clamp(Oxygen - (OxygenDrainRate * DeltaTime), 0.0f, MaxOxygen);
			OnOxygenChanged.Broadcast(Oxygen, MaxOxygen);
		}
		else
		{
			Health = FMath::Clamp(Health - (SuffocationDamageRate * DeltaTime), 0.0f, MaxHealth);
			OnHealthChanged.Broadcast(Health, MaxHealth);

			if (Health <= 0.0f)
			{
				// TODO: Implement Death logic
			}
		}
	}
	
	// -------------------------------------------------------------------------
	// INTERACTION PROMPT LOGIC
	// -------------------------------------------------------------------------
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
// EQUIPMENT
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
// INTERACTION
// -------------------------------------------------------------------------
void AAlphaExilemetCharacter::TryInteract()
{
	FVector StartLoc = FirstPersonCameraComponent->GetComponentLocation();
	FVector ForwardVector = FirstPersonCameraComponent->GetForwardVector();
	FVector EndLoc = StartLoc + (ForwardVector * InteractionDistance);

	FHitResult HitResult;
	FCollisionQueryParams CollisionParams;
	CollisionParams.AddIgnoredActor(this); 

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

// -------------------------------------------------------------------------
// UPGRADE SYSTEM
// -------------------------------------------------------------------------
void AAlphaExilemetCharacter::RecalculateStats()
{
	// Get the new math-calculated limits based on our structs
	MaxHealth = HealthProgression.GetValueAtLevel(HealthLevel);
	Health = FMath::Clamp(Health, 0.0f, MaxHealth); 

	OxygenDrainRate = OxygenDrainProgression.GetValueAtLevel(OxygenLevel);

	GetCharacterMovement()->MaxWalkSpeed = AgilityProgression.GetValueAtLevel(AgilityLevel);
 
	// Push UI updates
	OnHealthChanged.Broadcast(Health, MaxHealth);
	OnOxygenChanged.Broadcast(Oxygen, MaxOxygen);
}

void AAlphaExilemetCharacter::UpgradeStat(EPlayerStat StatToUpgrade)
{
	switch (StatToUpgrade)
	{
		case EPlayerStat::Health:
			if (HealthLevel < 5) HealthLevel++;
			break;
		case EPlayerStat::Oxygen:
			if (OxygenLevel < 5) OxygenLevel++;
			break;
		case EPlayerStat::Agility:
			if (AgilityLevel < 5) AgilityLevel++;
			break;
	}

	RecalculateStats();
}