#include "AlphaExilemetCharacter.h"
#include "ToolBase.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/SphereComponent.h"
#include "Kismet/GameplayStatics.h"
#include "DrawDebugHelpers.h"
#include "Interactable.h"
#include "TimerManager.h"
#include "ResourceBase.h"
#include "BaseCamp.h"

// -------------------------------------------------------------------------
// CONSTRUCTOR
// -------------------------------------------------------------------------
AAlphaExilemetCharacter::AAlphaExilemetCharacter()
{
	PrimaryActorTick.bCanEverTick = true;

	// --- CAMERA SETUP ---
	FirstPersonCameraComponent = CreateDefaultSubobject<UCameraComponent>(TEXT("FirstPersonCamera"));
	FirstPersonCameraComponent->SetupAttachment(GetMesh(), FName("head"));
	FirstPersonCameraComponent->SetRelativeLocation(FVector::ZeroVector);
	FirstPersonCameraComponent->SetRelativeRotation(FRotator::ZeroRotator);
	FirstPersonCameraComponent->bUsePawnControlRotation = true;
	
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

	// --- SCANNER SETUP ---
	ScannerSphere = CreateDefaultSubobject<USphereComponent>(TEXT("ScannerSphere"));
	ScannerSphere->SetupAttachment(RootComponent);
	ScannerSphere->InitSphereRadius(0.0f);
	ScannerSphere->SetCollisionProfileName(TEXT("Trigger"));
	ScannerSphere->OnComponentBeginOverlap.AddDynamic(this, &AAlphaExilemetCharacter::OnScannerOverlapBegin);
	ScannerSphere->OnComponentEndOverlap.AddDynamic(this, &AAlphaExilemetCharacter::OnScannerOverlapEnd);
	
	// --- PROGRESSION DEFAULTS ---
	SystemUpgradeLevels.Add(EPlayerStat::Health, 0);
	SystemUpgradeLevels.Add(EPlayerStat::Oxygen, 0);
	SystemUpgradeLevels.Add(EPlayerStat::Agility, 0);
	
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

	// --- RUNTIME DEFAULTS ---
	MaxOxygen = 100.0f;
	OxygenRegenRate = 10.0f;
	SuffocationDamageRate = 5.0f;
	MaxHealth = HealthProgression.BaseValue;
	Health = MaxHealth;
	Oxygen = MaxOxygen;
	Currency = 0.0f;
	bIsSurvivalActive = false;
	bIsInSafeZone = false;
	CurrentTool = nullptr;
	InteractionDistance = 300.0f;
	OxygenDrainRate = OxygenDrainProgression.BaseValue;
	CurrentSprintMultiplier = SprintMultiplierProgression.BaseValue;
}

// -------------------------------------------------------------------------
// ENGINE OVERRIDES
// -------------------------------------------------------------------------
void AAlphaExilemetCharacter::BeginPlay()
{
	Super::BeginPlay();
	
	RecalculateStats();

	// Cache standard physics to return to when leaving hazard ice
	DefaultGroundFriction = GetCharacterMovement()->GroundFriction;
	DefaultBrakingDeceleration = GetCharacterMovement()->BrakingDecelerationWalking;

	BaseCampRef = Cast<ABaseCamp>(UGameplayStatics::GetActorOfClass(GetWorld(), ABaseCamp::StaticClass()));
}

void AAlphaExilemetCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// --- 1. SURVIVAL LOGIC (OXYGEN & HEALTH) ---
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
	
	// --- 2. INTERACTION PROMPT LOGIC ---
	bIsLookingAtInteractable = false;
	FVector StartLoc = FirstPersonCameraComponent->GetComponentLocation();
	FVector ForwardVector = FirstPersonCameraComponent->GetForwardVector();
	FVector EndLoc = StartLoc + (ForwardVector * InteractionDistance);

	FHitResult HitResult;
	FCollisionQueryParams CollisionParams;
	CollisionParams.AddIgnoredActor(this); 
	
	if (GetWorld()->LineTraceSingleByChannel(HitResult, StartLoc, EndLoc, ECC_Visibility, CollisionParams))
	{
		if (HitResult.GetActor() && HitResult.GetActor()->Implements<UInteractable>())
		{
			bIsLookingAtInteractable = true;
		}
	}
	
	// --- 3. CENTRALIZED MOVEMENT LOGIC ---
	if (BaseCampRef)
	{
		if (HazardSpeedMultiplier < 1.0f || bIsInSafeZone)
		{
			TimeSpentMovingTowardsBase = 0.0f;
			CurrentBaseBoostMultiplier = 1.0f;
		}
		else
		{
			FVector Velocity = GetVelocity();
			if (Velocity.SizeSquared2D() > 10.0f) 
			{
				FVector VelocityDir = Velocity.GetSafeNormal2D();
				FVector ToBaseDir = (BaseCampRef->GetActorLocation() - GetActorLocation()).GetSafeNormal2D();
				
				float DotProduct = FVector::DotProduct(VelocityDir, ToBaseDir);
				float AngleDegrees = FMath::RadiansToDegrees(FMath::Acos(DotProduct));

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
				TimeSpentMovingTowardsBase = 0.0f; 
			}

			CurrentBaseBoostMultiplier = (TimeSpentMovingTowardsBase >= SecondsBeforeBaseAccelerationOccurs) ? BaseAccelerationMultiplier : 1.0f;
		}
	}

	// Apply final combined movement speed
	float SprintMod = bIsSprinting ? CurrentSprintMultiplier : 1.0f;
	GetCharacterMovement()->MaxWalkSpeed = BaseWalkSpeed * SprintMod * CurrentBaseBoostMultiplier * HazardSpeedMultiplier;
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
	if (CurrentTool) Unequip();
	
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

	if (GetWorld()->LineTraceSingleByChannel(HitResult, StartLoc, EndLoc, ECC_Visibility, CollisionParams))
	{
		if (AActor* HitActor = HitResult.GetActor())
		{
			if (HitActor->Implements<UInteractable>())
			{
				IInteractable::Execute_Interact(HitActor, this);
			}
		}
	}
}

// -------------------------------------------------------------------------
// PROGRESSION
// -------------------------------------------------------------------------
int32 AAlphaExilemetCharacter::GetSystemStatLevel(EPlayerStat StatName)
{
	return SystemUpgradeLevels.Contains(StatName) ? SystemUpgradeLevels[StatName] : 0;
}

void AAlphaExilemetCharacter::RecalculateStats()
{
	int32 CurrentHealthLevel = GetSystemStatLevel(EPlayerStat::Health);
	int32 CurrentOxygenLevel = GetSystemStatLevel(EPlayerStat::Oxygen);
	int32 CurrentAgilityLevel = GetSystemStatLevel(EPlayerStat::Agility);

	MaxHealth = HealthProgression.GetValueAtLevel(CurrentHealthLevel);
	Health = FMath::Clamp(Health, 0.0f, MaxHealth); 

	OxygenDrainRate = OxygenDrainProgression.GetValueAtLevel(CurrentOxygenLevel);
	
	BaseWalkSpeed = AgilityProgression.GetValueAtLevel(CurrentAgilityLevel);
	GetCharacterMovement()->MaxWalkSpeed = BaseWalkSpeed;
	GetCharacterMovement()->JumpZVelocity = JumpProgression.GetValueAtLevel(CurrentAgilityLevel);
	CurrentSprintMultiplier = SprintMultiplierProgression.GetValueAtLevel(CurrentAgilityLevel);
	GetCharacterMovement()->GravityScale = GravityScaleProgression.GetValueAtLevel(CurrentAgilityLevel);
	GetCharacterMovement()->AirControl = AirControlProgression.GetValueAtLevel(CurrentAgilityLevel);

	OnHealthChanged.Broadcast(Health, MaxHealth);
	OnOxygenChanged.Broadcast(Oxygen, MaxOxygen);
}

void AAlphaExilemetCharacter::UpgradeStat(EPlayerStat StatToUpgrade)
{
	if (SystemUpgradeLevels.Contains(StatToUpgrade) && SystemUpgradeLevels[StatToUpgrade] < 5)
	{
		SystemUpgradeLevels[StatToUpgrade]++;
	}

	RecalculateStats();

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
// DEATH & RESPAWN
// -------------------------------------------------------------------------
void AAlphaExilemetCharacter::Die()
{
	bIsDead = true;

	if (APlayerController* PC = Cast<APlayerController>(GetController()))
	{
		PC->DisableInput(PC);
	}
	
	GetCharacterMovement()->StopMovementImmediately(); 
	GetCharacterMovement()->DisableMovement();         

	FirstPersonCameraComponent->SetActive(false);
	DeathCameraComponent->SetActive(true);
	DeathCameraBoom->AttachToComponent(GetMesh(), FAttachmentTransformRules::KeepWorldTransform);

	for (AToolBase* Tool : OwnedTools)
	{
		if (Tool) Tool->ClearInventory();
	}

	GetMesh()->SetCollisionProfileName(TEXT("Ragdoll"));
	GetMesh()->SetSimulatePhysics(true);

	BP_OnPlayerDied();
}

void AAlphaExilemetCharacter::RespawnPlayer(FVector SpawnLocation, FRotator SpawnRotation)
{
	Health = MaxHealth;
	Oxygen = MaxOxygen;
	bIsDead = false;

	OnHealthChanged.Broadcast(Health, MaxHealth);
	OnOxygenChanged.Broadcast(Oxygen, MaxOxygen);
	
	DeathCameraComponent->SetActive(false);
	FirstPersonCameraComponent->SetActive(true);

	GetMesh()->SetSimulatePhysics(false);
	GetMesh()->SetCollisionProfileName(TEXT("CharacterMesh"));
	GetMesh()->AttachToComponent(RootComponent, FAttachmentTransformRules::SnapToTargetNotIncludingScale);
	GetMesh()->SetRelativeLocationAndRotation(FVector(0.f, 0.f, -90.f), FRotator(0.f, -90.f, 0.f)); 

	DeathCameraBoom->AttachToComponent(RootComponent, FAttachmentTransformRules::SnapToTargetNotIncludingScale);
	DeathCameraBoom->SetRelativeRotation(FRotator(-45.0f, 0.0f, 0.0f));

	SetActorLocationAndRotation(SpawnLocation, SpawnRotation);

	if (APlayerController* PC = Cast<APlayerController>(GetController()))
	{
		PC->EnableInput(PC);
	}
	
	GetCharacterMovement()->SetMovementMode(MOVE_Walking); 
}