#pragma once

#include "Animation/AnimInstance.h"
#include "FMCharacterAnimInstance.generated.h"

UCLASS(Blueprintable)
class FIELDMAKER_API UFMCharacterAnimInstance : public UAnimInstance
{
	GENERATED_BODY()

public:
	virtual void NativeUpdateAnimation(float DeltaSeconds) override;

	UFUNCTION(BlueprintPure, Category = "Locomotion")
	FName GetLocomotionState() const { return GetCurrentStateName(0); }
	UFUNCTION(BlueprintPure, Category="Locomotion")
	float GetWalkAnimationRate() const { return WalkAnimationRate; }

protected:
	UPROPERTY(BlueprintReadOnly, Category = "Locomotion")
	bool bIdle = true;

	UPROPERTY(BlueprintReadOnly, Category = "Locomotion")
	bool bWalking = false;

	UPROPERTY(BlueprintReadOnly, Category = "Locomotion")
	bool bRunning = false;

	UPROPERTY(BlueprintReadOnly, Category = "Locomotion")
	bool bJumping = false;

	UPROPERTY(BlueprintReadOnly, Category = "Locomotion")
	bool bFalling = false;

	UPROPERTY(BlueprintReadOnly, Category = "Locomotion")
	bool bGrounded = true;
	float WalkAnimationRate = 1.f;
};
