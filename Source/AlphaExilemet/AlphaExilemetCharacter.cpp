#include "AlphaExilemetCharacter.h"
#include "ToolBase.h"
#include "Camera/CameraComponent.h"
#include "DrawDebugHelpers.h"
#include "Interactable.h"
#include "TimerManager.h"
#include "GameFramework/SpringArmComponent.h"
#include "Components/SphereComponent.h"
#include "ResourceBase.h"
#include "PhysicalMaterials/PhysicalMaterial.h"
#include "BaseCamp.h"
#include "Kismet/GameplayStatics.h"
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
	HealthProgression.BaseValue = 100.0f;
	HealthProgression.AdditivePerLevel = 25.0f;
	HealthProgression.MultiplierPerLevel = 1.0f; 
	
	OxygenDrainProgression.BaseValue = 2.0f;
	OxygenDrainProgression.AdditivePerLevel = 0.0f;
	OxygenDrainProgression.MultiplierPerLevel = 0.85f; 
	
	AgilityProgression.BaseValue = 600.0f;
	AgilityProgression.AdditivePerLevel = 0.0f;
	AgilityProgression.MultiplierPerLevel = 1.1f;
	
	JumpProgression.BaseValue = 420.0f;
	JumpProgression.AdditivePerLevel = 50.0f;
	JumpProgression.MultiplierPerLevel = 1.0f;
	
	SprintMultiplierProgression.BaseValue = 1.5f;
	SprintMultiplierProgression.AdditivePerLevel = 0.1f;
	SprintMultiplierProgression.MultiplierPerLevel = 1.0f;
	
	GravityScaleProgression.BaseValue = 1.5f; 
	GravityScaleProgression.AdditivePerLevel = -0.1f;
	GravityScaleProgression.MultiplierPerLevel = 1.0f;

	AirControlProgression.BaseValue = 0.05f;
	AirControlProgression.AdditivePerLevel = 0.05f;
	AirControlProgression.MultiplierPerLevel = 1.0f;

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
	
	bIsSurvivalActive = false;
	
	CurrentTool = nullptr;
	InteractionDistance = 300.0f;

	bIsInSafeZone = false;
	OxygenDrainRate = OxygenDrainProgression.BaseValue;
	CurrentSprintMultiplier = SprintMultiplierProgression.BaseValue;
	
	// --- SCANNER SETUP ---
	ScannerSphere = CreateDefaultSubobject<USphereComponent>(TEXT("ScannerSphere"));
	ScannerSphere->SetupAttachment(RootComponent);
	ScannerSphere->InitSphereRadius(0.0f);
	ScannerSphere->SetCollisionProfileName(TEXT("Trigger"));

	// Bind the overlap events
	ScannerSphere->OnComponentBeginOverlap.AddDynamic(this, &AAlphaExilemetCharacter::OnScannerOverlapBegin);
	ScannerSphere->OnComponentEndOverlap.AddDynamic(this, &AAlphaExilemetCharacter::OnScannerOverlapEnd);
	
	// --- DEATH CAMERA SETUP ---
	DeathCameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("DeathCameraBoom"));
	DeathCameraBoom->SetupAttachment(RootComponent); 
	DeathCameraBoom->TargetArmLength = 500.0f;
	DeathCameraBoom->SetRelativeRotation(FRotator(-45.0f, 0.0f, 0.0f));
	DeathCameraBoom->bDoCollisionTest = true;
	DeathCameraBoom->bUsePawnControlRotation = false;

	DeathCameraComponent = CreateDefaultSubobject<UCameraComponent>(TEXT("DeathCamera"));
	DeathCameraComponent->SetupAttachment(DeathCameraBoom, USpringArmComponent::SocketName);
	DeathCameraComponent->bUsePawnControlRotation = false;
	DeathCameraComponent->SetActive(false);
}

void AAlphaExilemetCharacter::BeginPlay()
{
	Super::BeginPlay();
	
	// Apply stats immediately upon spawning
	RecalculateStats();

	// Store default friction so we can return to normal
	DefaultGroundFriction = GetCharacterMovement()->GroundFriction;
	DefaultBrakingDeceleration = GetCharacterMovement()->BrakingDecelerationWalking;

	// Cache Base Camp
	BaseCampRef = Cast<ABaseCamp>(UGameplayStatics::GetActorOfClass(GetWorld(), ABaseCamp::StaticClass()));
}

void AAlphaExilemetCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// -------------------------------------------------------------------------
	// COMPLETELY SMOOTH SURVIVAL LOGIC
	// -------------------------------------------------------------------------
	if (bIsSurvivalActive)
	{
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

				if (Health <= 0.0f && !bIsDead)
				{
					Die();
				}
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
	
	// -------------------------------------------------------------------------
	// CENTRALIZED MOVEMENT & SPEED LOGIC
	// -------------------------------------------------------------------------
	if (BaseCampRef)
	{
		// 1. Are we in a hazard OR already inside the Safe Zone? 
		// If so, strip the Base Boost entirely!
		if (HazardSpeedMultiplier < 1.0f || bIsInSafeZone)
		{
			TimeSpentMovingTowardsBase = 0.0f;
			CurrentBaseBoostMultiplier = 1.0f;
		}
		else
		{
			// 2. Calculate Direction and Velocity (Ignoring Z / Up and Down)
			FVector Velocity = GetVelocity();
			if (Velocity.SizeSquared2D() > 10.0f) // If the player is actually moving
			{
				FVector VelocityDir = Velocity.GetSafeNormal2D();
				FVector ToBaseDir = (BaseCampRef->GetActorLocation() - GetActorLocation()).GetSafeNormal2D();
				
				// 3. Dot Product to find the angle
				float DotProduct = FVector::DotProduct(VelocityDir, ToBaseDir);
				float AngleDegrees = FMath::RadiansToDegrees(FMath::Acos(DotProduct));

				// 4. Check if walking towards base
				if (AngleDegrees <= MaxAngleForBaseAcceleration)
				{
					TimeSpentMovingTowardsBase += DeltaTime;
				}
				else
				{
					TimeSpentMovingTowardsBase = 0.0f;
				}
			}
			else
			{
				// Reset if standing still
				TimeSpentMovingTowardsBase = 0.0f; 
			}

			// 5. Apply the Boost if enough time has passed
			if (TimeSpentMovingTowardsBase >= SecondsBeforeBaseAccelerationOccurs)
			{
				CurrentBaseBoostMultiplier = BaseAccelerationMultiplier;
			}
			else
			{
				CurrentBaseBoostMultiplier = 1.0f;
			}
		}
	}

	// 6. CALCULATE THE FINAL MASTER SPEED
	float SprintMod = bIsSprinting ? CurrentSprintMultiplier : 1.0f;
	
	// Formula: Base Speed * Sprint Boost * Base Return Boost * Hazard Slow
	GetCharacterMovement()->MaxWalkSpeed = BaseWalkSpeed * SprintMod * CurrentBaseBoostMultiplier * HazardSpeedMultiplier;
	
	// -------------------------------------------------------------------------
	// SURFACE HAZARD DETECTION
	// -------------------------------------------------------------------------
	if (GetCharacterMovement()->IsMovingOnGround())
	{
		// Check the floor directly under the player
		UPhysicalMaterial* FloorMat = GetCharacterMovement()->CurrentFloor.HitResult.PhysMaterial.Get();
		
		if (FloorMat == IcePhysicalMaterial)
		{
			float DampenerMod = 1.0f;
			if (BaseCampRef)
			{
				int32 DampenerLevel = BaseCampRef->GetShipSystemLevel(EShipSystem::HazardDampener);
				DampenerMod = BaseCampRef->DampenerProgression.GetValueAtLevel(DampenerLevel);
			}
			
			float IceFriction = 0.5f / DampenerMod;
			float IceBraking = 100.0f / DampenerMod;

			GetCharacterMovement()->GroundFriction = IceFriction;
			GetCharacterMovement()->BrakingDecelerationWalking = IceBraking;
		}
		else
		{
			// Normal Ground - Restore Defaults
			GetCharacterMovement()->GroundFriction = DefaultGroundFriction;
			GetCharacterMovement()->BrakingDecelerationWalking = DefaultBrakingDeceleration;
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
	BaseWalkSpeed = AgilityProgression.GetValueAtLevel(CurrentAgilityLevel);
	GetCharacterMovement()->MaxWalkSpeed = BaseWalkSpeed;
	GetCharacterMovement()->JumpZVelocity = JumpProgression.GetValueAtLevel(CurrentAgilityLevel);
	CurrentSprintMultiplier = SprintMultiplierProgression.GetValueAtLevel(CurrentAgilityLevel);
	GetCharacterMovement()->GravityScale = GravityScaleProgression.GetValueAtLevel(CurrentAgilityLevel);
	GetCharacterMovement()->AirControl = AirControlProgression.GetValueAtLevel(CurrentAgilityLevel);

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

// -------------------------------------------------------------------------
// SCANNER SYSTEM
// -------------------------------------------------------------------------
void AAlphaExilemetCharacter::OnScannerOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (AResourceBase* Resource = Cast<AResourceBase>(OtherActor))
	{
		Resource->SetOutline(true);
	}
}

void AAlphaExilemetCharacter::OnScannerOverlapEnd(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	if (AResourceBase* Resource = Cast<AResourceBase>(OtherActor))
	{
		Resource->SetOutline(false);
	}
}

// -------------------------------------------------------------------------
// DEATH
// -------------------------------------------------------------------------
void AAlphaExilemetCharacter::Die()
{
	bIsDead = true;

	// 1. Disable Input and completely stop momentum
	APlayerController* PC = Cast<APlayerController>(GetController());
	if (PC)
	{
		PC->DisableInput(PC);
	}
	GetCharacterMovement()->StopMovementImmediately(); // Kills velocity
	GetCharacterMovement()->DisableMovement();         // Prevents gravity/falling on the capsule

	// 2. Switch Cameras
	FirstPersonCameraComponent->SetActive(false);
	DeathCameraComponent->SetActive(true);

	// 3. Attach the Death Camera to the Mesh so it follows the ragdoll
	// We use KeepWorldTransform so it doesn't suddenly snap to the floor
	DeathCameraBoom->AttachToComponent(GetMesh(), FAttachmentTransformRules::KeepWorldTransform);

	// 4. Clear all tool inventories safely using our new OOP function
	for (AToolBase* Tool : OwnedTools)
	{
		if (Tool)
		{
			Tool->ClearInventory();
		}
	}

	// 5. Trigger Ragdoll
	GetMesh()->SetCollisionProfileName(TEXT("Ragdoll"));
	GetMesh()->SetSimulatePhysics(true);

	// 6. Tell BP to show the UI
	BP_OnPlayerDied();
}

void AAlphaExilemetCharacter::RespawnPlayer(FVector SpawnLocation, FRotator SpawnRotation)
{
	// 1. Reset Stats
	Health = MaxHealth;
	Oxygen = MaxOxygen;
	bIsDead = false;

	OnHealthChanged.Broadcast(Health, MaxHealth);
	OnOxygenChanged.Broadcast(Oxygen, MaxOxygen);
	
	// 2. Revert Cameras
	DeathCameraComponent->SetActive(false);
	FirstPersonCameraComponent->SetActive(true);

	// 3. Fix Physics and re-attach Mesh
	GetMesh()->SetSimulatePhysics(false);
	GetMesh()->SetCollisionProfileName(TEXT("CharacterMesh"));
	GetMesh()->AttachToComponent(RootComponent, FAttachmentTransformRules::SnapToTargetNotIncludingScale);
	GetMesh()->SetRelativeLocationAndRotation(FVector(0.f, 0.f, -90.f), FRotator(0.f, -90.f, 0.f)); 

	// 4. Re-attach the Death Camera back to the Capsule for next time
	DeathCameraBoom->AttachToComponent(RootComponent, FAttachmentTransformRules::SnapToTargetNotIncludingScale);
	DeathCameraBoom->SetRelativeRotation(FRotator(-45.0f, 0.0f, 0.0f));

	// 5. Teleport to Base
	SetActorLocationAndRotation(SpawnLocation, SpawnRotation);

	// 6. Re-enable Input and Movement
	APlayerController* PC = Cast<APlayerController>(GetController());
	if (PC)
	{
		PC->EnableInput(PC);
	}
	GetCharacterMovement()->SetMovementMode(MOVE_Walking); // Turns movement back on
}