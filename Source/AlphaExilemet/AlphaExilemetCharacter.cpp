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
	
	// Start all stats at Level 0
	SystemUpgradeLevels.Add(EPlayerStat::Health, 0);
	SystemUpgradeLevels.Add(EPlayerStat::Oxygen, 0);
	SystemUpgradeLevels.Add(EPlayerStat::Agility, 0);
	
	// -------------------------------------------------------------------------
	// DEFAULT PROGRESSION CONFIGURATION
	// -------------------------------------------------------------------------
	// HOW TO MODIFY: 
	// BaseValue: What the stat starts at when the player is Level 0.
	// AdditivePerLevel: A flat amount added every time you upgrade (e.g., +25 HP).
	// MultiplierPerLevel: A percentage multiplier. 1.0 = no change. 1.1 = +10%. 0.85 = -15%.
	// -------------------------------------------------------------------------
	
	// Health: Starts at 100, adds exactly 25 per level (100 -> 125 -> 150)
	HealthProgression.BaseValue = 100.0f;
	HealthProgression.AdditivePerLevel = 25.0f;
	HealthProgression.MultiplierPerLevel = 1.0f; 

	// Oxygen Drain: Starts at 2.0. We want it to go down, so we multiply by 0.85 per level.
	// (Level 0: 2.0 -> Level 1: 1.7 -> Level 2: 1.44)
	OxygenDrainProgression.BaseValue = 2.0f;
	OxygenDrainProgression.AdditivePerLevel = 0.0f;
	OxygenDrainProgression.MultiplierPerLevel = 0.85f; 

	// Agility Base Walk Speed: Starts at 600 speed, multiplies by 1.1 (+10% speed) per level
	AgilityProgression.BaseValue = 600.0f;
	AgilityProgression.AdditivePerLevel = 0.0f;
	AgilityProgression.MultiplierPerLevel = 1.1f;

	// Jump Height: Starts at Unreal Default (420), adds a flat 50 height per level
	JumpProgression.BaseValue = 420.0f;
	JumpProgression.AdditivePerLevel = 50.0f;
	JumpProgression.MultiplierPerLevel = 1.0f;

	// Sprint Multiplier: Starts at 1.5x (50% faster than walking). 
	// Adds +0.1 to the multiplier per level. (Level 0: 1.5x -> Level 5: 2.0x)
	SprintMultiplierProgression.BaseValue = 1.5f;
	SprintMultiplierProgression.AdditivePerLevel = 0.1f;
	SprintMultiplierProgression.MultiplierPerLevel = 1.0f;

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
	CurrentSprintMultiplier = SprintMultiplierProgression.BaseValue;
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
int32 AAlphaExilemetCharacter::GetSystemStatLevel(EPlayerStat StatName)
{
	if (SystemUpgradeLevels.Contains(StatName))
	{
		return SystemUpgradeLevels[StatName];
	}
	return 0;
}

void AAlphaExilemetCharacter::RecalculateStats()
{
	// Fetch the levels dynamically from the Map
	int32 CurrentHealthLevel = GetSystemStatLevel(EPlayerStat::Health);
	int32 CurrentOxygenLevel = GetSystemStatLevel(EPlayerStat::Oxygen);
	int32 CurrentAgilityLevel = GetSystemStatLevel(EPlayerStat::Agility);

	// Apply math progression to Health
	MaxHealth = HealthProgression.GetValueAtLevel(CurrentHealthLevel);
	Health = FMath::Clamp(Health, 0.0f, MaxHealth); 

	// Apply math progression to Oxygen
	OxygenDrainRate = OxygenDrainProgression.GetValueAtLevel(CurrentOxygenLevel);
	
	// Apply math progression to Character Movement (Agility)
	GetCharacterMovement()->MaxWalkSpeed = AgilityProgression.GetValueAtLevel(CurrentAgilityLevel);
	GetCharacterMovement()->JumpZVelocity = JumpProgression.GetValueAtLevel(CurrentAgilityLevel);
	CurrentSprintMultiplier = SprintMultiplierProgression.GetValueAtLevel(CurrentAgilityLevel);

	// Push UI updates
	OnHealthChanged.Broadcast(Health, MaxHealth);
	OnOxygenChanged.Broadcast(Oxygen, MaxOxygen);
}

void AAlphaExilemetCharacter::UpgradeStat(EPlayerStat StatToUpgrade)
{
	// Check if it exists and is under max level (5)
	if (SystemUpgradeLevels.Contains(StatToUpgrade))
	{
		if (SystemUpgradeLevels[StatToUpgrade] < 5)
		{
			SystemUpgradeLevels[StatToUpgrade]++;
		}
	}

	// Update speeds and limits based on new level
	RecalculateStats();

	// If upgraded Health, heal the player to full
	if (StatToUpgrade == EPlayerStat::Health)
	{
		Health = MaxHealth;
		OnHealthChanged.Broadcast(Health, MaxHealth);
	}
}