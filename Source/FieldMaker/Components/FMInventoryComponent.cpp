#include "FMInventoryComponent.h"

#include "GameFramework/Pawn.h"
#include "Net/UnrealNetwork.h"
#include "Weapon/FMWeaponBase.h"

UFMInventoryComponent::UFMInventoryComponent()
{
	PrimaryComponentTick.bCanEverTick = false;

	SetIsReplicatedByDefault(true);
}

void UFMInventoryComponent::BeginPlay()
{
	Super::BeginPlay();

	if (GetOwner() && GetOwner()->HasAuthority())
	{
		SpawnDefaultWeapons();
	}
}

void UFMInventoryComponent::SpawnDefaultWeapons()
{
	if (AFMWeaponBase* Weapon =
		SpawnDefaultWeapon(DefaultPrimaryWeaponClass))
	{
		AddWeapon(Weapon);
	}

	if (AFMWeaponBase* Weapon =
		SpawnDefaultWeapon(DefaultSecondaryWeaponClass))
	{
		AddWeapon(Weapon);
	}

	if (AFMWeaponBase* Weapon =
		SpawnDefaultWeapon(DefaultMeleeWeaponClass))
	{
		AddWeapon(Weapon);
	}

	if (AFMWeaponBase* Weapon =
		SpawnDefaultWeapon(DefaultThrowableWeaponClass))
	{
		AddWeapon(Weapon);
	}
}

AFMWeaponBase* UFMInventoryComponent::SpawnDefaultWeapon(
	TSubclassOf<AFMWeaponBase> WeaponClass
)
{
	if (!WeaponClass || !GetWorld())
	{
		return nullptr;
	}

	FActorSpawnParameters SpawnParameters;
	SpawnParameters.Owner = GetOwner();
	SpawnParameters.SpawnCollisionHandlingOverride =
		ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	if (APawn* OwnerPawn = Cast<APawn>(GetOwner()))
	{
		SpawnParameters.Instigator = OwnerPawn;
	}

	AFMWeaponBase* NewWeapon =
		GetWorld()->SpawnActor<AFMWeaponBase>(
			WeaponClass,
			FTransform::Identity,
			SpawnParameters
		);

	if (NewWeapon)
	{
		NewWeapon->SetActorHiddenInGame(true);
		NewWeapon->SetActorEnableCollision(false);
	}

	return NewWeapon;
}

bool UFMInventoryComponent::AddWeapon(
	AFMWeaponBase* NewWeapon
)
{
	if (!NewWeapon ||
		!GetOwner() ||
		!GetOwner()->HasAuthority())
	{
		return false;
	}

	AFMWeaponBase* ExistingWeapon =
		GetWeaponBySlot(NewWeapon->GetWeaponSlot());

	if (ExistingWeapon && ExistingWeapon != NewWeapon)
	{
		ExistingWeapon->Destroy();
	}

	switch (NewWeapon->GetWeaponSlot())
	{
	case EFMWeaponSlot::Primary:
		PrimaryWeapon = NewWeapon;
		break;

	case EFMWeaponSlot::Secondary:
		SecondaryWeapon = NewWeapon;
		break;

	case EFMWeaponSlot::Melee:
		MeleeWeapon = NewWeapon;
		break;

	case EFMWeaponSlot::Throwable:
		ThrowableWeapon = NewWeapon;
		break;

	default:
		return false;
	}

	NewWeapon->SetOwner(GetOwner());
	NewWeapon->SetActorEnableCollision(false);
	NewWeapon->SetActorHiddenInGame(true);

	return true;
}

AFMWeaponBase* UFMInventoryComponent::RemoveWeapon(
	EFMWeaponSlot WeaponSlot
)
{
	if (!GetOwner() || !GetOwner()->HasAuthority())
	{
		return nullptr;
	}

	AFMWeaponBase* RemovedWeapon =
		GetWeaponBySlot(WeaponSlot);

	switch (WeaponSlot)
	{
	case EFMWeaponSlot::Primary:
		PrimaryWeapon = nullptr;
		break;

	case EFMWeaponSlot::Secondary:
		SecondaryWeapon = nullptr;
		break;

	case EFMWeaponSlot::Melee:
		MeleeWeapon = nullptr;
		break;

	case EFMWeaponSlot::Throwable:
		ThrowableWeapon = nullptr;
		break;

	default:
		return nullptr;
	}

	return RemovedWeapon;
}

AFMWeaponBase* UFMInventoryComponent::GetWeaponBySlot(
	EFMWeaponSlot WeaponSlot
) const
{
	switch (WeaponSlot)
	{
	case EFMWeaponSlot::Primary:
		return PrimaryWeapon;

	case EFMWeaponSlot::Secondary:
		return SecondaryWeapon;

	case EFMWeaponSlot::Melee:
		return MeleeWeapon;

	case EFMWeaponSlot::Throwable:
		return ThrowableWeapon;

	default:
		return nullptr;
	}
}

void UFMInventoryComponent::GetLifetimeReplicatedProps(
	TArray<FLifetimeProperty>& OutLifetimeProps
) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(
		UFMInventoryComponent,
		PrimaryWeapon
	);

	DOREPLIFETIME(
		UFMInventoryComponent,
		SecondaryWeapon
	);

	DOREPLIFETIME(
		UFMInventoryComponent,
		MeleeWeapon
	);

	DOREPLIFETIME(
		UFMInventoryComponent,
		ThrowableWeapon
	);
}