#include "HazardVolume.h"
#include "Components/BoxComponent.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/CapsuleComponent.h" 
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "AlphaExilemetCharacter.h"
#include "BaseCamp.h"

// -------------------------------------------------------------------------
// CONSTRUCTOR
// -------------------------------------------------------------------------
AHazardVolume::AHazardVolume()
{
	PrimaryActorTick.bCanEverTick = true;

	DefaultRoot = CreateDefaultSubobject<USceneComponent>(TEXT("DefaultRoot"));
	RootComponent = DefaultRoot;

	HazardZone = CreateDefaultSubobject<UBoxComponent>(TEXT("HazardZone"));
	HazardZone->SetupAttachment(RootComponent);
	
	HazardSphere = CreateDefaultSubobject<USphereComponent>(TEXT("HazardSphere"));
	HazardSphere->SetupAttachment(RootComponent);

	HazardMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("HazardMesh"));
	HazardMesh->SetupAttachment(RootComponent);

	// Default Setup
	HazardShape = EHazardShape::Box;
	DamagePerSecond = 5.0f;
	SpeedMultiplier = 0.5f; 
	bIsSlippery = false;
	SlipperyFriction = 0.5f;
	SlipperyBraking = 100.0f;

	// Bind Overlaps to ALL shapes. OnConstruction dictates which one generates events.
	HazardZone->OnComponentBeginOverlap.AddDynamic(this, &AHazardVolume::OnOverlapBegin);
	HazardZone->OnComponentEndOverlap.AddDynamic(this, &AHazardVolume::OnOverlapEnd);

	HazardSphere->OnComponentBeginOverlap.AddDynamic(this, &AHazardVolume::OnOverlapBegin);
	HazardSphere->OnComponentEndOverlap.AddDynamic(this, &AHazardVolume::OnOverlapEnd);

	HazardMesh->OnComponentBeginOverlap.AddDynamic(this, &AHazardVolume::OnOverlapBegin);
	HazardMesh->OnComponentEndOverlap.AddDynamic(this, &AHazardVolume::OnOverlapEnd);
}

// -------------------------------------------------------------------------
// ENGINE OVERRIDES
// -------------------------------------------------------------------------
void AHazardVolume::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	// Reset all
	HazardZone->SetCollisionProfileName(TEXT("NoCollision"));
	HazardZone->SetGenerateOverlapEvents(false);
	
	HazardSphere->SetCollisionProfileName(TEXT("NoCollision"));
	HazardSphere->SetGenerateOverlapEvents(false);
	
	HazardMesh->SetCollisionProfileName(TEXT("NoCollision"));
	HazardMesh->SetGenerateOverlapEvents(false);

	// Activate chosen shape
	switch (HazardShape)
	{
		case EHazardShape::Box:
			HazardZone->SetCollisionProfileName(TEXT("Trigger"));
			HazardZone->SetGenerateOverlapEvents(true);
			break;
		case EHazardShape::Sphere:
			HazardSphere->SetCollisionProfileName(TEXT("Trigger"));
			HazardSphere->SetGenerateOverlapEvents(true);
			break;
		case EHazardShape::CustomMesh:
			HazardMesh->SetCollisionProfileName(TEXT("Trigger"));
			HazardMesh->SetGenerateOverlapEvents(true);
			break;
	}
}

void AHazardVolume::BeginPlay()
{
	Super::BeginPlay();
	BaseCampRef = Cast<ABaseCamp>(UGameplayStatics::GetActorOfClass(GetWorld(), ABaseCamp::StaticClass()));
}

void AHazardVolume::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// Apply Damage Over Time
	if (OverlappingPlayer && DamagePerSecond > 0.0f)
	{
		float DampenerMod = 1.0f;
		if (BaseCampRef)
		{
			int32 DampenerLevel = BaseCampRef->GetShipSystemLevel(EShipSystem::HazardDampener);
			DampenerMod = BaseCampRef->DampenerProgression.GetValueAtLevel(DampenerLevel);
		}

		float ActualDamage = (DamagePerSecond * DampenerMod) * DeltaTime;
		OverlappingPlayer->Health = FMath::Clamp(OverlappingPlayer->Health - ActualDamage, 0.0f, OverlappingPlayer->MaxHealth);
		OverlappingPlayer->OnHealthChanged.Broadcast(OverlappingPlayer->Health, OverlappingPlayer->MaxHealth);

		if (OverlappingPlayer->Health <= 0.0f && !OverlappingPlayer->bIsDead)
		{
			OverlappingPlayer->Die();
		}
	}
}

// -------------------------------------------------------------------------
// OVERLAP EVENTS
// -------------------------------------------------------------------------
void AHazardVolume::OnOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	AAlphaExilemetCharacter* Player = Cast<AAlphaExilemetCharacter>(OtherActor);
	
	if (Player && OtherComp == Player->GetCapsuleComponent())
	{
		OverlappingPlayer = Player;
		
		float DampenerMod = 1.0f;
		if (BaseCampRef)
		{
			int32 DampenerLevel = BaseCampRef->GetShipSystemLevel(EShipSystem::HazardDampener);
			DampenerMod = BaseCampRef->DampenerProgression.GetValueAtLevel(DampenerLevel);
		}

		// 1. Slow Movement (e.g., Swamp Water)
		if (SpeedMultiplier < 1.0f)
		{
			Player->HazardSpeedMultiplier = 1.0f - ((1.0f - SpeedMultiplier) * DampenerMod);
		}

		// 2. Slippery Physics (e.g., Frozen Lake)
		if (bIsSlippery)
		{
			Player->GetCharacterMovement()->GroundFriction = SlipperyFriction / DampenerMod;
			Player->GetCharacterMovement()->BrakingDecelerationWalking = SlipperyBraking / DampenerMod;
		}
	}
}

void AHazardVolume::OnOverlapEnd(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	AAlphaExilemetCharacter* Player = Cast<AAlphaExilemetCharacter>(OtherActor);
	
	if (Player && OtherComp == Player->GetCapsuleComponent() && OtherActor == OverlappingPlayer)
	{
		// Reset Speed Multiplier
		OverlappingPlayer->HazardSpeedMultiplier = 1.0f;
		
		// Reset Slippery Physics
		if (bIsSlippery)
		{
			OverlappingPlayer->GetCharacterMovement()->GroundFriction = OverlappingPlayer->DefaultGroundFriction;
			OverlappingPlayer->GetCharacterMovement()->BrakingDecelerationWalking = OverlappingPlayer->DefaultBrakingDeceleration;
		}
		
		OverlappingPlayer->RecalculateStats(); 
		OverlappingPlayer = nullptr;
	}
}