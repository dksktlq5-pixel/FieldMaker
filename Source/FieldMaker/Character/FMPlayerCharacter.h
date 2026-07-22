#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "FMPlayerCharacter.generated.h"

class UCameraComponent;
class USkeletalMeshComponent;
class UInputComponent;
class UInputMappingContext;
class UInputAction;

class UFMInventoryComponent;
class UFMEquipmentComponent;

struct FInputActionValue;

UCLASS()
class FIELDMAKER_API AFMPlayerCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	AFMPlayerCharacter();

	/* =========================================================
	 * Component Getters
	 * ========================================================= */

	UFUNCTION(BlueprintPure, Category = "FM|Character")
	USkeletalMeshComponent* GetFirstPersonMesh() const
	{
		return FirstPersonMesh;
	}

	UFUNCTION(BlueprintPure, Category = "FM|Inventory")
	UFMInventoryComponent* GetInventoryComponent() const
	{
		return InventoryComponent;
	}

	UFUNCTION(BlueprintPure, Category = "FM|Equipment")
	UFMEquipmentComponent* GetEquipmentComponent() const
	{
		return EquipmentComponent;
	}

protected:
	virtual void BeginPlay() override;

public:
	virtual void SetupPlayerInputComponent(
		UInputComponent* PlayerInputComponent
	) override;

protected:
	/* =========================================================
	 * Camera / Mesh Components
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

	/**
	 * 플레이어가 보유한 무기 슬롯을 관리
	 */
	UPROPERTY(
		VisibleAnywhere,
		BlueprintReadOnly,
		Category = "FM|Components",
		meta = (AllowPrivateAccess = "true")
	)
	TObjectPtr<UFMInventoryComponent> InventoryComponent;

	/**
	 * 현재 장착한 무기와 선택 슬롯을 관리
	 */
	UPROPERTY(
		VisibleAnywhere,
		BlueprintReadOnly,
		Category = "FM|Components",
		meta = (AllowPrivateAccess = "true")
	)
	TObjectPtr<UFMEquipmentComponent> EquipmentComponent;

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

	/**
	 * 1번 주무기 슬롯 입력
	 */
	UPROPERTY(
		EditDefaultsOnly,
		BlueprintReadOnly,
		Category = "FM|Input|Weapon"
	)
	TObjectPtr<UInputAction> PrimaryWeaponAction;

	/**
	 * 2번 보조무기 슬롯 입력
	 */
	UPROPERTY(
		EditDefaultsOnly,
		BlueprintReadOnly,
		Category = "FM|Input|Weapon"
	)
	TObjectPtr<UInputAction> SecondaryWeaponAction;

	/**
	 * 3번 근접무기 슬롯 입력
	 */
	UPROPERTY(
		EditDefaultsOnly,
		BlueprintReadOnly,
		Category = "FM|Input|Weapon"
	)
	TObjectPtr<UInputAction> MeleeWeaponAction;

	/**
	 * 4번 투척무기 슬롯 입력
	 */
	UPROPERTY(
		EditDefaultsOnly,
		BlueprintReadOnly,
		Category = "FM|Input|Weapon"
	)
	TObjectPtr<UInputAction> ThrowableWeaponAction;
	
	/**
 * 무기 발사 입력
 */
	UPROPERTY(
		EditDefaultsOnly,
		BlueprintReadOnly,
		Category = "FM|Input|Weapon"
	)
	TObjectPtr<UInputAction> FireAction;

	/**
	 * 무기 재장전 입력
	 */
	UPROPERTY(
		EditDefaultsOnly,
		BlueprintReadOnly,
		Category = "FM|Input|Weapon"
	)
	TObjectPtr<UInputAction> ReloadAction;

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
	 * Movement Input Functions
	 * ========================================================= */

	void Move(const FInputActionValue& Value);
	void Look(const FInputActionValue& Value);

	void StartJump();
	void StopJump();

	void ToggleCrouch();

	void StartSilentWalk();
	void StopSilentWalk();

	/* =========================================================
	 * Weapon Slot Input Functions
	 * ========================================================= */

	void EquipPrimaryWeapon();
	void EquipSecondaryWeapon();
	void EquipMeleeWeapon();
	void EquipThrowableWeapon();
	void StartFire();
	void StopFire();
	void ReloadWeapon();
};