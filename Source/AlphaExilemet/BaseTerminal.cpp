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
// Returns false while bIsInteracting so the prompt is hidden and the player
// cannot spam-interact during camera transitions.
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

	// Lock immediately — prevents double-interaction during the blend.
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
	// Camera has arrived at the terminal view — tell BP to show the UI.
	BP_OnTerminalViewReady(CurrentInteractor);
}

// ─────────────────────────────────────────────────────────────────────────────
// StopTerminalInteraction
// Called by the Blueprint UI when the player closes the terminal.
// Starts the camera blend back to the player. Input is restored only after
// the blend completes to prevent the player looking around mid-animation.
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

	// Unlock AFTER the camera has fully returned to the player.
	// This is the correct moment — the player now has full control and the
	// terminal is visible in the world again, so they can interact again.
	bIsInteracting = false;
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
