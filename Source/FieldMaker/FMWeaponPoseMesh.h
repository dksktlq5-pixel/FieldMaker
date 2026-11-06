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
};

struct FFMGripTargets
{
	FFMHandTarget Right;
	FFMHandTarget Left;
	bool bAlignKnifeWrist = false;
};

// Applies hand IK before the evaluated pose reaches the renderer and outfit followers.
UCLASS()
class UFMWeaponPoseMesh : public USkeletalMeshComponent
{
	GENERATED_BODY()

public:
	virtual void FinalizeBoneTransform() override;
};
