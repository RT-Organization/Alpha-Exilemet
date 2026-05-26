#include "BaseTerminal.h"
#include "Components/BoxComponent.h"
#include "Camera/CameraComponent.h"
#include "Camera/PlayerCameraManager.h"
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
// CanBeInteractedWith
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
	// Reset the close block each time the terminal is freshly opened.
	// This ensures a clean state if the player somehow re-enters.
	if (bRequiresConfirmation)
		bCloseBlocked = false;

	BP_OnTerminalViewReady(CurrentInteractor);
}

// ─────────────────────────────────────────────────────────────────────────────
// BlockTerminalClose
// Call from WBP_ShipTerminal when the warning widget is shown.
// ─────────────────────────────────────────────────────────────────────────────

void ABaseTerminal::BlockTerminalClose()
{
	bCloseBlocked = true;
	UE_LOG(LogTemp, Log, TEXT("ABaseTerminal [%s]: close blocked — waiting for confirmation."),
		*GetName());
}

// ─────────────────────────────────────────────────────────────────────────────
// TryStopTerminalInteraction
// The SAFE close — use on every UI back/close button.
// ─────────────────────────────────────────────────────────────────────────────

void ABaseTerminal::TryStopTerminalInteraction(AAlphaExilemetCharacter* Interactor)
{
	// If this terminal doesn't require confirmation, behave exactly as before.
	if (!bRequiresConfirmation)
	{
		StopTerminalInteraction(Interactor);
		return;
	}

	// Requires confirmation — check if we're still blocked.
	if (bCloseBlocked)
	{
		// Tell the BP it was denied — play a sound, shake the UI, etc.
		BP_OnClosureBlocked();
		UE_LOG(LogTemp, Log,
			TEXT("ABaseTerminal [%s]: close attempt BLOCKED — player must confirm warning first."),
			*GetName());
		return;
	}

	// Confirmation was already given this session — close normally.
	StopTerminalInteraction(Interactor);
}

// ─────────────────────────────────────────────────────────────────────────────
// ConfirmAndCloseTerminal
// Called by WB_ShipRepairWarning OK button.
// ─────────────────────────────────────────────────────────────────────────────

void ABaseTerminal::ConfirmAndCloseTerminal(AAlphaExilemetCharacter* Interactor)
{
	// Clear the lock — confirmation has been given.
	bCloseBlocked = false;

	UE_LOG(LogTemp, Log,
		TEXT("ABaseTerminal [%s]: confirmation received — closing terminal."),
		*GetName());

	// Tell BP to stop the alarm, remove the warning widget, etc.
	// This fires BEFORE the camera starts blending so BP can clean up UI first.
	BP_OnConfirmationReceived();

	// Now begin the camera blend back to the player.
	StopTerminalInteraction(Interactor);
}

// ─────────────────────────────────────────────────────────────────────────────
// StopTerminalInteraction  (direct / programmatic close — unchanged from v2)
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

void ABaseTerminal::RestoreInput()
{
	if (!CurrentInteractor) return;

	APlayerController* PC = Cast<APlayerController>(CurrentInteractor->GetController());
	if (PC)
	{
		PC->SetIgnoreMoveInput(false);
		PC->SetIgnoreLookInput(false);
	}

	bIsInteracting = false;

	// Camera is fully back — player has control — safe to remove UI.
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
