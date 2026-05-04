#include "AlphaExilemetGameInstance.h"

#include "AlphaExilemet/Tools/ToolBase.h"
#include "AlphaExilemet/BaseCamp.h"
#include "Kismet/GameplayStatics.h"
#include "Camera/CameraComponent.h"

// -------------------------------------------------------------------------
// EXISTING: SAVE / LOAD
// -------------------------------------------------------------------------

bool UAlphaExilemetGameInstance::DoesSaveExist(FString SlotName)
{
	return UGameplayStatics::DoesSaveGameExist(SlotName, 0);
}

void UAlphaExilemetGameInstance::CreateNewGame(FString SlotName)
{
	CurrentSaveSlot = SlotName;

	// Create a brand new save object using your custom class
	LocalSaveRef = Cast<UAlphaExilemetSaveGame>(
		UGameplayStatics::CreateSaveGameObject(UAlphaExilemetSaveGame::StaticClass()));
	
	if (LocalSaveRef)
	{
		// Force the starting level so the system knows where to drop the player
		LocalSaveRef->CurrentLevelName = FName("Tutorial");

		// TASK A1: bHasValidTransform starts false on a fresh game.
		// SetupPlayerData() will skip the teleport call entirely.
		// It will be set true the first time SavePlayerData() runs.
		LocalSaveRef->bHasValidTransform = false;
		
		// Save it to disk immediately so it registers as an existing game
		UGameplayStatics::SaveGameToSlot(LocalSaveRef, CurrentSaveSlot, 0);
	}
}

void UAlphaExilemetGameInstance::SavePlayerData()
{
	PlayerRef = Cast<AAlphaExilemetCharacter>(UGameplayStatics::GetPlayerCharacter(this, 0));
	if (!PlayerRef || !LocalSaveRef) return;

	LocalSaveRef->SavedHealth    = PlayerRef->Health;
	LocalSaveRef->SavedOxygen   = PlayerRef->Oxygen;
	LocalSaveRef->SavedCurrency = PlayerRef->Currency;
	LocalSaveRef->SavedUpgradeLevels = PlayerRef->SystemUpgradeLevels;

	LocalSaveRef->PlayerLocation = PlayerRef->GetActorTransform();
	if (PlayerRef->FirstPersonCameraComponent)
	{
		LocalSaveRef->PlayerCamera = PlayerRef->FirstPersonCameraComponent->GetComponentTransform();
	}

	PlayerRef->SaveToolDataToSaveObject(LocalSaveRef);

	// TASK A1: Mark that we have a real position now.
	// From this point on, SetupPlayerData() will apply the saved transform.
	LocalSaveRef->bHasValidTransform = true;
}

void UAlphaExilemetGameInstance::SetupPlayerData()
{
	PlayerRef = Cast<AAlphaExilemetCharacter>(UGameplayStatics::GetPlayerCharacter(this, 0));
	if (!PlayerRef || !LocalSaveRef) return;

	// TASK A1: Only teleport if a real saved position exists.
	// On a fresh new game bHasValidTransform = false, so we skip this block
	// and let the GameMode spawn the player at PlayerStart instead.
	if (LocalSaveRef->bHasValidTransform)
	{
		PlayerRef->SetActorTransform(LocalSaveRef->PlayerLocation, false, nullptr, ETeleportType::TeleportPhysics);
		if (APlayerController* PC = Cast<APlayerController>(PlayerRef->GetController()))
		{
			PC->SetControlRotation(LocalSaveRef->PlayerCamera.Rotator());
		}
	}

	PlayerRef->Health              = LocalSaveRef->SavedHealth;
	PlayerRef->Oxygen              = LocalSaveRef->SavedOxygen;
	PlayerRef->Currency            = LocalSaveRef->SavedCurrency;
	PlayerRef->SystemUpgradeLevels = LocalSaveRef->SavedUpgradeLevels;

	PlayerRef->RecalculateStats();
	PlayerRef->LoadToolDataFromSaveObject(LocalSaveRef);
}

void UAlphaExilemetGameInstance::SaveShipData()
{
	BaseRef = Cast<ABaseCamp>(UGameplayStatics::GetActorOfClass(this, ABaseCamp::StaticClass()));
	if (!BaseRef || !LocalSaveRef) return;

	LocalSaveRef->SavedShipRepairLevels = BaseRef->ShipRepairLevels;
}

void UAlphaExilemetGameInstance::SetupShipData()
{
	BaseRef = Cast<ABaseCamp>(UGameplayStatics::GetActorOfClass(this, ABaseCamp::StaticClass()));
	if (!BaseRef || !LocalSaveRef) return;

	BaseRef->ShipRepairLevels = LocalSaveRef->SavedShipRepairLevels;
	BaseRef->ApplyShipUpgrades();
}

// -------------------------------------------------------------------------
// NEW: PROGRESSION MANAGER
// -------------------------------------------------------------------------

void UAlphaExilemetGameInstance::InitProgressionManager()
{
	if (!ProgressionManager)
	{
		ProgressionManager = NewObject<UUpgradeProgressionManager>(this);
	}

	TMap<EPlayerStat, int32> CharacterLevels;
	TMap<FName, int32>       ToolStatLevels;
	TMap<EShipSystem, int32> ShipLevels;

	if (PlayerRef)
	{
		CharacterLevels = PlayerRef->SystemUpgradeLevels;

		for (AToolBase* Tool : PlayerRef->OwnedTools)
		{
			if (Tool)
			{
				ToolStatLevels.Append(Tool->ToolUpgradeLevels);
			}
		}
	}

	if (BaseRef)
	{
		ShipLevels = BaseRef->ShipRepairLevels;
	}

	ProgressionManager->InitializeFromDataTables(
		CharacterUpgradeTable,
		ToolUpgradeTable,
		ShipRepairTable,
		CharacterLevels,
		ToolStatLevels,
		ShipLevels
	);
}

void UAlphaExilemetGameInstance::SetupProgressionData()
{
	if (!ProgressionManager || !LocalSaveRef) return;
	ProgressionManager->LoadFromSaveObject(LocalSaveRef);
}

void UAlphaExilemetGameInstance::SaveProgressionData()
{
	if (!ProgressionManager || !LocalSaveRef) return;
	ProgressionManager->SaveToSaveObject(LocalSaveRef);
}
