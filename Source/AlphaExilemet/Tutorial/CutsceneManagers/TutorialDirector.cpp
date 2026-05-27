#include "TutorialDirector.h"
#include "AlphaExilemet/Tutorial/SkullProp.h"
#include "AlphaExilemet/Tutorial/CutsceneHelpers/CinematicHandoffComponent.h"

#include "Kismet/GameplayStatics.h"
#include "Components/SphereComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Camera/PlayerCameraManager.h"
#include "Camera/CameraActor.h"
#include "Camera/CameraComponent.h"
#include "EngineUtils.h"

#include "LevelSequenceActor.h"
#include "LevelSequencePlayer.h"

#include "AlphaExilemet/AlphaExilemetCharacter.h"
#include "AlphaExilemet/BaseCamp.h"
#include "AlphaExilemet/Tools/ToolBase.h"
#include "AlphaExilemet/Core/AlphaStreamingSubsystem.h"

ATutorialDirector::ATutorialDirector()
{
	PrimaryActorTick.bCanEverTick = false;
	CinematicHandoff = CreateDefaultSubobject<UCinematicHandoffComponent>(TEXT("CinematicHandoff"));
}

void ATutorialDirector::BeginPlay()
{
	Super::BeginPlay();
	BP_RegisterWithGameMode();
}

void ATutorialDirector::InitializeTutorial()
{
	UWorld* World = GetWorld();
	if (!World) return;

	CachedPlayer = Cast<AAlphaExilemetCharacter>(
		UGameplayStatics::GetPlayerCharacter(this, 0));
	if (!CachedPlayer)
	{
		UE_LOG(LogTemp, Error, TEXT("ATutorialDirector::InitializeTutorial — Player not found."));
		return;
	}

	CachedPC = Cast<APlayerController>(CachedPlayer->GetController());
	if (!CachedPC)
	{
		UE_LOG(LogTemp, Error, TEXT("ATutorialDirector::InitializeTutorial — PC not found."));
		return;
	}

	if (SkullRef) SkullRef->SetDirector(this);

	BP_HideHUD();
	CachedPlayer->bIsSurvivalActive = false;

	if (!IntroSequenceRef)
	{
		UE_LOG(LogTemp, Error,
			TEXT("ATutorialDirector::InitializeTutorial — IntroSequenceRef null."));
		return;
	}

	IntroSequencePlayer = IntroSequenceRef->GetSequencePlayer();
	if (!IntroSequencePlayer)
	{
		UE_LOG(LogTemp, Error, TEXT("ATutorialDirector — IntroSequenceRef has no SequencePlayer."));
		return;
	}

	HidePlayerForCutscene();
	LockPlayerInputFull();

	IntroSequencePlayer->OnStop.AddUniqueDynamic(
		this, &ATutorialDirector::OnIntroSequenceFinished);

	if (TutorialCineCamRef)
		CachedPC->SetViewTargetWithBlend(TutorialCineCamRef, 0.f);

	IntroSequencePlayer->Play();
	UE_LOG(LogTemp, Log, TEXT("ATutorialDirector: Intro cutscene playing."));
}

void ATutorialDirector::HidePlayerForCutscene()
{
	if (!CachedPlayer) return;
	CachedPlayer->SetActorHiddenInGame(true);
	if (USkeletalMeshComponent* Mesh = CachedPlayer->GetMesh())
		Mesh->SetVisibility(false, true);
}

// ─────────────────────────────────────────────────────────────────────────────
// FindProxyMeshInSequence
//
// Since the Spawnable SK has "When Finished = Keep State" it stays alive in
// the world after OnStop fires. We iterate ALL actors in the world and pick
// the one whose name contains ProxyMeshNameHint AND has a SkeletalMeshComponent.
// No LevelSequence API, no Actor Tags, no Sequencer Binding Tags required.
// ─────────────────────────────────────────────────────────────────────────────

USkeletalMeshComponent* ATutorialDirector::FindProxyMeshInSequence(
	AActor*& OutProxyActor) const
{
	OutProxyActor = nullptr;
	UWorld* World = GetWorld();
	if (!World) return nullptr;

	const bool bUseHint = !ProxyMeshNameHint.IsEmpty();

	// First pass: name hint + has SKM.
	for (TActorIterator<AActor> It(World); It; ++It)
	{
		AActor* Actor = *It;
		if (!Actor || !IsValid(Actor) || Actor == CachedPlayer) continue;
		if (Actor->IsA<AAlphaExilemetCharacter>())              continue;

		if (bUseHint && !Actor->GetName().Contains(ProxyMeshNameHint, ESearchCase::IgnoreCase))
			continue;

		USkeletalMeshComponent* SkMesh = Actor->FindComponentByClass<USkeletalMeshComponent>();
		if (!SkMesh) continue;

		OutProxyActor = Actor;
		UE_LOG(LogTemp, Log, TEXT("ATutorialDirector: Found proxy '%s'."), *Actor->GetName());
		return SkMesh;
	}

	// Second pass: any SK actor (fallback when name hint is too strict).
	if (bUseHint)
	{
		UE_LOG(LogTemp, Warning,
			TEXT("ATutorialDirector: No actor matched hint '%s'. Trying first SK actor."),
			*ProxyMeshNameHint);

		for (TActorIterator<AActor> It(World); It; ++It)
		{
			AActor* Actor = *It;
			if (!Actor || !IsValid(Actor) || Actor == CachedPlayer) continue;
			if (Actor->IsA<AAlphaExilemetCharacter>())              continue;

			USkeletalMeshComponent* SkMesh = Actor->FindComponentByClass<USkeletalMeshComponent>();
			if (!SkMesh) continue;

			OutProxyActor = Actor;
			UE_LOG(LogTemp, Log, TEXT("ATutorialDirector: Fallback proxy = '%s'."), *Actor->GetName());
			return SkMesh;
		}
	}

	UE_LOG(LogTemp, Warning,
		TEXT("ATutorialDirector: No proxy SK found. "
		     "Verify SKM_Manny track has 'When Finished = Keep State' in Sequencer."));
	return nullptr;
}

AActor* ATutorialDirector::FindShipInSequence() const
{
	if (CutsceneShipNameHint.IsEmpty()) return nullptr;
	UWorld* World = GetWorld();
	if (!World) return nullptr;

	for (TActorIterator<AActor> It(World); It; ++It)
	{
		AActor* Actor = *It;
		if (!Actor || !IsValid(Actor)) continue;
		if (Actor->GetName().Contains(CutsceneShipNameHint, ESearchCase::IgnoreCase))
		{
			UE_LOG(LogTemp, Log, TEXT("ATutorialDirector: Found ship '%s'."), *Actor->GetName());
			return Actor;
		}
	}
	return nullptr;
}

void ATutorialDirector::OnIntroSequenceFinished()
{
	UE_LOG(LogTemp, Log, TEXT("ATutorialDirector: OnStop fired. Deferring by one tick."));
	GetWorldTimerManager().SetTimerForNextTick(
		this, &ATutorialDirector::OnIntroSequenceFinishedDeferred);
}

void ATutorialDirector::OnIntroSequenceFinishedDeferred()
{
	if (!CachedPlayer || !CachedPC) return;

	// A. Ghost camera start from last active CineCamera.
	FVector  GhostStartLocation;
	FRotator GhostStartRotation;

	if (AActor* VT = CachedPC->GetViewTarget())
	{
		GhostStartLocation = VT->GetActorLocation();
		GhostStartRotation = VT->GetActorRotation();
		UE_LOG(LogTemp, Log, TEXT("ATutorialDirector: Ghost start = '%s' at %s."),
			*VT->GetName(), *GhostStartLocation.ToString());
	}
	else
	{
		GhostStartLocation = CachedPlayer->GetActorLocation();
		GhostStartRotation = CachedPlayer->GetActorRotation();
		UE_LOG(LogTemp, Warning, TEXT("ATutorialDirector: No ViewTarget. Ghost at player."));
	}

	// B. Find proxy.
	AActor*                 ProxyActor = nullptr;
	USkeletalMeshComponent* ProxyMesh  = FindProxyMeshInSequence(ProxyActor);

	// C. Camera travel target = head bone.
	FVector CameraTarget;
	if (ProxyMesh && ProxyMesh->DoesSocketExist(HeadBoneName))
	{
		CameraTarget = ProxyMesh->GetBoneLocation(HeadBoneName);
		UE_LOG(LogTemp, Log, TEXT("ATutorialDirector: Camera target bone '%s' at %s."),
			*HeadBoneName.ToString(), *CameraTarget.ToString());
	}
	else if (ProxyActor)
	{
		CameraTarget = ProxyActor->GetActorLocation() + FVector(0.f, 0.f, CachedPlayer->BaseEyeHeight);
		UE_LOG(LogTemp, Warning, TEXT("ATutorialDirector: Bone '%s' not found. Using actor+EyeHeight."),
			*HeadBoneName.ToString());
	}
	else
	{
		CameraTarget = GhostStartLocation;
	}

	// D. Player spawn = root bone.
	FTransform PlayerSpawnTransform;
	if (ProxyMesh && ProxyMesh->DoesSocketExist(RootBoneName))
	{
		FVector  RootLoc  = ProxyMesh->GetBoneLocation(RootBoneName);
		FRotator SpawnRot = FRotator(0.f, ProxyActor->GetActorRotation().Yaw, 0.f);
		PlayerSpawnTransform = FTransform(SpawnRot, RootLoc, FVector::OneVector);
		UE_LOG(LogTemp, Log, TEXT("ATutorialDirector: Player spawn at root '%s' = %s."),
			*RootBoneName.ToString(), *RootLoc.ToString());
	}
	else if (ProxyActor)
	{
		FRotator SpawnRot = FRotator(0.f, ProxyActor->GetActorRotation().Yaw, 0.f);
		PlayerSpawnTransform = FTransform(SpawnRot, ProxyActor->GetActorLocation());
	}
	else
	{
		PlayerSpawnTransform = CachedPlayer->GetActorTransform();
	}

	// E. Reveal persistent ship.
	if (PersistentShipActor)
	{
		if (AActor* SeqShip = FindShipInSequence())
			PersistentShipActor->SetActorTransform(SeqShip->GetActorTransform(),
				false, nullptr, ETeleportType::TeleportPhysics);

		PersistentShipActor->SetActorHiddenInGame(false);
		PersistentShipActor->SetActorEnableCollision(true);
	}

	// F. Begin handoff.
	CinematicHandoff->OnHandoffComplete.AddUniqueDynamic(
		this, &ATutorialDirector::OnCinematicHandoffComplete);

	CinematicHandoff->BeginHandoff(
		CachedPlayer, CachedPC,
		GhostStartLocation, GhostStartRotation,
		CameraTarget, PlayerSpawnTransform,
		ProxyActor);
}

void ATutorialDirector::OnCinematicHandoffComplete()
{
	if (!CachedPlayer || !CachedPC) return;

	RestorePlayerMoveInput();

	FInputModeGameOnly GameMode;
	CachedPC->SetInputMode(GameMode);
	CachedPC->bShowMouseCursor = false;

	CraterStartTransform = CachedPlayer->GetActorTransform();

	if (AActor* BC = FindBaseCamp())
	{
		if (ABaseCamp* BaseCamp = Cast<ABaseCamp>(BC))
		{
			if (BaseCamp->OxygenSphere)
				BaseCamp->OxygenSphere->OnComponentEndOverlap.AddUniqueDynamic(
					this, &ATutorialDirector::OnOxygenSphereEndOverlap);
		}
	}

	if (TutorialPickaxeClass && !SpawnedTutorialPickaxe)
	{
		FActorSpawnParameters Params;
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		SpawnedTutorialPickaxe = GetWorld()->SpawnActor<AToolBase>(
			TutorialPickaxeClass,
			CachedPlayer->GetActorLocation(),
			CachedPlayer->GetActorRotation(), Params);
		if (SpawnedTutorialPickaxe)
		{
			SpawnedTutorialPickaxe->SetActorEnableCollision(false);
			CachedPlayer->AddToolToInventory(SpawnedTutorialPickaxe);
			CachedPlayer->StartWieldTool(0);
		}
	}

	UE_LOG(LogTemp, Log, TEXT("ATutorialDirector: Handoff complete. Crater=%s."),
		*CraterStartTransform.GetLocation().ToString());

	BP_OnIntroFinished();
}

void ATutorialDirector::OnOxygenSphereEndOverlap(
	UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	if (!Cast<AAlphaExilemetCharacter>(OtherActor)) return;
	if (bSkullInteractionActive) return;
	if (CraterStartTransform.GetLocation().IsZero()) return;
	ExecuteTeleportToCrater();
}

void ATutorialDirector::ExecuteTeleportToCrater()
{
	if (!CachedPlayer || !CachedPC) return;
	SuppressPlayerMoveInput();
	if (APlayerCameraManager* Cam = CachedPC->PlayerCameraManager)
		Cam->StartCameraFade(0.f, 1.f, 1.f, FLinearColor::Black, false, true);
	GetWorldTimerManager().SetTimer(TeleportFadeOutHandle,
		this, &ATutorialDirector::OnTeleportReadyToMove, 1.f, false);
}

void ATutorialDirector::OnTeleportReadyToMove()
{
	if (!CachedPlayer || !CachedPC) return;
	CachedPlayer->SetActorLocationAndRotation(
		CraterStartTransform.GetLocation(),
		CraterStartTransform.GetRotation().Rotator(),
		false, nullptr, ETeleportType::TeleportPhysics);
	CachedPC->SetControlRotation(CraterStartTransform.GetRotation().Rotator());
	if (APlayerCameraManager* Cam = CachedPC->PlayerCameraManager)
		Cam->StartCameraFade(1.f, 0.f, 1.f, FLinearColor::Black, false, false);
	GetWorldTimerManager().SetTimer(TeleportFadeInHandle,
		this, &ATutorialDirector::OnTeleportComplete, 1.f, false);
}

void ATutorialDirector::OnTeleportComplete() { RestorePlayerMoveInput(); }

void ATutorialDirector::ClearTutorialPickaxe()
{
	if (!CachedPlayer || !SpawnedTutorialPickaxe) return;
	CachedPlayer->OwnedTools.Remove(SpawnedTutorialPickaxe);
	if (CachedPlayer->CurrentTool == SpawnedTutorialPickaxe)
	{
		CachedPlayer->CurrentTool     = nullptr;
		CachedPlayer->ActiveToolIndex = -1;
	}
	SpawnedTutorialPickaxe->Destroy();
	SpawnedTutorialPickaxe = nullptr;
	CachedPlayer->OnInventoryUpdated.Broadcast();
	CachedPlayer->OnToolWielded.Broadcast(-1);
}

void ATutorialDirector::OnSkullInteracted()
{
	if (bSkullInteractionActive) return;
	bSkullInteractionActive = true;
	ClearTutorialPickaxe();
	if (SkullRef) SkullRef->StartFlicker();
	BP_PlayMagicSpellSound();
	GetWorldTimerManager().SetTimer(SpellDurationHandle,
		this, &ATutorialDirector::OnSpellDurationComplete, 5.5f, false);
}

void ATutorialDirector::OnSpellDurationComplete()
{
	if (SkullRef) SkullRef->StopAndReset();
	GetWorldTimerManager().SetTimer(SkullPauseHandle,
		this, &ATutorialDirector::OnSkullPausedBeforeBlack, 0.5f, false);
}

void ATutorialDirector::OnSkullPausedBeforeBlack()
{
	LockPlayerInputFull();
	BP_ShowInstantBlack();
	GetWorldTimerManager().SetTimer(PostBlackSoundHandle,
		this, &ATutorialDirector::OnPostBlackDelay, 1.f, false);
}

void ATutorialDirector::OnPostBlackDelay()
{
	BP_PlayExplosionSequence();
	GetWorldTimerManager().SetTimer(LevelSwapHandle,
		this, &ATutorialDirector::OnLevelSwapReady, 3.f, false);
}

void ATutorialDirector::OnLevelSwapReady()
{
	if (UGameInstance* GI = GetGameInstance())
		if (UAlphaStreamingSubsystem* SS = GI->GetSubsystem<UAlphaStreamingSubsystem>())
			SS->HandleTutorialCompletion();
}

void ATutorialDirector::SuppressPlayerMoveInput()
{
	if (CachedPC) CachedPC->SetIgnoreMoveInput(true);
}

void ATutorialDirector::RestorePlayerMoveInput()
{
	if (!CachedPC) return;
	CachedPC->SetIgnoreMoveInput(false);
	if (CachedPlayer)
		if (UCharacterMovementComponent* Mv = CachedPlayer->GetCharacterMovement())
			if (Mv->MovementMode == MOVE_None)
				Mv->SetMovementMode(MOVE_Walking);
}

void ATutorialDirector::LockPlayerInputFull()
{
	if (!CachedPlayer || !CachedPC) return;
	if (UCharacterMovementComponent* Mv = CachedPlayer->GetCharacterMovement())
		Mv->DisableMovement();
	CachedPC->SetIgnoreMoveInput(true);
	CachedPC->SetInputMode(FInputModeUIOnly());
}

AActor* ATutorialDirector::FindBaseCamp() const
{
	return UGameplayStatics::GetActorOfClass(GetWorld(), ABaseCamp::StaticClass());
}
