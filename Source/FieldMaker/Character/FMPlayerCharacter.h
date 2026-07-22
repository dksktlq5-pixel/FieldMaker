#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "FMPlayerCharacter.generated.h"

class UCameraComponent;
class USkeletalMeshComponent;
class UInputComponent;
class UInputMappingContext;
class UInputAction;

struct FInputActionValue;

UCLASS()
class FIELDMAKER_API AFMPlayerCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	AFMPlayerCharacter();

protected:
	virtual void BeginPlay() override;

public:
	virtual void SetupPlayerInputComponent(
		UInputComponent* PlayerInputComponent
	) override;

protected:
	/* =========================================================
	 * Components
	 * ========================================================= */

	/**
	 * 플레이어 본인 시점에서 사용하는 1인칭 카메라
	 */
	UPROPERTY(
		VisibleAnywhere,
		BlueprintReadOnly,
		Category = "FM|Camera",
		meta = (AllowPrivateAccess = "true")
	)
	TObjectPtr<UCameraComponent> FirstPersonCamera;

	/**
	 * 플레이어 본인에게만 보이는 1인칭 팔 메시
	 */
	UPROPERTY(
		VisibleAnywhere,
		BlueprintReadOnly,
		Category = "FM|Character",
		meta = (AllowPrivateAccess = "true")
	)
	TObjectPtr<USkeletalMeshComponent> FirstPersonMesh;

	/* =========================================================
	 * Enhanced Input
	 * ========================================================= */

	/**
	 * 플레이어 기본 입력 매핑 컨텍스트
	 */
	UPROPERTY(
		EditDefaultsOnly,
		BlueprintReadOnly,
		Category = "FM|Input"
	)
	TObjectPtr<UInputMappingContext> PlayerMappingContext;

	/**
	 * 이동 입력
	 */
	UPROPERTY(
		EditDefaultsOnly,
		BlueprintReadOnly,
		Category = "FM|Input"
	)
	TObjectPtr<UInputAction> MoveAction;

	/**
	 * 시점 입력
	 */
	UPROPERTY(
		EditDefaultsOnly,
		BlueprintReadOnly,
		Category = "FM|Input"
	)
	TObjectPtr<UInputAction> LookAction;

	/**
	 * 점프 입력
	 */
	UPROPERTY(
		EditDefaultsOnly,
		BlueprintReadOnly,
		Category = "FM|Input"
	)
	TObjectPtr<UInputAction> JumpAction;

	/**
	 * 앉기 입력
	 */
	UPROPERTY(
		EditDefaultsOnly,
		BlueprintReadOnly,
		Category = "FM|Input"
	)
	TObjectPtr<UInputAction> CrouchAction;

	/**
	 * 조용히 걷기 입력
	 */
	UPROPERTY(
		EditDefaultsOnly,
		BlueprintReadOnly,
		Category = "FM|Input"
	)
	TObjectPtr<UInputAction> SilentWalkAction;

	/* =========================================================
	 * Movement
	 * ========================================================= */

	/**
	 * 평상시 이동 속도
	 */
	UPROPERTY(
		EditDefaultsOnly,
		BlueprintReadOnly,
		Category = "FM|Movement"
	)
	float DefaultMoveSpeed = 715.0f;

	/**
	 * Shift 조용히 걷기 속도
	 */
	UPROPERTY(
		EditDefaultsOnly,
		BlueprintReadOnly,
		Category = "FM|Movement"
	)
	float SilentWalkSpeed = 500.0f;

	/**
	 * 앉은 상태 이동 속도
	 */
	UPROPERTY(
		EditDefaultsOnly,
		BlueprintReadOnly,
		Category = "FM|Movement"
	)
	float CrouchedMoveSpeed = 280.0f;

private:
	/* =========================================================
	 * Input Functions
	 * ========================================================= */

	void Move(const FInputActionValue& Value);
	void Look(const FInputActionValue& Value);

	void StartJump();
	void StopJump();

	void ToggleCrouch();

	void StartSilentWalk();
	void StopSilentWalk();
};