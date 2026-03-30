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

void ABaseTerminal::Interact_Implementation(AAlphaExilemetCharacter* Interactor)
{
	if (!Interactor) return;

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
			false
		);
	}
}

void ABaseTerminal::OnBlendComplete()
{
	BP_OnTerminalViewReady(CurrentInteractor);
}

void ABaseTerminal::StopTerminalInteraction(AAlphaExilemetCharacter* Interactor)
{
	if (!Interactor) return;

	APlayerController* PC = Cast<APlayerController>(Interactor->GetController());
	if (PC)
	{
		PC->SetViewTargetWithBlend(Interactor, CameraBlendTime, EViewTargetBlendFunction::VTBlend_Cubic);

		// DELETE the EnableInput line and ADD these two instead:
		PC->SetIgnoreMoveInput(false);
		PC->SetIgnoreLookInput(false);
	}
}

void ABaseTerminal::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	if (bUseMeshForInteraction)
	{
		// 1. Disable the box so the raycast passes through it
		InteractionBox->SetCollisionResponseToChannel(ECC_Visibility, ECR_Ignore);
        
		// 2. Enable the mesh to block the raycast
		TerminalMesh->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
		TerminalMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	}
	else
	{
		// 1. Enable the box to block the raycast
		InteractionBox->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
		InteractionBox->SetCollisionEnabled(ECollisionEnabled::QueryOnly);

		// 2. Disable the mesh interaction
		TerminalMesh->SetCollisionResponseToChannel(ECC_Visibility, ECR_Ignore);
	}
}