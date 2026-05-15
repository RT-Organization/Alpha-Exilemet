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

	// The arch mesh is the root — no extra scene component needed.
	PortalMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PortalMesh"));
	SetRootComponent(PortalMesh);
	PortalMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	// Walk-through trigger: sits at the interior of the arch.
	// Resize and reposition in the Blueprint viewport until it feels right.
	TriggerBox = CreateDefaultSubobject<UBoxComponent>(TEXT("TriggerBox"));
	TriggerBox->SetupAttachment(PortalMesh);
	TriggerBox->SetBoxExtent(FVector(40.f, 80.f, 90.f));
	TriggerBox->SetCollisionProfileName(TEXT("Trigger"));

	// Per-portal VFX: assign the Niagara System in the Blueprint CDO.
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
	TriggerBox->OnComponentBeginOverlap.AddDynamic(this, &APortalBase::OnOverlapBegin);
}

// ─────────────────────────────────────────────────────────────────────────────
// OVERLAP — entry gate
// ─────────────────────────────────────────────────────────────────────────────

void APortalBase::OnOverlapBegin(UPrimitiveComponent* OverlappedComp,
                                  AActor*             OtherActor,
                                  UPrimitiveComponent* OtherComp,
                                  int32               OtherBodyIndex,
                                  bool                bFromSweep,
                                  const FHitResult&   SweepResult)
{
	// Gate: portal must be active, not on cooldown, overlapper must be the player.
	if (!bPortalActive || bOnCooldown) return;

	AAlphaExilemetCharacter* Player = Cast<AAlphaExilemetCharacter>(OtherActor);
	if (!Player || Player->bIsDead) return;

	EnterPortalLevel(Player);
}

// ─────────────────────────────────────────────────────────────────────────────
// ENTRY — main logic
// ─────────────────────────────────────────────────────────────────────────────

void APortalBase::EnterPortalLevel(AAlphaExilemetCharacter* Player)
{
	if (!Player) return;

	if (PortalLevelName.IsNone())
	{
		UE_LOG(LogTemp, Warning,
			TEXT("APortalBase [%s]: EnterPortalLevel called but PortalLevelName is not set — aborting."),
			*GetName());
		return;
	}

	// ── COOLDOWN — prevents double-trigger during the loading-screen fade ────
	bOnCooldown = true;
	GetWorldTimerManager().SetTimer(
		CooldownHandle, this, &APortalBase::ClearCooldown, TriggerCooldown, false);

	// ── DISABLE SURVIVAL (oxygen drain stops in portal levels) ──────────────
	Player->bIsSurvivalActive = false;

	// ── BLUEPRINT HOOK — other programmer sets up challenge state here ───────
	// Examples: equip pickaxe, set a challenge flag on GI, spawn a guide actor.
	BP_OnPortalSetup(Player);

	// ── BUILD RETURN TRANSFORM ───────────────────────────────────────────────
	// We bake the return-yaw offset NOW so ExitPortal() can restore it directly
	// without extra math. The player's XY position is preserved; only Yaw
	// is rotated so they face back through the arch when they return.
	const FTransform CurrentTransform = Player->GetActorTransform();
	FRotator         ReturnRotation   = CurrentTransform.GetRotation().Rotator();
	ReturnRotation.Yaw += ReturnYawOffset;

	const FTransform ReturnTransform(
		FQuat(ReturnRotation),
		CurrentTransform.GetLocation(),
		FVector::OneVector);   // Scale is always 1

	// ── STREAM — Main stays loaded; portal level is loaded on top ───────────
	// EnterPortal() in AlphaStreamingSubsystem:
	//   1. Saves ReturnTransform as PrePortalTransform in the save object.
	//   2. Calls SavePlayerData() → preserves health, inventory, tools, etc.
	//   3. Fires OnPortalEnterStarted → GameMode BP shows the loading screen.
	//   4. Calls LoadStreamLevel(PortalLevelName) WITHOUT unloading Main.
	//   5. On completion, fires OnStreamComplete → GameMode BP fades out screen.
	if (UGameInstance* GI = GetGameInstance())
	{
		if (UAlphaStreamingSubsystem* Streaming = GI->GetSubsystem<UAlphaStreamingSubsystem>())
		{
			Streaming->EnterPortal(PortalLevelName, ReturnTransform);
		}
	}
}

// ─────────────────────────────────────────────────────────────────────────────
// HELPERS
// ─────────────────────────────────────────────────────────────────────────────

void APortalBase::SetPortalActive(bool bActive)
{
	bPortalActive = bActive;
	// Also disable overlap events so UE doesn't do the cast work on every frame.
	TriggerBox->SetGenerateOverlapEvents(bActive);
}

void APortalBase::ClearCooldown()
{
	bOnCooldown = false;
}