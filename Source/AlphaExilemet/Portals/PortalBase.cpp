#include "PortalBase.h"

#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "NiagaraComponent.h"
#include "AlphaExilemet/AlphaExilemetCharacter.h"
#include "AlphaExilemet/Core/AlphaStreamingSubsystem.h"
#include "Kismet/GameplayStatics.h"

// ─────────────────────────────────────────────────────────────────────────────
// CONSTRUCTION
// ─────────────────────────────────────────────────────────────────────────────

APortalBase::APortalBase()
{
	PrimaryActorTick.bCanEverTick = false;

	// ── MESH ──────────────────────────────────────────────────────────────────
	// Root is the mesh. Collision is DISABLED — the TriggerBox handles overlaps.
	PortalMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PortalMesh"));
	SetRootComponent(PortalMesh);
	PortalMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	// ── TRIGGER BOX ───────────────────────────────────────────────────────────
	// Attached to the mesh at origin.
	// Default extents: 50×130×130 cm — covers a standard arch opening.
	//
	// IMPORTANT: if you scale the portal Actor (not just the mesh) in the editor,
	// the TriggerBox extents scale with it.
	//   Portal actor scale 7.5 + TriggerBox extent 50×130×130
	//   → world size 375×975×975 cm — FAR too large.
	//
	// Best practice: keep Actor scale = 1 and scale PortalMesh component instead.
	// OR override TriggerBoxExtent in the Blueprint CDO and adjust manually.
	TriggerBox = CreateDefaultSubobject<UBoxComponent>(TEXT("TriggerBox"));
	TriggerBox->SetupAttachment(PortalMesh);
	TriggerBox->SetBoxExtent(TriggerBoxExtent);

	// ── COLLISION — explicit setup, never rely on named profiles ─────────────
	//
	// NoCollision           = zero physics presence, cannot be walked on.
	//                         Solves the "invisible step" the player trips on.
	// QueryOnly             = still generates overlap queries even with NoCollision.
	// ECC_Pawn Overlap      = only the player capsule (Pawn channel) triggers this.
	// SetGenerateOverlapEvents(true) = required for OnComponentBeginOverlap to fire.
	//
	// Using SetCollisionProfileName("OverlapOnlyPawn") is NOT enough because that
	// profile sets the object type to WorldDynamic, which can still block movement
	// and act as a step depending on the project's collision matrix.
	// Setting everything explicitly removes any dependency on project collision settings.

	TriggerBox->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	TriggerBox->SetCollisionObjectType(ECC_WorldDynamic);

	// Ignore everything by default, then selectively overlap Pawn.
	TriggerBox->SetCollisionResponseToAllChannels(ECR_Ignore);
	TriggerBox->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);

	TriggerBox->SetGenerateOverlapEvents(true);

	// Prevent UE from treating this box as a walkable step surface.
	TriggerBox->CanCharacterStepUpOn = ECB_No;

	// ── VFX ───────────────────────────────────────────────────────────────────
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

	// Re-apply extent from CDO property (may have been changed in the BP editor).
	TriggerBox->SetBoxExtent(TriggerBoxExtent);

	// Re-apply collision explicitly at runtime.
	// This overrides anything the editor might have serialised onto the component,
	// ensuring the trigger always works regardless of what the Details panel shows.
	TriggerBox->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	TriggerBox->SetCollisionObjectType(ECC_WorldDynamic);
	TriggerBox->SetCollisionResponseToAllChannels(ECR_Ignore);
	TriggerBox->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	TriggerBox->SetGenerateOverlapEvents(true);
	TriggerBox->CanCharacterStepUpOn = ECB_No;

	TriggerBox->OnComponentBeginOverlap.AddDynamic(this, &APortalBase::OnOverlapBegin);
}

// ─────────────────────────────────────────────────────────────────────────────
// OVERLAP — immediate entry
//
// Fires the moment the player capsule intersects the TriggerBox.
// No hold-time, no delay — entry is instantaneous.
// The cooldown prevents double-fires that would happen if the loading
// screen takes a moment to appear and the player is still moving forward.
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
// ENTRY
// ─────────────────────────────────────────────────────────────────────────────

void APortalBase::EnterPortalLevel(AAlphaExilemetCharacter* Player)
{
	if (!Player) return;

	if (PortalLevelName.IsNone())
	{
		UE_LOG(LogTemp, Warning,
			TEXT("APortalBase [%s]: PortalLevelName is not set — aborting entry."), *GetName());
		return;
	}

	// Lock trigger for TriggerCooldown seconds.
	bOnCooldown = true;
	GetWorldTimerManager().SetTimer(
		CooldownHandle, this, &APortalBase::ClearCooldown, TriggerCooldown, false);

	// Disable survival immediately — oxygen won't drain inside the challenge.
	Player->bIsSurvivalActive = false;

	// Per-portal Blueprint hook (equip tool, set challenge flag, play VO, etc.)
	BP_OnPortalSetup(Player);

	// Build the return transform with yaw offset baked in so ExitPortal()
	// can restore it directly with no extra math.
	FRotator ReturnRotation = Player->GetActorRotation();
	ReturnRotation.Yaw     += ReturnYawOffset;

	const FTransform ReturnTransform(
		FQuat(ReturnRotation),
		Player->GetActorLocation(),
		FVector::OneVector);

	// Hand off to the subsystem — it handles loading screen, save, streaming.
	if (UGameInstance* GI = GetGameInstance())
	{
		if (UAlphaStreamingSubsystem* SS = GI->GetSubsystem<UAlphaStreamingSubsystem>())
		{
			SS->EnterPortal(PortalLevelName, ReturnTransform);
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