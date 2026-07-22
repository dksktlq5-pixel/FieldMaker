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

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FName WeaponID = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FText DisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	EFMWeaponSlot WeaponSlot = EFMWeaponSlot::Primary;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TSubclassOf<AFMWeaponBase> WeaponClass;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	int32 MagazineSize = 30;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	int32 MaxReserveAmmo = 90;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	float Damage = 30.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	float FireInterval = 0.1f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	float Range = 10000.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	EFMFireMode FireMode = EFMFireMode::FullAuto;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FName FirstPersonSocket = TEXT("Grip_M4");

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FName ThirdPersonSocket = TEXT("WeaponSocket_R");

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TObjectPtr<UTexture2D> Icon = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	int32 Price = 2700;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TObjectPtr<UAnimMontage> EquipMontage = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TObjectPtr<UAnimMontage> FireMontage = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TObjectPtr<UAnimMontage> ReloadMontage = nullptr;
};