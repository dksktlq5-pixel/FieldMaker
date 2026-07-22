#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "FMWeaponTypes.h"
#include "FMWeaponDefinition.generated.h"

class AFMWeaponBase;
class UTexture2D;
class UAnimMontage;

USTRUCT(BlueprintType)
struct FFMWeaponDefinition : public FTableRowBase
{
	GENERATED_BODY()

public:
	/* =========================================================
	 * Identity
	 * ========================================================= */

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon")
	FName WeaponID = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon")
	FText DisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon")
	EFMWeaponSlot WeaponSlot = EFMWeaponSlot::Primary;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon")
	TSubclassOf<AFMWeaponBase> WeaponClass;

	/* =========================================================
	 * Fire
	 * ========================================================= */

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon|Fire")
	float Damage = 30.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon|Fire")
	float FireRange = 10000.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon|Fire")
	float FireInterval = 0.1f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon|Fire")
	EFMFireMode FireMode = EFMFireMode::FullAuto;

	/* =========================================================
	 * Ammo
	 * ========================================================= */

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon|Ammo")
	int32 MagazineSize = 30;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon|Ammo")
	int32 InitialReserveAmmo = 90;

	/* =========================================================
	 * Reload
	 * ========================================================= */

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon|Reload")
	float ReloadDuration = 2.0f;

	/* =========================================================
	 * Attachment
	 * ========================================================= */

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon|Attachment")
	FName FirstPersonSocket = TEXT("Grip_M4");

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon|Attachment")
	FName ThirdPersonSocket = TEXT("WeaponSocket_R");

	/* =========================================================
	 * Shop / UI
	 * ========================================================= */

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon|Shop")
	int32 Price = 2700;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon|UI")
	TObjectPtr<UTexture2D> Icon = nullptr;

	/* =========================================================
	 * Animation
	 * ========================================================= */

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon|Animation")
	TObjectPtr<UAnimMontage> EquipMontage = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon|Animation")
	TObjectPtr<UAnimMontage> FireMontage = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon|Animation")
	TObjectPtr<UAnimMontage> ReloadMontage = nullptr;
};