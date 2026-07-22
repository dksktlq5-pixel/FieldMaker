#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Weapon/FMWeaponTypes.h"
#include "FMInventoryComponent.generated.h"

class AFMWeaponBase;

UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class FIELDMAKER_API UFMInventoryComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UFMInventoryComponent();

	virtual void GetLifetimeReplicatedProps(
		TArray<FLifetimeProperty>& OutLifetimeProps
	) const override;

	UFUNCTION(BlueprintCallable, Category = "FM|Inventory")
	bool AddWeapon(AFMWeaponBase* NewWeapon);

	UFUNCTION(BlueprintCallable, Category = "FM|Inventory")
	AFMWeaponBase* RemoveWeapon(EFMWeaponSlot WeaponSlot);

	UFUNCTION(BlueprintPure, Category = "FM|Inventory")
	AFMWeaponBase* GetWeaponBySlot(EFMWeaponSlot WeaponSlot) const;

protected:
	virtual void BeginPlay() override;

	void SpawnDefaultWeapons();

	AFMWeaponBase* SpawnDefaultWeapon(
		TSubclassOf<AFMWeaponBase> WeaponClass
	);

	/* =========================================================
	 * Default Weapons
	 * ========================================================= */

	UPROPERTY(
		EditDefaultsOnly,
		BlueprintReadOnly,
		Category = "FM|Inventory|Default"
	)
	TSubclassOf<AFMWeaponBase> DefaultPrimaryWeaponClass;

	UPROPERTY(
		EditDefaultsOnly,
		BlueprintReadOnly,
		Category = "FM|Inventory|Default"
	)
	TSubclassOf<AFMWeaponBase> DefaultSecondaryWeaponClass;

	UPROPERTY(
		EditDefaultsOnly,
		BlueprintReadOnly,
		Category = "FM|Inventory|Default"
	)
	TSubclassOf<AFMWeaponBase> DefaultMeleeWeaponClass;

	UPROPERTY(
		EditDefaultsOnly,
		BlueprintReadOnly,
		Category = "FM|Inventory|Default"
	)
	TSubclassOf<AFMWeaponBase> DefaultThrowableWeaponClass;

	/* =========================================================
	 * Weapon Slots
	 * ========================================================= */

	UPROPERTY(
		Replicated,
		VisibleInstanceOnly,
		BlueprintReadOnly,
		Category = "FM|Inventory"
	)
	TObjectPtr<AFMWeaponBase> PrimaryWeapon;

	UPROPERTY(
		Replicated,
		VisibleInstanceOnly,
		BlueprintReadOnly,
		Category = "FM|Inventory"
	)
	TObjectPtr<AFMWeaponBase> SecondaryWeapon;

	UPROPERTY(
		Replicated,
		VisibleInstanceOnly,
		BlueprintReadOnly,
		Category = "FM|Inventory"
	)
	TObjectPtr<AFMWeaponBase> MeleeWeapon;

	UPROPERTY(
		Replicated,
		VisibleInstanceOnly,
		BlueprintReadOnly,
		Category = "FM|Inventory"
	)
	TObjectPtr<AFMWeaponBase> ThrowableWeapon;
};