#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "FMPlayerAnimInstance.generated.h"

class AFMPlayerCharacter;
class UCharacterMovementComponent;

UCLASS()
class FIELDMAKER_API UFMPlayerAnimInstance : public UAnimInstance
{
	GENERATED_BODY()

public:
	virtual void NativeInitializeAnimation() override;
	virtual void NativeUpdateAnimation(float DeltaSeconds) override;

protected:
	/**
	 * 현재 애니메이션을 사용하는 FIELDMAKER 플레이어 캐릭터
	 */
	UPROPERTY(
		BlueprintReadOnly,
		Category = "FM|Character"
	)
	TObjectPtr<AFMPlayerCharacter> PlayerCharacter;

	/**
	 * 플레이어의 이동 컴포넌트
	 */
	UPROPERTY(
		BlueprintReadOnly,
		Category = "FM|Character"
	)
	TObjectPtr<UCharacterMovementComponent> CharacterMovement;

	/**
	 * 현재 캐릭터 속도
	 */
	UPROPERTY(
		BlueprintReadOnly,
		Category = "FM|Movement"
	)
	FVector Velocity = FVector::ZeroVector;

	/**
	 * 수평 이동 속도
	 */
	UPROPERTY(
		BlueprintReadOnly,
		Category = "FM|Movement"
	)
	float GroundSpeed = 0.0f;

	/**
	 * 이동 방향
	 *
	 * -180 ~ 180도
	 * 0도: 전진
	 * 90도: 오른쪽
	 * -90도: 왼쪽
	 * 180도: 후진
	 */
	UPROPERTY(
		BlueprintReadOnly,
		Category = "FM|Movement"
	)
	float Direction = 0.0f;

	/**
	 * 실제 지상 이동 중인지 여부
	 */
	UPROPERTY(
		BlueprintReadOnly,
		Category = "FM|Movement"
	)
	bool bShouldMove = false;

	/**
	 * 공중 상태인지 여부
	 */
	UPROPERTY(
		BlueprintReadOnly,
		Category = "FM|Movement"
	)
	bool bIsFalling = false;

	/**
	 * 앉은 상태인지 여부
	 */
	UPROPERTY(
		BlueprintReadOnly,
		Category = "FM|Movement"
	)
	bool bIsCrouched = false;
};