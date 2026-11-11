#pragma once

#include "Engine/DataTable.h"
#include "GameFramework/DamageType.h"
#include "FMCombatData.generated.h"

namespace FMCatalog
{
inline constexpr TCHAR Weapons[] = TEXT("/Game/FieldMaker/Data/DT_FMWeapons.DT_FMWeapons");
inline constexpr TCHAR Visuals[] = TEXT("/Game/FieldMaker/Data/DT_FMWeaponVisuals.DT_FMWeaponVisuals");
}

UENUM(BlueprintType)
enum class EFMHitZone : uint8 { Head, Body, Limb };

UENUM(BlueprintType)
enum class EFMWeaponCategory : uint8 { Rifle, Pistol, SMG, Sniper, Shotgun, MachineGun, Melee, Grenade };

USTRUCT(BlueprintType)
struct FFMWeaponRow : public FTableRowBase
{
	GENERATED_BODY()
	UPROPERTY(EditAnywhere, BlueprintReadWrite) FText DisplayName;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) EFMWeaponCategory Category = EFMWeaponCategory::Rifle;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) int32 Price = 0;
 UPROPERTY(EditAnywhere, BlueprintReadWrite) bool bInCatalog = true;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) int32 PelletCount = 1;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) float SpreadDegrees = 0.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) float AimFOV = 65.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) float Damage = 28.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) float HeadMultiplier = 2.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) float LimbMultiplier = 0.75f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) float Range = 10000.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) float FalloffStart = 10000.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) float MinimumDamageFraction = 1.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) float AttackInterval = 0.12f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) float ReloadTime = 1.55f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) float EquipTime = 0.28f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) int32 MagazineSize = 30;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) int32 ReserveAmmo = 90;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) bool bAutomatic = true;
};

USTRUCT(BlueprintType)
struct FFMArmorRow : public FTableRowBase
{
	GENERATED_BODY()
	UPROPERTY(EditAnywhere, BlueprintReadWrite) FText DisplayName;
 UPROPERTY(EditAnywhere, BlueprintReadWrite) int32 Price = 650;
 UPROPERTY(EditAnywhere, BlueprintReadWrite) float MaximumArmor = 50.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) float AbsorptionFraction = 0.5f;
};

// Only point damage with this server-selected type is eligible for bullet armor.
UCLASS()
class FIELDMAKER_API UFMFirearmDamageType : public UDamageType
{
	GENERATED_BODY()
};
