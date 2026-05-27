#include "BaseTerminal.h"
#include "Components/BoxComponent.h"
#include "Camera/CameraComponent.h"
#include "TimerManager.h"
#include "AlphaExilemetCharacter.h"

ABaseTerminal::ABaseTerminal()
{
	PrimaryActorTick.bCanEverTick = false;
	
	USceneComponent* DummyRoot = CreateDefaultSubobject<USceneComponent>(TEXT("DefaultSceneRoot"));
	RootComponent = DummyRoot;
	
	InteractionBox = CreateDefaultSubobject<UBoxComponent>(TEXT("InteractionBox"));
	InteractionBox->SetupAttachment(RootComponent);

	TerminalMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("TerminalMesh"));
	TerminalMesh->SetupAttachment(RootComponent);
	
	TerminalCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("TerminalCamera"));
	TerminalCamera->SetupAttachment(RootComponent);

	bUseMeshForInteraction = false;
}

void ABaseTerminal::BeginPlay()
{
	Super::BeginPlay();
}

// ─────────────────────────────────────────────────────────────────────────────
// CanBeInteractedWith — hides prompt during camera blend
// ─────────────────────────────────────────────────────────────────────────────

bool ABaseTerminal::CanBeInteractedWith_Implementation() const
{
	return !bIsInteracting;
}

// ─────────────────────────────────────────────────────────────────────────────
// Interact_Implementation
// ─────────────────────────────────────────────────────────────────────────────

void ABaseTerminal::Interact_Implementation(AAlphaExilemetCharacter* Interactor)
{
	if (!Interactor) return;

	bIsInteracting = true;

	APlayerController* PC = Cast<APlayerController>(Interactor->GetController());
	if (PC)
	{
		CurrentInteractor = Interactor;
		
		PC->SetIgnoreMoveInput(true);
		PC->SetIgnoreLookInput(true);

		PC->SetViewTargetWithBlend(this, CameraBlendTime, EViewTargetBlendFunction::VTBlend_Cubic);

		GetWorld()->GetTimerManager().SetTimer(
			CameraBlendTimerHandle,
			this,
			&ABaseTerminal::OnBlendComplete,
			CameraBlendTime,
			false);
	}
}

void ABaseTerminal::OnBlendComplete()
{
	// Always reset the close block on a fresh open so there's no stale state
	// if the player somehow re-enters (e.g. after a non-confirmation close).
	bCloseBlocked = false;

	BP_OnTerminalViewReady(CurrentInteractor);
}

// ─────────────────────────────────────────────────────────────────────────────
// BlockTerminalClose
// Called by WBP_ShipTerminal after showing the warning widget.
// ─────────────────────────────────────────────────────────────────────────────

void ABaseTerminal::BlockTerminalClose()
{
	bCloseBlocked = true;
	UE_LOG(LogTemp, Log,
		TEXT("ABaseTerminal [%s]: close blocked — waiting for player confirmation."),
		*GetName());
}

// ─────────────────────────────────────────────────────────────────────────────
// TryStopTerminalInteraction
// Called by BP_Player when the Interact key is pressed and ActiveTerminal is valid.
// Replaces the old "CloseTerminal" custom event call from BP_Player.
// ─────────────────────────────────────────────────────────────────────────────

void ABaseTerminal::TryStopTerminalInteraction(AAlphaExilemetCharacter* Interactor)
{
	// Terminals that don't need confirmation always close freely.
	if (!bRequiresConfirmation)
	{
		StopTerminalInteraction(Interactor);
		return;
	}

	// Needs confirmation — check if still blocked.
	if (bCloseBlocked)
	{
		// Tell BP to play a denied sound, shake UI, etc.
		BP_OnClosureBlocked();
		UE_LOG(LogTemp, Log,
			TEXT("ABaseTerminal [%s]: close DENIED — warning not confirmed yet."),
			*GetName());
		return;
	}

	// Confirmation was already given this session — close normally.
	StopTerminalInteraction(Interactor);
}

// ─────────────────────────────────────────────────────────────────────────────
// ConfirmAndCloseTerminal
// Called by WB_ShipRepairWarning OK button via BP_Player.ActiveTerminal.
// ─────────────────────────────────────────────────────────────────────────────

void ABaseTerminal::ConfirmAndCloseTerminal(AAlphaExilemetCharacter* Interactor)
{
	// 1. Clear the lock.
	bCloseBlocked = false;

	// 2. Notify BP — stop alarm here (BP_ShipTerminal already has this wired).
	BP_OnConfirmationReceived();

	// 3. Begin camera blend back. RestoreInput fires at the end, then
	//    BP_OnTerminalClosed fires to safely remove the widget.
	StopTerminalInteraction(Interactor);

	UE_LOG(LogTemp, Log,
		TEXT("ABaseTerminal [%s]: confirmed — starting close sequence."), *GetName());
}

// ─────────────────────────────────────────────────────────────────────────────
// StopTerminalInteraction — starts camera blend back to player
// ─────────────────────────────────────────────────────────────────────────────

void ABaseTerminal::StopTerminalInteraction(AAlphaExilemetCharacter* Interactor)
{
	if (!Interactor) return;

	APlayerController* PC = Cast<APlayerController>(Interactor->GetController());
	if (PC)
	{
		PC->SetViewTargetWithBlend(Interactor, CameraBlendTime, EViewTargetBlendFunction::VTBlend_Cubic);
		CurrentInteractor = Interactor;

		GetWorld()->GetTimerManager().SetTimer(
			StopBlendTimerHandle,
			this,
			&ABaseTerminal::RestoreInput,
			CameraBlendTime,
			false);
	}
}

// ─────────────────────────────────────────────────────────────────────────────
// RestoreInput — fires after blend-out completes
// ─────────────────────────────────────────────────────────────────────────────

void ABaseTerminal::RestoreInput()
{
	if (!CurrentInteractor) return;

	APlayerController* PC = Cast<APlayerController>(CurrentInteractor->GetController());
	if (PC)
	{
		PC->SetIgnoreMoveInput(false);
		PC->SetIgnoreLookInput(false);
	}

	// Unlock interaction prompt.
	bIsInteracting = false;

	// Camera is fully back — safe to remove widgets now.
	// BP_BaseTerminal EventGraph implements this (replaces CloseTerminal custom event body).
	BP_OnTerminalClosed();
}

void ABaseTerminal::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	if (bUseMeshForInteraction)
	{
		InteractionBox->SetCollisionResponseToChannel(ECC_Visibility, ECR_Ignore);
		TerminalMesh->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
		TerminalMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	}
	else
	{
		InteractionBox->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
		InteractionBox->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
		TerminalMesh->SetCollisionResponseToChannel(ECC_Visibility, ECR_Ignore);
	}
}
