#pragma once

#include "CoreMinimal.h"
#include "FMWeaponTypes.generated.h"

UENUM(BlueprintType)
enum class EFMWeaponSlot : uint8
{
	None        UMETA(DisplayName = "None"),
	Primary     UMETA(DisplayName = "Primary"),
	Secondary   UMETA(DisplayName = "Secondary")
};

UENUM(BlueprintType)
enum class EFMWeaponState : uint8
{
	Idle        UMETA(DisplayName = "Idle"),
	Equipping   UMETA(DisplayName = "Equipping"),
	Firing      UMETA(DisplayName = "Firing"),
	Reloading   UMETA(DisplayName = "Reloading")
};

UENUM(BlueprintType)
enum class EFMFireMode : uint8
{
	SemiAuto    UMETA(DisplayName = "Semi Auto"),
	FullAuto    UMETA(DisplayName = "Full Auto")
};