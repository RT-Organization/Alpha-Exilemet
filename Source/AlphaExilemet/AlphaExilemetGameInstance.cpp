#include "AlphaExilemetGameInstance.h"
#include "Kismet/GameplayStatics.h"
#include "Camera/CameraComponent.h"

void UAlphaExilemetGameInstance::SavePlayerData()
{
	PlayerRef = Cast<AAlphaExilemetCharacter>(UGameplayStatics::GetPlayerCharacter(this, 0));
	if (!PlayerRef || !LocalSaveRef) return;

	LocalSaveRef->SavedHealth = PlayerRef->Health;
	LocalSaveRef->SavedOxygen = PlayerRef->Oxygen;
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

	PlayerRef->Health = LocalSaveRef->SavedHealth;
	PlayerRef->Oxygen = LocalSaveRef->SavedOxygen;
	PlayerRef->Currency = LocalSaveRef->SavedCurrency;
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
	
	// Calls the function in BaseCamp to update the Oxygen bubble radius instantly
	BaseRef->ApplyShipUpgrades(); 
}