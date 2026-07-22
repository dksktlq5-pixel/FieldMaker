#include "FMEquipmentComponent.h"

#include "Character/FMPlayerCharacter.h"
#include "FMInventoryComponent.h"
#include "Net/UnrealNetwork.h"
#include "Weapon/FMWeaponBase.h"

UFMEquipmentComponent::UFMEquipmentComponent()
{
	PrimaryComponentTick.bCanEverTick = false;

	SetIsReplicatedByDefault(true);
}

void UFMEquipmentComponent::BeginPlay()
{
	Super::BeginPlay();

	OwnerCharacter =
		Cast<AFMPlayerCharacter>(GetOwner());

	if (!OwnerCharacter)
	{
		UE_LOG(
			LogTemp,
			Error,
			TEXT(
				"FMEquipmentComponent: "
				"Owner가 FMPlayerCharacter가 아닙니다."
			)
		);

		return;
	}

	InventoryComponent =
		OwnerCharacter->GetInventoryComponent();
}

void UFMEquipmentComponent::EquipSlot(
	EFMWeaponSlot NewSlot
)
{
	if (!OwnerCharacter)
	{
		return;
	}

	if (!OwnerCharacter->HasAuthority())
	{
		ServerEquipSlot(NewSlot);
		return;
	}

	EquipSlotInternal(NewSlot);
}

void UFMEquipmentComponent::ServerEquipSlot_Implementation(
	EFMWeaponSlot NewSlot
)
{
	EquipSlotInternal(NewSlot);
}

void UFMEquipmentComponent::EquipSlotInternal(
	EFMWeaponSlot NewSlot
)
{
	if (!OwnerCharacter || !InventoryComponent)
	{
		return;
	}

	if (NewSlot == EFMWeaponSlot::None)
	{
		UnequipCurrentWeapon();
		return;
	}

	AFMWeaponBase* NewWeapon =
		InventoryComponent->GetWeaponBySlot(NewSlot);

	// 선택 슬롯은 변경하지만 무기가 없으면 빈손 상태가 된다.
	CurrentSlot = NewSlot;

	if (CurrentWeapon == NewWeapon)
	{
		return;
	}

	if (CurrentWeapon)
	{
		CurrentWeapon->DetachFromActor(
			FDetachmentTransformRules::KeepWorldTransform
		);

		CurrentWeapon->SetActorHiddenInGame(true);
	}

	CurrentWeapon = NewWeapon;

	if (!CurrentWeapon)
	{
		return;
	}

	CurrentWeapon->SetOwner(OwnerCharacter);
	CurrentWeapon->SetInstigator(OwnerCharacter);

	AttachCurrentWeapon();
}

void UFMEquipmentComponent::AttachCurrentWeapon()
{
	if (!OwnerCharacter || !CurrentWeapon)
	{
		return;
	}

	USkeletalMeshComponent* FirstPersonMesh =
		OwnerCharacter->GetFirstPersonMesh();

	if (!FirstPersonMesh)
	{
		return;
	}

	CurrentWeapon->AttachToComponent(
		FirstPersonMesh,
		FAttachmentTransformRules::SnapToTargetNotIncludingScale,
		CurrentWeapon->GetFirstPersonSocketName()
	);

	CurrentWeapon->SetActorRelativeLocation(
		FVector::ZeroVector
	);

	CurrentWeapon->SetActorRelativeRotation(
		FRotator::ZeroRotator
	);

	CurrentWeapon->SetActorEnableCollision(false);
	CurrentWeapon->SetActorHiddenInGame(false);
}

void UFMEquipmentComponent::UnequipCurrentWeapon()
{
	if (!OwnerCharacter)
	{
		return;
	}

	if (!OwnerCharacter->HasAuthority())
	{
		ServerEquipSlot(EFMWeaponSlot::None);
		return;
	}

	if (CurrentWeapon)
	{
		CurrentWeapon->DetachFromActor(
			FDetachmentTransformRules::KeepWorldTransform
		);

		CurrentWeapon->SetActorHiddenInGame(true);
	}

	CurrentWeapon = nullptr;
	CurrentSlot = EFMWeaponSlot::None;
}

void UFMEquipmentComponent::HideInventoryWeapons()
{
	if (!InventoryComponent)
	{
		return;
	}

	const EFMWeaponSlot WeaponSlots[] =
	{
		EFMWeaponSlot::Primary,
		EFMWeaponSlot::Secondary,
		EFMWeaponSlot::Melee,
		EFMWeaponSlot::Throwable
	};

	for (const EFMWeaponSlot Slot : WeaponSlots)
	{
		if (AFMWeaponBase* Weapon =
			InventoryComponent->GetWeaponBySlot(Slot))
		{
			Weapon->SetActorHiddenInGame(
				Weapon != CurrentWeapon
			);
		}
	}
}

void UFMEquipmentComponent::OnRep_CurrentWeapon()
{
	HideInventoryWeapons();
	AttachCurrentWeapon();
}

void UFMEquipmentComponent::GetLifetimeReplicatedProps(
	TArray<FLifetimeProperty>& OutLifetimeProps
) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(
		UFMEquipmentComponent,
		CurrentWeapon
	);

	DOREPLIFETIME(
		UFMEquipmentComponent,
		CurrentSlot
	);
}