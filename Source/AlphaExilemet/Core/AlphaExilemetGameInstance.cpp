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
	LocalSaveRef = Cast<UAlphaExilemetSaveGame>(UGameplayStatics::CreateSaveGameObject(UAlphaExilemetSaveGame::StaticClass()));
	
	if (LocalSaveRef)
	{
		// Force the starting level so the system knows where to drop the player
		LocalSaveRef->CurrentLevelName = FName("Tutorial");
		
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
}

void UAlphaExilemetGameInstance::SetupPlayerData()
{
	PlayerRef = Cast<AAlphaExilemetCharacter>(UGameplayStatics::GetPlayerCharacter(this, 0));
	if (!PlayerRef || !LocalSaveRef) return;

	PlayerRef->SetActorTransform(LocalSaveRef->PlayerLocation, false, nullptr, ETeleportType::TeleportPhysics);
	if (APlayerController* PC = Cast<APlayerController>(PlayerRef->GetController()))
	{
		PC->SetControlRotation(LocalSaveRef->PlayerCamera.Rotator());
	}

	PlayerRef->Health                = LocalSaveRef->SavedHealth;
	PlayerRef->Oxygen                = LocalSaveRef->SavedOxygen;
	PlayerRef->Currency              = LocalSaveRef->SavedCurrency;
	PlayerRef->SystemUpgradeLevels   = LocalSaveRef->SavedUpgradeLevels;

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
	// Create the manager if it doesn't exist yet
	if (!ProgressionManager)
	{
		ProgressionManager = NewObject<UUpgradeProgressionManager>(this);
	}

	// Gather current levels from the already-loaded player and ship references.
	// This works both for fresh games (all levels = 0) and loaded games.
	TMap<EPlayerStat, int32> CharacterLevels;
	TMap<FName, int32>       ToolStatLevels;
	TMap<EShipSystem, int32> ShipLevels;

	if (PlayerRef)
	{
		CharacterLevels = PlayerRef->SystemUpgradeLevels;

		// Merge tool upgrade levels from all owned tools.
		// This correctly picks up Pickaxe_Strength, Vacuum_Speed, Rod_AbsSpeed, etc.
		for (AToolBase* Tool : PlayerRef->OwnedTools)
		{
			if (Tool)
			{
				// TMap::Append keeps existing keys if they collide — safe because
				// each StatID should only appear on one tool type.
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
