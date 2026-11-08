#include "FMWeaponPoseMesh.h"

#include "Engine/SkeletalMesh.h"
#include "FMCharacter.h"
#include "Math/RotationMatrix.h"
#include "TwoBoneIK.h"

namespace
{
struct FDigit
{
	FName Proximal;
	FName Middle;
	FName Tip;
};

const FDigit RightDigits[] = {
	{TEXT("index_01_r"), TEXT("index_02_r"), TEXT("index_03_r")},
	{TEXT("middle_01_r"), TEXT("middle_02_r"), TEXT("middle_03_r")},
	{TEXT("ring_01_r"), TEXT("ring_02_r"), TEXT("ring_03_r")},
	{TEXT("pinky_01_r"), TEXT("pinky_02_r"), TEXT("pinky_03_r")},
	{TEXT("thumb_01_r"), TEXT("thumb_02_r"), TEXT("thumb_03_r")}
};
const FDigit LeftDigits[] = {
	{TEXT("index_01_l"), TEXT("index_02_l"), TEXT("index_03_l")},
	{TEXT("middle_01_l"), TEXT("middle_02_l"), TEXT("middle_03_l")},
	{TEXT("ring_01_l"), TEXT("ring_02_l"), TEXT("ring_03_l")},
	{TEXT("pinky_01_l"), TEXT("pinky_02_l"), TEXT("pinky_03_l")},
	{TEXT("thumb_01_l"), TEXT("thumb_02_l"), TEXT("thumb_03_l")}
};

FFMPoseChain BuildChain(const FReferenceSkeleton& Skeleton, const FDigit& Names)
{
	FFMPoseChain Chain;
	Chain.Bones[0] = Skeleton.FindBoneIndex(Names.Proximal);
	Chain.Bones[1] = Skeleton.FindBoneIndex(Names.Middle);
	Chain.Bones[2] = Skeleton.FindBoneIndex(Names.Tip);
	if (Chain.Bones[0] == INDEX_NONE || Chain.Bones[1] == INDEX_NONE || Chain.Bones[2] == INDEX_NONE)
	{
		return Chain;
	}

	TArray<int32, TInlineAllocator<512>> NearestDriver;
	NearestDriver.Init(INDEX_NONE, Skeleton.GetNum());
	for (int32 Bone = Chain.Bones[0]; Bone < Skeleton.GetNum(); ++Bone)
	{
		if (Bone == Chain.Bones[0] || Bone == Chain.Bones[1] || Bone == Chain.Bones[2])
		{
			NearestDriver[Bone] = Bone == Chain.Bones[0] ? 0 : Bone == Chain.Bones[1] ? 1 : 2;
			continue;
		}
		const int32 Parent = Skeleton.GetParentIndex(Bone);
		if (Parent != INDEX_NONE && NearestDriver[Parent] != INDEX_NONE)
		{
			const int32 Driver = NearestDriver[Parent];
			NearestDriver[Bone] = Driver;
			Chain.Descendants.Add({Bone, Driver});
		}
	}
	return Chain;
}

FFMPoseArm BuildArm(const FReferenceSkeleton& Skeleton, bool bRight)
{
	FFMPoseArm Rig;
	Rig.Arm = BuildChain(Skeleton, bRight
		? FDigit{TEXT("upperarm_r"), TEXT("lowerarm_r"), TEXT("hand_r")}
		: FDigit{TEXT("upperarm_l"), TEXT("lowerarm_l"), TEXT("hand_l")});
	const auto& Digits = bRight ? RightDigits : LeftDigits;
	for (int32 Index = 0; Index < UE_ARRAY_COUNT(Rig.Digits); ++Index)
	{
		Rig.Digits[Index] = BuildChain(Skeleton, Digits[Index]);
	}
	return Rig;
}

void RebaseDescendants(const FFMPoseChain& Chain, TArray<FTransform>& Pose, const FTransform (&Previous)[3])
{
	for (const auto& Entry : Chain.Descendants)
	{
		if (Pose.IsValidIndex(Entry.Bone))
		{
			Pose[Entry.Bone] = Pose[Entry.Bone].GetRelativeTransform(Previous[Entry.Driver]) * Pose[Chain.Bones[Entry.Driver]];
		}
	}
}

void AimJoint(FTransform& Joint, const FTransform& Child, const FVector& Direction)
{
	const FVector Current = (Child.GetLocation() - Joint.GetLocation()).GetSafeNormal();
	if (!Current.IsNearlyZero())
	{
		Joint.SetRotation(FQuat::FindBetweenNormals(Current, Direction.GetSafeNormal()) * Joint.GetRotation());
	}
}

void PoseDigit(const FFMPoseChain& Chain, TArray<FTransform>& Pose,
	const FVector& ProximalDirection, const FVector& MiddleDirection, const FQuat& TipBend)
{
	const int32 A = Chain.Bones[0];
	const int32 B = Chain.Bones[1];
	const int32 C = Chain.Bones[2];
	if (!Pose.IsValidIndex(A) || !Pose.IsValidIndex(B) || !Pose.IsValidIndex(C))
	{
		return;
	}
	const FTransform DigitPrevious[] = {Pose[A], Pose[B], Pose[C]};
	const FTransform SecondLocal = Pose[B].GetRelativeTransform(Pose[A]);
	const FTransform ThirdLocal = Pose[C].GetRelativeTransform(Pose[B]);
	AimJoint(Pose[A], Pose[B], ProximalDirection);
	Pose[B] = SecondLocal * Pose[A];
	Pose[C] = ThirdLocal * Pose[B];
	AimJoint(Pose[B], Pose[C], MiddleDirection);
	Pose[C] = ThirdLocal * Pose[B];
	Pose[C].SetRotation(TipBend * Pose[C].GetRotation());
	// Preserve the local transforms of Quantum's finger correction bones.
	RebaseDescendants(Chain, Pose, DigitPrevious);
}

void PoseFingers(const FFMPoseArm& Rig, TArray<FTransform>& Pose,
	const FTransform& Wrist, const FFMHandTarget& Hand, bool bRight)
{
	// The two meshes use mirrored hand axes. The palm faces +Y on the right and -Y on the left.
	const double Side = bRight ? 1.0 : -1.0;
	const auto Direction = [&Wrist, Side](double Angle, double Spread)
	{
		const double Radians = FMath::DegreesToRadians(Angle);
		return Wrist.TransformVectorNoScale(FVector(-Side * FMath::Cos(Radians),
			Side * FMath::Sin(Radians), Spread * Side));
	};
	for (int32 Index = 0; Index < UE_ARRAY_COUNT(Rig.Digits) - 1; ++Index)
	{
		// The outer digits close progressively farther around the grip than the middle finger.
		const double OuterCurl = FMath::Clamp((Hand.FingerBend.X - 25.0) / 40.0, 0.0, 1.0);
		const FVector2D Bend = Index == 0 ? Hand.IndexBend :
			Hand.FingerBend + FVector2D(3.0, 4.0) * ((Index - 1) * OuterCurl);
		const double Spread = (1.5 - Index) * FMath::Lerp(0.045, 0.016, OuterCurl);
		const FQuat TipBend(Wrist.GetUnitAxis(EAxis::Z), -FMath::DegreesToRadians(Bend.X * 0.20));
		PoseDigit(Rig.Digits[Index], Pose, Direction(Bend.X, Spread), Direction(Bend.Y, Spread), TipBend);
	}

	const float Opposition = Hand.ThumbOpposition;
	const bool bSupportThumb=Hand.bWeaponGrip && !bRight;
 const FVector ThumbBase=bSupportThumb?FVector(-.75,.32,-.50):FVector(-0.70,0.38+0.16*Opposition,0.63);
	const FVector ThumbTip=bSupportThumb?FVector(-.82,.35,-.30):FVector(-0.72,0.35+0.45*Opposition,0.35-0.42*Opposition);
 const FVector AlongBase=bSupportThumb?FVector(-.86,.08,-.50):Hand.bWeaponGrip?FVector(-.91,-.10,.36):FVector(-.91,.06,.36);
 const FVector AlongTip=bSupportThumb?FVector(-.98,.06,-.23):Hand.bWeaponGrip?FVector(-.99,-.10,-.08):FVector(-.99,.08,-.08);
	PoseDigit(Rig.Digits[UE_ARRAY_COUNT(Rig.Digits) - 1], Pose,
		Wrist.TransformVectorNoScale(FMath::Lerp(ThumbBase, AlongBase, Hand.ThumbAlongFrame) * Side),
		Wrist.TransformVectorNoScale(FMath::Lerp(ThumbTip, AlongTip, Hand.ThumbAlongFrame) * Side),
		FQuat::Identity);
}

void SolveArm(const FFMPoseArm& Rig, TArray<FTransform>& Pose, const FFMHandTarget& Target, bool bRight,
	bool bAlignKnifeWrist = false, bool bFirstPersonKnife = false)
{
	const int32 Shoulder = Rig.Arm.Bones[0];
	const int32 Elbow = Rig.Arm.Bones[1];
	const int32 Wrist = Rig.Arm.Bones[2];
	if (!Pose.IsValidIndex(Shoulder) || !Pose.IsValidIndex(Elbow) || !Pose.IsValidIndex(Wrist))
	{
		return;
	}

	const FTransform OldShoulder = Pose[Shoulder];
	const FTransform OldElbow = Pose[Elbow];
	const FTransform OldWrist = Pose[Wrist];
	// Component +Y is forward and -X is the character's right. Keep the elbows low and outboard.
	const FVector PoleOffset = bFirstPersonKnife
		? FVector(12, 14, Target.Wrist.GetLocation().Z - OldShoulder.GetLocation().Z)
		: bAlignKnifeWrist ? FVector(-22, -4, -32)
		: Target.bWeaponGrip && !bRight?Target.ElbowPole:FVector(bRight ? -26 : 26,5,-46);
	const FVector Pole = OldShoulder.GetLocation() + PoleOffset;
	AnimationCore::SolveTwoBoneIK(Pose[Shoulder], Pose[Elbow], Pose[Wrist], Pole, Target.Wrist.GetLocation(), false, 1.f, 1.f);
	// Share wrist roll with the forearm without moving the elbow or changing bone lengths.
	const FVector ForearmAxis = (Pose[Wrist].GetLocation() - Pose[Elbow].GetLocation()).GetSafeNormal();
	FQuat WristRotation = Target.Wrist.GetRotation();
	if (!ForearmAxis.IsNearlyZero())
	{
		if (bAlignKnifeWrist)
		{
			// Keep the wrist in line with the forearm, using the swing track for world-space blade roll.
			const FVector ThumbSide = bFirstPersonKnife
				? FVector(1, 0.7, 0.30) + 0.10 * Target.Wrist.GetUnitAxis(EAxis::Z)
				: Target.Wrist.GetUnitAxis(EAxis::Z);
			WristRotation = FRotationMatrix::MakeFromXZ(-ForearmAxis, ThumbSide).ToQuat();
		}
		const FQuat WristDelta = WristRotation * Pose[Wrist].GetRotation().Inverse();
		const double Twist = WristDelta.GetTwistAngle(ForearmAxis);
		const double RollLimit = bAlignKnifeWrist ? UE_PI * 0.65 : UE_PI / 4.0;
		const double Roll = FMath::Clamp(Twist * (bAlignKnifeWrist ? 1.0 : 0.60), -RollLimit, RollLimit);
		Pose[Elbow].SetRotation(FQuat(ForearmAxis, Roll) * Pose[Elbow].GetRotation());
	}
	Pose[Wrist].SetRotation(WristRotation);

	const FTransform Previous[] = {OldShoulder, OldElbow, OldWrist};
	RebaseDescendants(Rig.Arm, Pose, Previous);
	PoseFingers(Rig, Pose, Pose[Wrist], Target, bRight);
}
}

void UFMWeaponPoseMesh::FinalizeBoneTransform()
{
	if (bNeedToFlipSpaceBaseBuffers && GetSkeletalMeshAsset())
	{
		FFMGripTargets Grips;
		const auto* Character = Cast<AFMCharacter>(GetOwner());
		if (Character && Character->GetWeaponGripTargets(*this, Grips))
		{
			const auto* Mesh = GetSkeletalMeshAsset();
			if (CachedRigMesh.Get() != Mesh)
			{
				RightRig = BuildArm(Mesh->GetRefSkeleton(), true);
				LeftRig = BuildArm(Mesh->GetRefSkeleton(), false);
				CachedRigMesh = GetSkeletalMeshAsset();
			}
			TArray<FTransform>& Pose = GetEditableComponentSpaceTransforms();
			SolveArm(RightRig, Pose, Grips.Right, true, Grips.bAlignKnifeWrist, Grips.bFirstPersonKnife);
			SolveArm(LeftRig, Pose, Grips.Left, false);
		}
	}
	Super::FinalizeBoneTransform();
}
