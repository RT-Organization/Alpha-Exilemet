#include "GasSphere.h"
#include "GasRodTool.h"
#include "AlphaExilemet/Resources/GasResource.h"
#include "AlphaExilemet/AlphaExilemetCharacter.h"
#include "Components/StaticMeshComponent.h"

AGasSphere::AGasSphere()
{
	PrimaryActorTick.bCanEverTick = true;
	
	Mesh = CreateDefaultSubobject<UStaticMeshComponent>("Mesh");
	SetRootComponent(Mesh);
	
	Mesh->SetSimulatePhysics(true);
	Mesh->SetCollisionProfileName(TEXT("PhysicsActor"));
	Mesh->SetCollisionResponseToChannel(ECC_Pawn, ECR_Ignore);
}

void AGasSphere::BeginPlay()
{
	Super::BeginPlay();
}

void AGasSphere::NotifyActorBeginOverlap(AActor* OtherActor)
{
	Super::NotifyActorBeginOverlap(OtherActor);
	
	if (bIsAttached || bIsFull) return;
	if (!OtherActor) return;
	
	AGasResource* Gas = Cast<AGasResource>(OtherActor);
	if (!Gas) return;
	
	TryAttachToGas(Gas);
}

void AGasSphere::InitSphere(AGasRodTool* InOwnerTool, float InDamagePerSecond)
{
	OwnerTool = InOwnerTool;
	DamagePerSecond = InDamagePerSecond;
}

void AGasSphere::TryAttachToGas(AGasResource* Gas)
{
	if (!Gas || bIsAttached || bIsFull) return;
	if (!Gas->TryReserve(this)) return;
	
	TargetGas = Gas;
	GasType = Gas->GetGasType();
	
	bIsAttached = true;
	
	Velocity = Mesh->GetPhysicsLinearVelocity();
	Mesh->SetSimulatePhysics(false);
	
	// --- Duration from gameplay ---
	float GasHealth = Gas->GetHealth();
	TimeToKill = FMath::Max(0.01f, GasHealth / DamagePerSecond);
	
	ElapsedTime = 0.f;
	
	// --- TRUE starting offset (fixes snapping) ---
	InitialOffset = GetActorLocation() - Gas->GetActorLocation();
	InitialRadius = InitialOffset.Size();
	
	// Random start angle
	CurrentAngle = FMath::FRandRange(0.f, PI * 2.f);
	
	FVector ToSphere = InitialOffset.GetSafeNormal();
	FVector VelDir = Velocity.GetSafeNormal();
	
	// Cross product tells us rotation direction
	FVector Cross = FVector::CrossProduct(ToSphere, VelDir);
	
	// Use Z sign (since you're orbiting around UpVector)
	OrbitDirection = (Cross.Z >= 0.f) ? 1.f : -1.f;
}

void AGasSphere::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	
	if (!bIsAttached || bIsFull || !TargetGas) return;
	
	ElapsedTime += DeltaTime;
	
	float Alpha = FMath::Clamp(ElapsedTime / TimeToKill, 0.f, 1.f);
	
	float EasedAlpha = FMath::InterpEaseInOut(0.f, 1.f, Alpha, SpiralEasePower);
	
	// Shrink radius
	float CurrentRadius = FMath::Lerp(InitialRadius, 0.f, EasedAlpha);
	
	// Speed up rotation
	float SpeedMul = FMath::Lerp(1.f, OrbitSpeedMultiplier, EasedAlpha);
	CurrentAngle += OrbitDirection * OrbitSpeed * SpeedMul * DeltaTime;
	
	// --- TRUE 3D ORBIT ---
	FVector StartDir = InitialOffset.GetSafeNormal();
	
	// Rotate around Z (can be changed later)
	FVector RotatedDir = StartDir.RotateAngleAxis(FMath::RadiansToDegrees(CurrentAngle), FVector::UpVector);
	
	FVector NewOffset = RotatedDir * CurrentRadius;
	
	// Preserve original vertical start → collapse naturally
	NewOffset.Z = FMath::Lerp(InitialOffset.Z, 0.f, EasedAlpha);
	
	FVector TargetLocation = TargetGas->GetActorLocation() + NewOffset;
	FVector CurrentLocation = GetActorLocation();
	
	// Spring toward target
	float Stiffness = FMath::Lerp(15.f, 60.f, EasedAlpha);
	float Damping = 8.f;
	
	FVector Force = (TargetLocation - CurrentLocation) * Stiffness;
	Velocity += Force * DeltaTime;
	
	// Damping
	Velocity *= (1.f - FMath::Clamp(Damping * DeltaTime, 0.f, 1.f));
	
	SetActorLocation(CurrentLocation + Velocity * DeltaTime);
	
	// --- DAMAGE ---
	bool bKilled = TargetGas->ApplyResourceDamage(DamagePerSecond * DeltaTime);
	
	if (bKilled)
	{
		bIsFull = true;
		
		if (TargetGas)
		{
			TargetGas->ReleaseReservation();
		}
		
		bIsAttached = false;
		TargetGas = nullptr;
		
		Mesh->SetSimulatePhysics(true);
	}
}

void AGasSphere::Interact_Implementation(AAlphaExilemetCharacter* Interactor)
{
	if (!bIsFull || !Interactor || !Interactor->CurrentTool) return;
	
	AGasRodTool* Rod = Cast<AGasRodTool>(Interactor->CurrentTool);
	if (!Rod) return;
	
	bool bAdded = Rod->ReturnFullSphere(GasType);
	
	if (bAdded)
	{
		Destroy();
	}
}

void AGasSphere::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);
	
	if (TargetGas)
	{
		TargetGas->ReleaseReservation();
	}
}