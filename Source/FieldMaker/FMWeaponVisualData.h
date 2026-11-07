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
 // All grip frames are in normalized weapon-local space; never alter the skeleton reference pose.
 UPROPERTY(EditAnywhere,BlueprintReadWrite) bool bUseCustomHands = false;
 UPROPERTY(EditAnywhere,BlueprintReadWrite) bool bScopeOverlay = false;
 // Elbow pole is expressed in skeletal-mesh component space (+Y forward).
 UPROPERTY(EditAnywhere,BlueprintReadWrite) FVector LeftElbowPole = FVector(22,-8,-18);
 UPROPERTY(EditAnywhere,BlueprintReadWrite) FVector RightBoneX = FVector(-1,0,.06f);
 UPROPERTY(EditAnywhere,BlueprintReadWrite) FVector RightBoneY = FVector(0,-1,0);
 UPROPERTY(EditAnywhere,BlueprintReadWrite) FVector2D RightFingerBend = FVector2D(74,130);
 UPROPERTY(EditAnywhere,BlueprintReadWrite) FVector2D RightIndexIdle = FVector2D(8,28);
 UPROPERTY(EditAnywhere,BlueprintReadWrite) FVector2D RightIndexFire = FVector2D(28,67);
 UPROPERTY(EditAnywhere,BlueprintReadWrite) float RightThumb = .8f;
 UPROPERTY(EditAnywhere,BlueprintReadWrite) float RightThumbAlong = .85f;
 UPROPERTY(EditAnywhere,BlueprintReadWrite) FVector2D LeftFingerBend = FVector2D(51,105);
 UPROPERTY(EditAnywhere,BlueprintReadWrite) FVector2D LeftIndexBend = FVector2D(46,96);
 UPROPERTY(EditAnywhere,BlueprintReadWrite) float LeftThumb = .54f;
 UPROPERTY(EditAnywhere,BlueprintReadWrite) float LeftThumbAlong = 0.f;
 UPROPERTY(EditAnywhere,BlueprintReadWrite) FVector MagazineHand = FVector(-2.6f,-1.9f,-.9f);
 UPROPERTY(EditAnywhere,BlueprintReadWrite) FVector ReloadPalm = FVector(7,-2,-4);
 UPROPERTY(EditAnywhere,BlueprintReadWrite) FVector ArmOffset = FVector::ZeroVector;
 UPROPERTY(EditAnywhere,BlueprintReadWrite) FVector ADSArmOffset = FVector::ZeroVector;
 UPROPERTY(EditAnywhere,BlueprintReadWrite) FVector ViewOffset = FVector::ZeroVector;
 UPROPERTY(EditAnywhere,BlueprintReadWrite) FVector ADSOffset = FVector::ZeroVector;
 UPROPERTY(EditAnywhere,BlueprintReadWrite) FText ModelLabel;
 UPROPERTY(EditAnywhere,BlueprintReadWrite) bool bAvailable = true;
 UPROPERTY(EditAnywhere,BlueprintReadWrite) bool bRepresentative = true;
};
