#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Weapon/FMWeaponTypes.h"
#include "FMEquipmentComponent.generated.h"

class AFMPlayerCharacter;
class AFMWeaponBase;
class UFMInventoryComponent;

UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class FIELDMAKER_API UFMEquipmentComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UFMEquipmentComponent();

	virtual void GetLifetimeReplicatedProps(
		TArray<FLifetimeProperty>& OutLifetimeProps
	) const override;

	UFUNCTION(BlueprintCallable, Category = "FM|Equipment")
	void EquipSlot(EFMWeaponSlot NewSlot);

	UFUNCTION(BlueprintCallable, Category = "FM|Equipment")
	void UnequipCurrentWeapon();

	UFUNCTION(BlueprintPure, Category = "FM|Equipment")
	AFMWeaponBase* GetCurrentWeapon() const
	{
		return CurrentWeapon;
	}

	UFUNCTION(BlueprintPure, Category = "FM|Equipment")
	EFMWeaponSlot GetCurrentSlot() const
	{
		return CurrentSlot;
	}

protected:
	virtual void BeginPlay() override;

	UFUNCTION(Server, Reliable)
	void ServerEquipSlot(EFMWeaponSlot NewSlot);

	void EquipSlotInternal(EFMWeaponSlot NewSlot);
	void AttachCurrentWeapon();
	void HideInventoryWeapons();

	UFUNCTION()
	void OnRep_CurrentWeapon();

	UPROPERTY()
	TObjectPtr<AFMPlayerCharacter> OwnerCharacter;

	UPROPERTY()
	TObjectPtr<UFMInventoryComponent> InventoryComponent;

	UPROPERTY(
		ReplicatedUsing = OnRep_CurrentWeapon,
		VisibleInstanceOnly,
		BlueprintReadOnly,
		Category = "FM|Equipment"
	)
	TObjectPtr<AFMWeaponBase> CurrentWeapon;

	UPROPERTY(
		Replicated,
		VisibleInstanceOnly,
		BlueprintReadOnly,
		Category = "FM|Equipment"
	)
	EFMWeaponSlot CurrentSlot = EFMWeaponSlot::Melee;
};