#include "BaseCamp.h"
#include "Components/SphereComponent.h"
#include "AlphaExilemetCharacter.h"

ABaseCamp::ABaseCamp()
{
	// Disable Tick completely to improve performance
	PrimaryActorTick.bCanEverTick = false; 

	OxygenRegenRadius = 500.f;
	
	OxygenSphere = CreateDefaultSubobject<USphereComponent>(TEXT("OxygenSphere"));
	OxygenSphere->InitSphereRadius(OxygenRegenRadius);
	OxygenSphere->SetCollisionProfileName(TEXT("OverlapAllDynamic"));
	RootComponent = OxygenSphere;

	// Bind the Overlap Events
	OxygenSphere->OnComponentBeginOverlap.AddDynamic(this, &ABaseCamp::OnOverlapBegin);
	OxygenSphere->OnComponentEndOverlap.AddDynamic(this, &ABaseCamp::OnOverlapEnd);
}

void ABaseCamp::BeginPlay()
{
	Super::BeginPlay();
	OxygenSphere->SetSphereRadius(OxygenRegenRadius);
	
	TArray<AActor*> OverlappingActors;
	OxygenSphere->GetOverlappingActors(OverlappingActors, AAlphaExilemetCharacter::StaticClass());

	for (AActor* Actor : OverlappingActors)
	{
		if (AAlphaExilemetCharacter* Character = Cast<AAlphaExilemetCharacter>(Actor))
		{
			Character->bIsInSafeZone = true;
		}
	}
}

void ABaseCamp::OnOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	// If the player steps into the base camp, set them to safe
	if (AAlphaExilemetCharacter* Character = Cast<AAlphaExilemetCharacter>(OtherActor))
	{
		Character->bIsInSafeZone = true;
	}
}

void ABaseCamp::OnOverlapEnd(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	// If the player leaves the base camp, they are no longer safe
	if (AAlphaExilemetCharacter* Character = Cast<AAlphaExilemetCharacter>(OtherActor))
	{
		Character->bIsInSafeZone = false;
	}
}

void ABaseCamp::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	
	if (OxygenSphere)
	{
		OxygenSphere->SetSphereRadius(OxygenRegenRadius);
	}
}