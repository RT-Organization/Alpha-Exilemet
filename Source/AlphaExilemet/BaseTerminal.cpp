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

bool ABaseTerminal::CanBeInteractedWith_Implementation() const
{
	return !bIsInteracting;
}

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
			CameraBlendTimerHandle, this,
			&ABaseTerminal::OnBlendComplete, CameraBlendTime, false);
	}
}

void ABaseTerminal::OnBlendComplete()
{
	bCloseBlocked = false;
	BP_OnTerminalViewReady(CurrentInteractor);
}

void ABaseTerminal::BlockTerminalClose()
{
	bCloseBlocked = true;
}

void ABaseTerminal::TryStopTerminalInteraction(AAlphaExilemetCharacter* Interactor)
{
	if (!bRequiresConfirmation)
	{
		StopTerminalInteraction(Interactor);
		return;
	}

	if (bCloseBlocked)
	{
		BP_OnClosureBlocked();
		return;
	}

	StopTerminalInteraction(Interactor);
}

void ABaseTerminal::ConfirmAndCloseTerminal(AAlphaExilemetCharacter* Interactor)
{
	bCloseBlocked = false;
	BP_OnConfirmationReceived();
	StopTerminalInteraction(Interactor);
}

void ABaseTerminal::StopTerminalInteraction(AAlphaExilemetCharacter* Interactor)
{
	if (!Interactor) return;

	// ── IMMEDIATE: fire the "closing started" event so BP can remove the
	// widget RIGHT NOW, before the camera starts moving back.
	BP_OnTerminalClosingStarted();

	APlayerController* PC = Cast<APlayerController>(Interactor->GetController());
	if (PC)
	{
		CurrentInteractor = Interactor;
		PC->SetViewTargetWithBlend(Interactor, CameraBlendTime,
			EViewTargetBlendFunction::VTBlend_Cubic);

		GetWorld()->GetTimerManager().SetTimer(
			StopBlendTimerHandle, this,
			&ABaseTerminal::RestoreInput, CameraBlendTime, false);
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

	// Camera fully back — safe to restore HUD and input mode.
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