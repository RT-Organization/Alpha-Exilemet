#include "PortalBase.h"

#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "NiagaraComponent.h"
#include "AlphaExilemet/AlphaExilemetCharacter.h"
#include "AlphaExilemet/Core/LevelStreamingManager.h"
#include "Kismet/GameplayStatics.h"

// ─────────────────────────────────────────────────────────────────────────────
// CONSTRUCTION
// ─────────────────────────────────────────────────────────────────────────────

APortalBase::APortalBase()
{
	PrimaryActorTick.bCanEverTick = false;

	PortalMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PortalMesh"));
	SetRootComponent(PortalMesh);
	PortalMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	TriggerBox = CreateDefaultSubobject<UBoxComponent>(TEXT("TriggerBox"));
	TriggerBox->SetupAttachment(PortalMesh);
	TriggerBox->SetBoxExtent(TriggerBoxExtent);

	TriggerBox->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	TriggerBox->SetCollisionObjectType(ECC_WorldDynamic);
	TriggerBox->SetCollisionResponseToAllChannels(ECR_Ignore);
	TriggerBox->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	TriggerBox->SetGenerateOverlapEvents(true);
	TriggerBox->CanCharacterStepUpOn = ECB_No;

	PortalVFX = CreateDefaultSubobject<UNiagaraComponent>(TEXT("PortalVFX"));
	PortalVFX->SetupAttachment(PortalMesh);
	PortalVFX->SetAutoActivate(true);
}

// ─────────────────────────────────────────────────────────────────────────────
// BEGIN PLAY
// ─────────────────────────────────────────────────────────────────────────────

void APortalBase::BeginPlay()
{
	Super::BeginPlay();

	TriggerBox->SetBoxExtent(TriggerBoxExtent);
	TriggerBox->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	TriggerBox->SetCollisionObjectType(ECC_WorldDynamic);
	TriggerBox->SetCollisionResponseToAllChannels(ECR_Ignore);
	TriggerBox->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	TriggerBox->SetGenerateOverlapEvents(true);
	TriggerBox->CanCharacterStepUpOn = ECB_No;

	TriggerBox->OnComponentBeginOverlap.AddDynamic(this, &APortalBase::OnOverlapBegin);
}

// ─────────────────────────────────────────────────────────────────────────────
// OVERLAP
// ─────────────────────────────────────────────────────────────────────────────

void APortalBase::OnOverlapBegin(UPrimitiveComponent* OverlappedComp,
                                  AActor*             OtherActor,
                                  UPrimitiveComponent* OtherComp,
                                  int32               OtherBodyIndex,
                                  bool                bFromSweep,
                                  const FHitResult&   SweepResult)
{
	if (!bPortalActive || bOnCooldown) return;

	AAlphaExilemetCharacter* Player = Cast<AAlphaExilemetCharacter>(OtherActor);
	if (!Player || Player->bIsDead) return;

	EnterPortalLevel(Player);
}

// ─────────────────────────────────────────────────────────────────────────────
// ENTRY — uses LevelStreamingManager with typed EGameLevel
// ─────────────────────────────────────────────────────────────────────────────

void APortalBase::EnterPortalLevel(AAlphaExilemetCharacter* Player)
{
	if (!Player) return;

	if (PortalLevelName.IsNone())
	{
		UE_LOG(LogTemp, Warning,
			TEXT("APortalBase [%s]: PortalLevelName is not set."), *GetName());
		return;
	}

	// Convert FName to EGameLevel.
	EGameLevel PortalEnum = ULevelStreamingManager::NameToLevel(PortalLevelName);
	if (PortalEnum == EGameLevel::None || !ULevelStreamingManager::IsPortalLevel(PortalEnum))
	{
		UE_LOG(LogTemp, Warning,
			TEXT("APortalBase [%s]: PortalLevelName '%s' is not a valid portal level. "
			     "Make sure it matches an EGameLevel entry."),
			*GetName(), *PortalLevelName.ToString());
		return;
	}

	// Cooldown.
	bOnCooldown = true;
	GetWorldTimerManager().SetTimer(
		CooldownHandle, this, &APortalBase::ClearCooldown, TriggerCooldown, false);

	// Per-portal Blueprint hook.
	BP_OnPortalSetup(Player);

	// Build return transform with yaw offset.
	FRotator ReturnRotation = Player->GetActorRotation();
	ReturnRotation.Yaw     += ReturnYawOffset;

	const FTransform ReturnTransform(
		FQuat(ReturnRotation),
		Player->GetActorLocation(),
		FVector::OneVector);

	// Hand off to LevelStreamingManager.
	if (UGameInstance* GI = GetGameInstance())
	{
		if (ULevelStreamingManager* M = GI->GetSubsystem<ULevelStreamingManager>())
		{
			M->EnterPortal(PortalEnum, ReturnTransform);
		}
	}
}

// ─────────────────────────────────────────────────────────────────────────────
// HELPERS
// ─────────────────────────────────────────────────────────────────────────────

void APortalBase::SetPortalActive(bool bActive)
{
	bPortalActive = bActive;
	TriggerBox->SetGenerateOverlapEvents(bActive);
}

void APortalBase::ClearCooldown()
{
	bOnCooldown = false;
}
