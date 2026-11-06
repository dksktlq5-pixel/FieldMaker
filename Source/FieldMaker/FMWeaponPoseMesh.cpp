#include "FMWeaponPoseMesh.h"

#include "Algo/Find.h"
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
	{TEXT("pinky_01_r"), TEXT("pinky_02_r"), TEXT("pinky_03_r")}
};
const FDigit LeftDigits[] = {
	{TEXT("index_01_l"), TEXT("index_02_l"), TEXT("index_03_l")},
	{TEXT("middle_01_l"), TEXT("middle_02_l"), TEXT("middle_03_l")},
	{TEXT("ring_01_l"), TEXT("ring_02_l"), TEXT("ring_03_l")},
	{TEXT("pinky_01_l"), TEXT("pinky_02_l"), TEXT("pinky_03_l")}
};

void AimJoint(FTransform& Joint, const FTransform& Child, const FVector& Direction)
{
	const FVector Current = (Child.GetLocation() - Joint.GetLocation()).GetSafeNormal();
	if (!Current.IsNearlyZero())
	{
		Joint.SetRotation(FQuat::FindBetweenNormals(Current, Direction.GetSafeNormal()) * Joint.GetRotation());
	}
}

void PoseDigit(const FReferenceSkeleton& Skeleton, TArray<FTransform>& Pose, const FDigit& Digit,
	const FVector& ProximalDirection, const FVector& MiddleDirection, const FQuat& TipBend)
{
	const int32 A = Skeleton.FindBoneIndex(Digit.Proximal);
	const int32 B = Skeleton.FindBoneIndex(Digit.Middle);
	const int32 C = Skeleton.FindBoneIndex(Digit.Tip);
	if (!Pose.IsValidIndex(A) || !Pose.IsValidIndex(B) || !Pose.IsValidIndex(C))
	{
		return;
	}
	const FTransform SecondLocal = Pose[B].GetRelativeTransform(Pose[A]);
	const FTransform ThirdLocal = Pose[C].GetRelativeTransform(Pose[B]);
	AimJoint(Pose[A], Pose[B], ProximalDirection);
	Pose[B] = SecondLocal * Pose[A];
	Pose[C] = ThirdLocal * Pose[B];
	AimJoint(Pose[B], Pose[C], MiddleDirection);
	Pose[C] = ThirdLocal * Pose[B];
	Pose[C].SetRotation(TipBend * Pose[C].GetRotation());
}

void PoseFingers(const FReferenceSkeleton& Skeleton, TArray<FTransform>& Pose,
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
	const FDigit* Digits = bRight ? RightDigits : LeftDigits;
	for (int32 Index = 0; Index < UE_ARRAY_COUNT(RightDigits); ++Index)
	{
		// The outer digits close progressively farther around the grip than the middle finger.
		const double OuterCurl = FMath::Clamp((Hand.FingerBend.X - 25.0) / 40.0, 0.0, 1.0);
		const FVector2D Bend = Index == 0 ? Hand.IndexBend :
			Hand.FingerBend + FVector2D(3.0, 4.0) * ((Index - 1) * OuterCurl);
		const double Spread = (1.5 - Index) * FMath::Lerp(0.045, 0.016, OuterCurl);
		const FQuat TipBend(Wrist.GetUnitAxis(EAxis::Z), -FMath::DegreesToRadians(Bend.X * 0.20));
		PoseDigit(Skeleton, Pose, Digits[Index], Direction(Bend.X, Spread), Direction(Bend.Y, Spread), TipBend);
	}

	const FDigit Thumb = bRight
		? FDigit{TEXT("thumb_01_r"), TEXT("thumb_02_r"), TEXT("thumb_03_r")}
		: FDigit{TEXT("thumb_01_l"), TEXT("thumb_02_l"), TEXT("thumb_03_l")};
	const float Opposition = Hand.ThumbOpposition;
	const FVector ThumbBase(-0.70, 0.38 + 0.16 * Opposition, 0.63);
	const FVector ThumbTip(-0.72, 0.35 + 0.45 * Opposition, 0.35 - 0.42 * Opposition);
	PoseDigit(Skeleton, Pose, Thumb,
		Wrist.TransformVectorNoScale(FMath::Lerp(ThumbBase, FVector(-0.91, 0.06, 0.36), Hand.ThumbAlongFrame) * Side),
		Wrist.TransformVectorNoScale(FMath::Lerp(ThumbTip, FVector(-0.99, 0.08, -0.08), Hand.ThumbAlongFrame) * Side),
		FQuat::Identity);
}

void SolveArm(const FReferenceSkeleton& Skeleton, TArray<FTransform>& Pose, const FFMHandTarget& Target, bool bRight, bool bAlignKnifeWrist = false)
{
	const int32 Shoulder = Skeleton.FindBoneIndex(bRight ? TEXT("upperarm_r") : TEXT("upperarm_l"));
	const int32 Elbow = Skeleton.FindBoneIndex(bRight ? TEXT("lowerarm_r") : TEXT("lowerarm_l"));
	const int32 Wrist = Skeleton.FindBoneIndex(bRight ? TEXT("hand_r") : TEXT("hand_l"));
	if (!Pose.IsValidIndex(Shoulder) || !Pose.IsValidIndex(Elbow) || !Pose.IsValidIndex(Wrist))
	{
		return;
	}

	const FTransform OldShoulder = Pose[Shoulder];
	const FTransform OldElbow = Pose[Elbow];
	const FTransform OldWrist = Pose[Wrist];
	// Component +Y is forward and -X is the character's right. Keep the elbows low and outboard.
	const FVector PoleOffset = bAlignKnifeWrist
		? FVector(12, 14, Target.Wrist.GetLocation().Z - OldShoulder.GetLocation().Z)
		: FVector(bRight ? -26 : 26, 5, -46);
	const FVector Pole = OldShoulder.GetLocation() + PoleOffset;
	AnimationCore::SolveTwoBoneIK(Pose[Shoulder], Pose[Elbow], Pose[Wrist], Pole, Target.Wrist.GetLocation(), false, 1.f, 1.f);
	// Share wrist roll with the forearm without moving the elbow or changing bone lengths.
	const FVector ForearmAxis = (Pose[Wrist].GetLocation() - Pose[Elbow].GetLocation()).GetSafeNormal();
	FQuat WristRotation = Target.Wrist.GetRotation();
	if (!ForearmAxis.IsNearlyZero())
	{
		if (bAlignKnifeWrist)
		{
			// The elbow stays inboard; the palm and blade lead forward across the lower screen.
			const FVector ThumbSide = FVector(1, 0.7, 0.30) + 0.10 * Target.Wrist.GetUnitAxis(EAxis::Z);
			WristRotation = FRotationMatrix::MakeFromXZ(-ForearmAxis, ThumbSide).ToQuat();
		}
		const FQuat WristDelta = WristRotation * Pose[Wrist].GetRotation().Inverse();
		const double Twist = WristDelta.GetTwistAngle(ForearmAxis);
		const double RollLimit = bAlignKnifeWrist ? UE_PI * 0.65 : UE_PI / 4.0;
		const double Roll = FMath::Clamp(Twist * (bAlignKnifeWrist ? 1.0 : 0.60), -RollLimit, RollLimit);
		Pose[Elbow].SetRotation(FQuat(ForearmAxis, Roll) * Pose[Elbow].GetRotation());
	}
	Pose[Wrist].SetRotation(WristRotation);

	const int32 Chain[] = {Shoulder, Elbow, Wrist};
	const FTransform Previous[] = {OldShoulder, OldElbow, OldWrist};
	for (int32 Bone = Shoulder + 1; Bone < Pose.Num(); ++Bone)
	{
		if (Bone == Elbow || Bone == Wrist)
		{
			continue;
		}

		for (int32 Parent = Skeleton.GetParentIndex(Bone); Parent >= Shoulder; Parent = Skeleton.GetParentIndex(Parent))
		{
			const int32* ChainBone = Algo::Find(Chain, Parent);
			if (ChainBone)
			{
				Pose[Bone] = Pose[Bone].GetRelativeTransform(Previous[ChainBone - Chain]) * Pose[Parent];
				break;
			}
		}
	}
	PoseFingers(Skeleton, Pose, Pose[Wrist], Target, bRight);
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
			const FReferenceSkeleton& Skeleton = GetSkeletalMeshAsset()->GetRefSkeleton();
			TArray<FTransform>& Pose = GetEditableComponentSpaceTransforms();
			SolveArm(Skeleton, Pose, Grips.Right, true, Grips.bAlignKnifeWrist);
			SolveArm(Skeleton, Pose, Grips.Left, false);
		}
	}
	Super::FinalizeBoneTransform();
}
