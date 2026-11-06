#include "FMCharacterAnimInstance.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Animation/AnimNode_SequencePlayer.h"
#include "Animation/AnimClassInterface.h"
#include "Animation/AnimSequenceBase.h"
#include "UObject/UnrealType.h"

void UFMCharacterAnimInstance::NativeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeUpdateAnimation(DeltaSeconds);

	const auto* Character = Cast<ACharacter>(TryGetPawnOwner());
	if (!Character)
	{
		return;
	}

	const FVector Velocity = Character->GetVelocity();
	const double GroundSpeed = Velocity.Size2D();
	bGrounded = Character->GetCharacterMovement()->IsMovingOnGround();
	const bool bInAir = Character->GetCharacterMovement()->IsFalling();

	bJumping = bInAir && Velocity.Z > 0.0;
	bFalling = bInAir && !bJumping;
	bIdle = bGrounded && GroundSpeed <= 5.0;
	bWalking = bGrounded && GroundSpeed > 5.0 && GroundSpeed <= 330.0;
	bRunning = bGrounded && GroundSpeed > 330.0;
	const FVector LocalVelocity = Character->GetActorTransform().InverseTransformVectorNoScale(Velocity);
	const float Direction = LocalVelocity.X < -10.f ? -1.f : 1.f;
	WalkAnimationRate = Direction * FMath::Clamp(float(GroundSpeed) / 200.f, 0.15f, 1.65f);
	// Change only grounded locomotion players; Jump/Fall/Land keep their original timing.
	if (const auto* AnimClass = IAnimClassInterface::GetFromClass(GetClass()))
	{
		for (FStructProperty* Property : AnimClass->GetAnimNodeProperties())
		{
			if (Property->Struct == FAnimNode_SequencePlayer::StaticStruct())
			{
				auto* Player = Property->ContainerPtrToValuePtr<FAnimNode_SequencePlayer>(this);
				const auto* Sequence = Player->GetSequence();
				if (!Sequence) continue;
				if (Sequence->GetFName() == TEXT("A_MM_Walk_InPlace"))
				{
					Player->SetPlayRate(WalkAnimationRate);
					WalkAnimationRate = Player->GetPlayRate();
				}
				else if (Sequence->GetFName() == TEXT("A_FMRun_InPlace")) Player->SetPlayRate(FMath::Clamp(float(GroundSpeed)/550.f, 0.6f, 1.3f));
			}
		}
	}
}
