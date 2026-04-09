#include "HazardVolume.h"
#include "Components/BoxComponent.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/CapsuleComponent.h" // Needed to verify the player's physical body
#include "AlphaExilemetCharacter.h"
#include "BaseCamp.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/CharacterMovementComponent.h"

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

	DamagePerSecond = 5.0f;
	SpeedMultiplier = 0.5f; 
	HazardShape = EHazardShape::Box; // Default to Box

	// Bind Overlaps to ALL shapes. OnConstruction manages which one fires.
	HazardZone->OnComponentBeginOverlap.AddDynamic(this, &AHazardVolume::OnOverlapBegin);
	HazardZone->OnComponentEndOverlap.AddDynamic(this, &AHazardVolume::OnOverlapEnd);

	HazardSphere->OnComponentBeginOverlap.AddDynamic(this, &AHazardVolume::OnOverlapBegin);
	HazardSphere->OnComponentEndOverlap.AddDynamic(this, &AHazardVolume::OnOverlapEnd);

	HazardMesh->OnComponentBeginOverlap.AddDynamic(this, &AHazardVolume::OnOverlapBegin);
	HazardMesh->OnComponentEndOverlap.AddDynamic(this, &AHazardVolume::OnOverlapEnd);
}

void AHazardVolume::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	// 1. Turn OFF collision for all shapes first
	HazardZone->SetCollisionProfileName(TEXT("NoCollision"));
	HazardZone->SetGenerateOverlapEvents(false);
	
	HazardSphere->SetCollisionProfileName(TEXT("NoCollision"));
	HazardSphere->SetGenerateOverlapEvents(false);
	
	HazardMesh->SetCollisionProfileName(TEXT("NoCollision"));
	HazardMesh->SetGenerateOverlapEvents(false);

	// 2. Turn ON collision only for the selected shape
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
	AAlphaExilemetCharacter* Player = Cast<AAlphaExilemetCharacter>(OtherActor);
	
	// CRUCIAL FIX: Only trigger if the component overlapping is the actual physical Capsule!
	// This prevents the giant invisible Scanner Sphere from triggering the swamp early.
	if (Player && OtherComp == Player->GetCapsuleComponent())
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

			// Apply the slow math to the BaseWalkSpeed so it stacks correctly
			float ActualSpeedMultiplier = 1.0f - ((1.0f - SpeedMultiplier) * DampenerMod);
			Player->GetCharacterMovement()->MaxWalkSpeed = Player->BaseWalkSpeed * ActualSpeedMultiplier;
		}
	}
}

void AHazardVolume::OnOverlapEnd(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	AAlphaExilemetCharacter* Player = Cast<AAlphaExilemetCharacter>(OtherActor);
	
	// Only trigger the reset if the Capsule leaves
	if (Player && OtherComp == Player->GetCapsuleComponent() && OtherActor == OverlappingPlayer)
	{
		OverlappingPlayer->RecalculateStats(); // Resets speed back to normal
		OverlappingPlayer = nullptr;
	}
}