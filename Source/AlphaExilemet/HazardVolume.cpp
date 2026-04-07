#include "HazardVolume.h"
#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "AlphaExilemetCharacter.h"
#include "BaseCamp.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/CharacterMovementComponent.h"

AHazardVolume::AHazardVolume()
{
	PrimaryActorTick.bCanEverTick = true;

	// 1. Setup Root
	DefaultRoot = CreateDefaultSubobject<USceneComponent>(TEXT("DefaultRoot"));
	RootComponent = DefaultRoot;

	// 2. Setup Box
	HazardZone = CreateDefaultSubobject<UBoxComponent>(TEXT("HazardZone"));
	HazardZone->SetupAttachment(RootComponent);
	
	// 3. Setup Mesh
	HazardMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("HazardMesh"));
	HazardMesh->SetupAttachment(RootComponent);

	// Default hazard settings
	DamagePerSecond = 5.0f;
	SpeedMultiplier = 0.5f; 
	bUseMeshForOverlap = false;

	// Bind Overlaps to BOTH. OnConstruction will manage which one actually fires.
	HazardZone->OnComponentBeginOverlap.AddDynamic(this, &AHazardVolume::OnOverlapBegin);
	HazardZone->OnComponentEndOverlap.AddDynamic(this, &AHazardVolume::OnOverlapEnd);

	HazardMesh->OnComponentBeginOverlap.AddDynamic(this, &AHazardVolume::OnOverlapBegin);
	HazardMesh->OnComponentEndOverlap.AddDynamic(this, &AHazardVolume::OnOverlapEnd);
}

void AHazardVolume::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	// This runs in the Editor. It toggles collision based on your checkbox.
	if (bUseMeshForOverlap)
	{
		// Turn OFF the Box
		HazardZone->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		
		// Turn ON the Mesh as a trigger
		HazardMesh->SetCollisionProfileName(TEXT("Trigger"));
		HazardMesh->SetGenerateOverlapEvents(true);
	}
	else
	{
		// Turn ON the Box as a trigger
		HazardZone->SetCollisionProfileName(TEXT("Trigger"));
		HazardZone->SetGenerateOverlapEvents(true);

		// Turn OFF the Mesh overlap (but keep it visible)
		HazardMesh->SetCollisionProfileName(TEXT("NoCollision"));
		HazardMesh->SetGenerateOverlapEvents(false);
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

void AHazardVolume::OnOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (AAlphaExilemetCharacter* Player = Cast<AAlphaExilemetCharacter>(OtherActor))
	{
		OverlappingPlayer = Player;

		if (SpeedMultiplier < 1.0f)
		{
			float DampenerMod = 1.0f;
			if (BaseCampRef)
			{
				int32 DampenerLevel = BaseCampRef->GetShipSystemLevel(EShipSystem::HazardDampener);
				DampenerMod = BaseCampRef->DampenerProgression.GetValueAtLevel(DampenerLevel);
			}

			float ActualSpeedMultiplier = 1.0f - ((1.0f - SpeedMultiplier) * DampenerMod);
			Player->GetCharacterMovement()->MaxWalkSpeed *= ActualSpeedMultiplier;
		}
	}
}

void AHazardVolume::OnOverlapEnd(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	if (OtherActor == OverlappingPlayer)
	{
		OverlappingPlayer->RecalculateStats();
		OverlappingPlayer = nullptr;
	}
}