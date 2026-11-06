#pragma once
#include "Engine/DataTable.h"
#include "FMWeaponVisualData.generated.h"

// Cosmetic presentation only. Combat numbers and hit detection stay in FMWeaponRow/Character.
USTRUCT(BlueprintType)
struct FFMWeaponVisualRow : public FTableRowBase
{
 GENERATED_BODY()
 UPROPERTY(EditAnywhere,BlueprintReadWrite) TSoftObjectPtr<class UStaticMesh> Mesh;
 UPROPERTY(EditAnywhere,BlueprintReadWrite) TSoftObjectPtr<UStaticMesh> MagazineMesh;
 UPROPERTY(EditAnywhere,BlueprintReadWrite) TSoftObjectPtr<UStaticMesh> AccessoryMesh;
 UPROPERTY(EditAnywhere,BlueprintReadWrite) FVector MagazineLocation = FVector(11,0,-3);
 UPROPERTY(EditAnywhere,BlueprintReadWrite) FRotator MagazineRotation = FRotator::ZeroRotator;
 UPROPERTY(EditAnywhere,BlueprintReadWrite) FVector AccessoryLocation = FVector::ZeroVector;
 UPROPERTY(EditAnywhere,BlueprintReadWrite) FVector AccessoryScale = FVector::OneVector;
 UPROPERTY(EditAnywhere,BlueprintReadWrite) FVector Muzzle = FVector(62,0,9);
 UPROPERTY(EditAnywhere,BlueprintReadWrite) FVector RightPalm = FVector(-1,.7f,-.4f);
 UPROPERTY(EditAnywhere,BlueprintReadWrite) FVector LeftPalm = FVector(21,-1.1f,4);
 UPROPERTY(EditAnywhere,BlueprintReadWrite) FVector LeftBoneX = FVector(1,.12f,.19f);
 UPROPERTY(EditAnywhere,BlueprintReadWrite) FVector LeftBoneY = FVector(0,-.4f,-.91f);
 UPROPERTY(EditAnywhere,BlueprintReadWrite) FVector ArmOffset = FVector::ZeroVector;
 UPROPERTY(EditAnywhere,BlueprintReadWrite) FVector ADSArmOffset = FVector::ZeroVector;
 UPROPERTY(EditAnywhere,BlueprintReadWrite) FVector ViewOffset = FVector::ZeroVector;
 UPROPERTY(EditAnywhere,BlueprintReadWrite) FVector ADSOffset = FVector::ZeroVector;
 UPROPERTY(EditAnywhere,BlueprintReadWrite) FText ModelLabel;
 UPROPERTY(EditAnywhere,BlueprintReadWrite) bool bRepresentative = true;
};
