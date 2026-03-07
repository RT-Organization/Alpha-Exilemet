#include "BaseCamp.h"
#include "Components/SphereComponent.h"
#include "AlphaExilemetCharacter.h"
ABaseCamp::ABaseCamp()
{
	PrimaryActorTick.bCanEverTick = true;
	OxygenRegenRadius = 500.f;
	OxygenRegenRate = 10.f;
	OxygenSphere = CreateDefaultSubobject<USphereComponent>(TEXT("OxygenSphere"));
	OxygenSphere->InitSphereRadius(OxygenRegenRadius);
	OxygenSphere->SetCollisionProfileName(TEXT("OverlapAllDynamic"));
	RootComponent = OxygenSphere;
}

void ABaseCamp::BeginPlay()
{
	Super::BeginPlay();
	OxygenSphere->SetSphereRadius(OxygenRegenRadius);
}

void ABaseCamp::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	
	TArray<AActor*> OverlappingActors;
	OxygenSphere->GetOverlappingActors(OverlappingActors, AAlphaExilemetCharacter::StaticClass());
	for (AActor* Actor : OverlappingActors)
	{
		if (AAlphaExilemetCharacter* Character = Cast<AAlphaExilemetCharacter>(Actor))
		{
			Character->Oxygen = FMath::Clamp(Character->Oxygen +
			OxygenRegenRate * DeltaTime, 0.f, Character->MaxOxygen);
		}
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
