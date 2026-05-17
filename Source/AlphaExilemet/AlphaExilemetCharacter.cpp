#include "AlphaExilemetCharacter.h"

#include "AlphaExilemet/Core/AlphaExilemetSaveGame.h"
#include "AlphaExilemet/Tools/ToolBase.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/SphereComponent.h"
#include "Kismet/GameplayStatics.h"
#include "DrawDebugHelpers.h"
#include "TimerManager.h"
#include "AlphaExilemet/Resources/ResourceBase.h"
#include "Components/AudioComponent.h"
#include "Components/CapsuleComponent.h"
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
	
	// --- AUDIO SETUP ---
	BreathingAudioComponent = CreateDefaultSubobject<UAudioComponent>(TEXT("BreathingAudioComponent"));
	BreathingAudioComponent->SetupAttachment(RootComponent);
	BreathingAudioComponent->bAutoActivate = false;
}

// -------------------------------------------------------------------------
// ENGINE OVERRIDES
// -------------------------------------------------------------------------
void AAlphaExilemetCharacter::BeginPlay()
{
	Super::BeginPlay();
	
	RecalculateStats();

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
	
	// --- VIGNETTE OXYGEN EFFECT ---
	if (Oxygen < (MaxOxygen * OxygenVignetteThreshold))
	{
		FirstPersonCameraComponent->PostProcessSettings.bOverride_VignetteIntensity = true;
		
		float Alpha = 1.0f - (Oxygen / (MaxOxygen * OxygenVignetteThreshold));
		FirstPersonCameraComponent->PostProcessSettings.VignetteIntensity = FMath::Lerp(0.5f, MaxVignetteIntensity, Alpha);
	}
	else
	{
		FirstPersonCameraComponent->PostProcessSettings.bOverride_VignetteIntensity = false;
	}
	
	// --- 2. INTERACTION PROMPT LOGIC ---
	bIsLookingAtInteractable = false;
	FVector StartLoc    = FirstPersonCameraComponent->GetComponentLocation();
	FVector EndLoc      = StartLoc + (FirstPersonCameraComponent->GetForwardVector() * InteractionDistance);

	FHitResult HitResult;
	FCollisionQueryParams CollisionParams;
	CollisionParams.AddIgnoredActor(this);

	if (GetWorld()->LineTraceSingleByChannel(HitResult, StartLoc, EndLoc, ECC_Visibility, CollisionParams))
	{
		AActor* HitActor = HitResult.GetActor();
		if (HitActor && HitActor->Implements<UInteractable>())
		{
			if (IInteractable::Execute_CanBeInteractedWith(HitActor))
			{
				bIsLookingAtInteractable = true;
			}
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
void AAlphaExilemetCharacter::AddToolToInventory(AToolBase* NewTool)
{
	if (!NewTool || OwnedTools.Contains(NewTool)) return;

	if (OwnedTools.Num() < MaxInventorySize)
	{
		OwnedTools.Add(NewTool);
		NewTool->SetOwner(this);

		NewTool->AttachToComponent(GetMesh(), FAttachmentTransformRules::SnapToTargetNotIncludingScale, NewTool->HolsterSocketName);

		OnInventoryUpdated.Broadcast();
	}
}

void AAlphaExilemetCharacter::StartWieldTool(int32 Index)
{
	if (!OwnedTools.IsValidIndex(Index)) return;
	if (Index == ActiveToolIndex) return; 

	HolsterCurrentTool();
	
	PendingToolIndex = Index;
	
	WieldPendingTool();
}

void AAlphaExilemetCharacter::WieldPendingTool()
{
	if (!CurrentTool && OwnedTools.IsValidIndex(PendingToolIndex) && PendingToolIndex != ActiveToolIndex)
	{
		PlayAnimMontage(OwnedTools[PendingToolIndex]->EquipAnimation);
	}
}

void AAlphaExilemetCharacter::HolsterCurrentTool()
{
	if (CurrentTool)
	{
		PlayAnimMontage(CurrentTool->HolsterAnimation);
	}
}

void AAlphaExilemetCharacter::SnapPendingToolToHand()
{
	if (!CurrentTool && OwnedTools.IsValidIndex(PendingToolIndex) && PendingToolIndex != ActiveToolIndex)
	{
		ActiveToolIndex = PendingToolIndex;
		CurrentTool = OwnedTools[PendingToolIndex];
		
		CurrentTool->OnEquip();
		OnToolEquipped.Broadcast(CurrentTool);
		
		CurrentTool->AttachToComponent(GetMesh(), FAttachmentTransformRules::SnapToTargetNotIncludingScale, FName("weapon_r"));
		OnToolWielded.Broadcast(ActiveToolIndex); 
		
		PendingToolIndex = -1;
	}
}

void AAlphaExilemetCharacter::SnapCurrentToolToHolster()
{
	if (CurrentTool)
	{
		CurrentTool->OnUnequip();
		
		CurrentTool->AttachToComponent(GetMesh(), FAttachmentTransformRules::SnapToTargetNotIncludingScale, CurrentTool->HolsterSocketName);
		
		CurrentTool = nullptr;
		ActiveToolIndex = -1;
		
		OnToolWielded.Broadcast(ActiveToolIndex);
	}
}

// -------------------------------------------------------------------------
// ECONOMY & SELLING
// -------------------------------------------------------------------------

int32 AAlphaExilemetCharacter::GetRefinedSellValue(int32 BaseTotalValue)
{
	float RefinerMultiplier = 1.0f;

	if (BaseCampRef)
	{
		int32 RefinerLevel = BaseCampRef->GetShipSystemLevel(EShipSystem::MolecularRefiner);
		RefinerMultiplier = BaseCampRef->RefinerProgression.GetValueAtLevel(RefinerLevel);
	}

	return FMath::FloorToInt(BaseTotalValue * RefinerMultiplier);
}

void AAlphaExilemetCharacter::ProcessSale(int32 BaseTotalValue)
{
	int32 FinalPayout = GetRefinedSellValue(BaseTotalValue);
	Currency += FinalPayout;
}

// -------------------------------------------------------------------------
// RESOURCE HELPERS (for Upgrade Cost System)
// -------------------------------------------------------------------------

int32 AAlphaExilemetCharacter::GetTotalResourceAmount(FName ResourceID) const
{
	int32 Total = 0;
	for (AToolBase* Tool : OwnedTools)
	{
		if (Tool)
		{
			Total += Tool->GetResourceAmount(ResourceID);
		}
	}
	return Total;
}

void AAlphaExilemetCharacter::DeductResourceFromTools(FName ResourceID, int32 Amount)
{
	int32 Remaining = Amount;
	for (AToolBase* Tool : OwnedTools)
	{
		if (Tool && Remaining > 0)
		{
			int32 Removed = Tool->RemoveResource(ResourceID, Remaining);
			Remaining -= Removed;
		}
	}
}

TMap<FName, int32> AAlphaExilemetCharacter::GetAllResourcesFromTools() const
{
	TMap<FName, int32> AllResources;
	for (AToolBase* Tool : OwnedTools)
	{
		if (Tool)
		{
			TMap<FName, int32> ToolResources = Tool->GetAllResources();
			for (const auto& Pair : ToolResources)
			{
				AllResources.FindOrAdd(Pair.Key) += Pair.Value;
			}
		}
	}
	return AllResources;
}

// -------------------------------------------------------------------------
// INSPECT INVENTORY SYSTEM
// -------------------------------------------------------------------------

void AAlphaExilemetCharacter::StartInspectCurrentTool()
{
	// Guard: must have a tool and not already be inspecting
	if (!CurrentTool || bIsInspecting)
	{
		return;
	}

	bIsInspecting = true;
	BP_OnInspectToolStarted(CurrentTool, CurrentTool->InventoryWidgetClass);
}

void AAlphaExilemetCharacter::StopInspectCurrentTool()
{
	if (!bIsInspecting)
	{
		return;
	}

	bIsInspecting = false;
	BP_OnInspectToolStopped();
}

// -------------------------------------------------------------------------
// INTERACTION
// -------------------------------------------------------------------------
void AAlphaExilemetCharacter::TryInteract()
{
	FVector StartLoc = FirstPersonCameraComponent->GetComponentLocation();
	FVector EndLoc   = StartLoc + (FirstPersonCameraComponent->GetForwardVector() * InteractionDistance);

	FHitResult HitResult;
	FCollisionQueryParams CollisionParams;
	CollisionParams.AddIgnoredActor(this);

	if (GetWorld()->LineTraceSingleByChannel(HitResult, StartLoc, EndLoc, ECC_Visibility, CollisionParams))
	{
		AActor* HitActor = HitResult.GetActor();
		if (HitActor && HitActor->Implements<UInteractable>())
		{
			if (IInteractable::Execute_CanBeInteractedWith(HitActor))
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
	int32 CurrentHealthLevel  = GetSystemStatLevel(EPlayerStat::Health);
	int32 CurrentOxygenLevel  = GetSystemStatLevel(EPlayerStat::Oxygen);
	int32 CurrentAgilityLevel = GetSystemStatLevel(EPlayerStat::Agility);

	MaxHealth = HealthProgression.GetValueAtLevel(CurrentHealthLevel);
	Health = FMath::Clamp(Health, 0.0f, MaxHealth); 

	OxygenDrainRate = OxygenDrainProgression.GetValueAtLevel(CurrentOxygenLevel);

	// One Agility level drives all movement parameters simultaneously
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
	
	GetCharacterMovement()->StopMovementImmediately();
	GetCharacterMovement()->DisableMovement();        

	FirstPersonCameraComponent->SetActive(false);
	DeathCameraComponent->SetActive(true);
	DeathCameraBoom->AttachToComponent(GetMesh(), FAttachmentTransformRules::KeepWorldTransform);

	// --- EMERGENCY MATTER RETRIEVER LOGIC ---
	float RetainedFraction = 0.0f;
	
	if (BaseCampRef)
	{
		int32 RetrieverLevel = BaseCampRef->GetShipSystemLevel(EShipSystem::MatterRetriever);
		float RetainedPercentage = BaseCampRef->RetrieverProgression.GetValueAtLevel(RetrieverLevel);
		RetainedFraction = FMath::Clamp(RetainedPercentage / 100.0f, 0.0f, 1.0f);
	}
	
	if (DeathSound)
	{
		UGameplayStatics::PlaySound2D(this, DeathSound);
	}

	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	GetMesh()->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	GetMesh()->SetCollisionProfileName(TEXT("Ragdoll"));
	GetMesh()->SetAllBodiesSimulatePhysics(true); 
	GetMesh()->WakeAllRigidBodies();
	GetMesh()->SetSimulatePhysics(true);

	BP_OnPlayerDied();

	for (AToolBase* Tool : OwnedTools)
	{
		if (Tool) 
		{
			Tool->ClearInventory(RetainedFraction); 
		}
	}
}

void AAlphaExilemetCharacter::RespawnPlayer(FVector SpawnLocation, FRotator SpawnRotation)
{
	Health = MaxHealth;
	Oxygen = MaxOxygen;
	bIsDead = false;

	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);

	OnHealthChanged.Broadcast(Health, MaxHealth);
	OnOxygenChanged.Broadcast(Oxygen, MaxOxygen);
	
	DeathCameraComponent->SetActive(false);
	FirstPersonCameraComponent->SetActive(true);
	
	GetMesh()->SetAllBodiesSimulatePhysics(false);
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

// -------------------------------------------------------------------------
// SAVE & LOAD (DATA EXTRACTION)
// -------------------------------------------------------------------------
void AAlphaExilemetCharacter::SaveToolDataToSaveObject(UAlphaExilemetSaveGame* SaveObject)
{
	if (!SaveObject) return;
	
	SaveObject->SavedToolUpgrades.Empty();
	SaveObject->SavedOwnedToolClasses.Empty();
	
	SaveObject->SavedActiveToolIndex = ActiveToolIndex;

	for (AToolBase* Tool : OwnedTools)
	{
		if (Tool)
		{
			SaveObject->SavedOwnedToolClasses.Add(Tool->GetClass());
			Tool->SaveToolData(SaveObject);
		}
	}
}

void AAlphaExilemetCharacter::LoadToolDataFromSaveObject(UAlphaExilemetSaveGame* SaveObject)
{
	if (!SaveObject) return;

	for (AToolBase* OldTool : OwnedTools)
	{
		if (OldTool) OldTool->Destroy();
	}
	OwnedTools.Empty();
	CurrentTool = nullptr;
	ActiveToolIndex = -1;

	for (TSubclassOf<AToolBase> ToolClass : SaveObject->SavedOwnedToolClasses)
	{
		if (ToolClass)
		{
			FActorSpawnParameters SpawnParams;
			SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
			
			if (AToolBase* SpawnedTool = GetWorld()->SpawnActor<AToolBase>(ToolClass, GetActorLocation(), GetActorRotation(), SpawnParams))
			{
				SpawnedTool->SetActorEnableCollision(false);
				AddToolToInventory(SpawnedTool);
				SpawnedTool->LoadToolData(SaveObject);
			}
		}
	}

	if (SaveObject->SavedActiveToolIndex >= 0 && SaveObject->SavedActiveToolIndex < OwnedTools.Num())
	{
		StartWieldTool(SaveObject->SavedActiveToolIndex);
	}
}

// -------------------------------------------------------------------------
// SAFE ZONE AUDIO & LOGIC
// -------------------------------------------------------------------------
void AAlphaExilemetCharacter::EnterSafeZone()
{
	bIsInSafeZone = true;

	if (Oxygen < (MaxOxygen * 0.5f) && RecoveryBreathingSound)
	{
		BreathingAudioComponent->SetSound(RecoveryBreathingSound);
		BreathingAudioComponent->Play();

		GetWorldTimerManager().SetTimer(BreathingFadeTimerHandle, this, &AAlphaExilemetCharacter::FadeOutBreathingSound, 4.0f, false);
	}
}

void AAlphaExilemetCharacter::ExitSafeZone()
{
	bIsInSafeZone = false;

	if (BreathingAudioComponent->IsPlaying())
	{
		BreathingAudioComponent->Stop();
		GetWorldTimerManager().ClearTimer(BreathingFadeTimerHandle);
	}
}

void AAlphaExilemetCharacter::FadeOutBreathingSound()
{
	if (BreathingAudioComponent->IsPlaying())
	{
		BreathingAudioComponent->FadeOut(1.0f, 0.0f);
	}
}
