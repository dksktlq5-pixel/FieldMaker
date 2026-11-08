#pragma once

#include "Components/SkeletalMeshComponent.h"
#include "FMWeaponPoseMesh.generated.h"

struct FFMHandTarget
{
	FTransform Wrist;
	FVector2D FingerBend = FVector2D::ZeroVector;
	FVector2D IndexBend = FVector2D::ZeroVector;
	float ThumbOpposition = 0.f;
	float ThumbAlongFrame = 0.f;
 bool bWeaponGrip = false;
 FVector ElbowPole = FVector(22,-8,-18);
};

struct FFMGripTargets
{
	FFMHandTarget Right;
	FFMHandTarget Left;
	bool bAlignKnifeWrist = false;
	bool bFirstPersonKnife = false;
};

struct FFMPoseChain
{
	struct FDescendant
	{
		int32 Bone;
		int32 Driver;
	};

	int32 Bones[3] = {INDEX_NONE, INDEX_NONE, INDEX_NONE};
	TArray<FDescendant> Descendants;
};

struct FFMPoseArm
{
	FFMPoseChain Arm;
	FFMPoseChain Digits[5];
};

// Applies hand IK before the evaluated pose reaches the renderer and outfit followers.
UCLASS()
class UFMWeaponPoseMesh : public USkeletalMeshComponent
{
	GENERATED_BODY()

public:
	virtual void FinalizeBoneTransform() override;

private:
	TWeakObjectPtr<USkeletalMesh> CachedRigMesh;
	FFMPoseArm RightRig;
	FFMPoseArm LeftRig;
};
