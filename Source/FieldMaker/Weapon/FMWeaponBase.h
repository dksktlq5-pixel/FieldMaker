#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "FMWeaponTypes.h"
#include "FMWeaponBase.generated.h"

class USceneComponent;
class UDamageType;
class UDataTable;

UCLASS()
class FIELDMAKER_API AFMWeaponBase : public AActor
{
	GENERATED_BODY()

public:
	AFMWeaponBase();

	virtual void GetLifetimeReplicatedProps(
		TArray<FLifetimeProperty>& OutLifetimeProps
	) const override;

	/* =========================================================
	 * Weapon Information
	 * ========================================================= */

	UFUNCTION(BlueprintPure, Category = "FM|Weapon")
	EFMWeaponSlot GetWeaponSlot() const
	{
		return WeaponSlot;
	}

	UFUNCTION(BlueprintPure, Category = "FM|Weapon")
	FName GetWeaponID() const
	{
		return WeaponID;
	}

	UFUNCTION(BlueprintPure, Category = "FM|Weapon|Data")
	FName GetWeaponRowName() const
	{
		return WeaponRowName;
	}

	UFUNCTION(BlueprintPure, Category = "FM|Weapon|Attachment")
	FName GetFirstPersonSocketName() const
	{
		return FirstPersonSocketName;
	}

	UFUNCTION(BlueprintPure, Category = "FM|Weapon|Attachment")
	FName GetThirdPersonSocketName() const
	{
		return ThirdPersonSocketName;
	}

	/* =========================================================
	 * Ammo
	 * ========================================================= */

	UFUNCTION(BlueprintPure, Category = "FM|Weapon|Ammo")
	int32 GetCurrentAmmo() const
	{
		return CurrentAmmo;
	}

	UFUNCTION(BlueprintPure, Category = "FM|Weapon|Ammo")
	int32 GetReserveAmmo() const
	{
		return ReserveAmmo;
	}

	UFUNCTION(BlueprintPure, Category = "FM|Weapon|Ammo")
	int32 GetMagazineSize() const
	{
		return MagazineSize;
	}

	UFUNCTION(BlueprintPure, Category = "FM|Weapon|Ammo")
	bool HasAmmo() const
	{
		return CurrentAmmo > 0;
	}

	UFUNCTION(BlueprintPure, Category = "FM|Weapon|Ammo")
	bool CanReload() const
	{
		return
			CurrentAmmo < MagazineSize &&
			ReserveAmmo > 0 &&
			WeaponState != EFMWeaponState::Reloading;
	}

	/* =========================================================
	 * Weapon State
	 * ========================================================= */

	UFUNCTION(BlueprintPure, Category = "FM|Weapon")
	EFMWeaponState GetWeaponState() const
	{
		return WeaponState;
	}

	UFUNCTION(BlueprintPure, Category = "FM|Weapon")
	bool IsFiring() const
	{
		return WeaponState == EFMWeaponState::Firing;
	}

	UFUNCTION(BlueprintPure, Category = "FM|Weapon")
	bool IsReloading() const
	{
		return WeaponState == EFMWeaponState::Reloading;
	}

	/* =========================================================
	 * Fire
	 * ========================================================= */

	UFUNCTION(BlueprintCallable, Category = "FM|Weapon")
	void StartFire();

	UFUNCTION(BlueprintCallable, Category = "FM|Weapon")
	void StopFire();

	/* =========================================================
	 * Reload
	 * ========================================================= */

	UFUNCTION(BlueprintCallable, Category = "FM|Weapon")
	void StartReload();

	/* =========================================================
	 * Visibility
	 * ========================================================= */

	UFUNCTION(BlueprintCallable, Category = "FM|Weapon")
	void SetWeaponVisible(bool bVisible);

protected:
	virtual void BeginPlay() override;

	/* =========================================================
	 * Weapon Data
	 * ========================================================= */

	/**
	 * 지정된 DataTable과 RowName을 사용해 무기 설정을 초기화한다.
	 *
	 * 성공하면 true를 반환하며, 실패 시 기존 C++/BP 기본값을 유지한다.
	 */
	bool InitializeFromWeaponData();

	/* =========================================================
	 * Server RPC
	 * ========================================================= */

	UFUNCTION(Server, Reliable)
	void ServerStartFire();

	UFUNCTION(Server, Reliable)
	void ServerStopFire();

	UFUNCTION(Server, Reliable)
	void ServerStartReload();

	/* =========================================================
	 * Internal Fire
	 * ========================================================= */

	void StartFireInternal();
	void StopFireInternal();

	void FireOnce();
	bool CanFire() const;

	void PerformHitScan();
	void ScheduleNextShot();

	/* =========================================================
	 * Internal Reload
	 * ========================================================= */

	void StartReloadInternal();
	void FinishReload();

	/* =========================================================
	 * Replication
	 * ========================================================= */

	UFUNCTION()
	void OnRep_CurrentAmmo();

	UFUNCTION()
	void OnRep_ReserveAmmo();

	UFUNCTION()
	void OnRep_WeaponState();

	/* =========================================================
	 * Components
	 * ========================================================= */

	UPROPERTY(
		VisibleAnywhere,
		BlueprintReadOnly,
		Category = "FM|Components"
	)
	TObjectPtr<USceneComponent> SceneRoot;

	/* =========================================================
	 * DataTable
	 * ========================================================= */

	/**
	 * 무기 설정을 읽어올 데이터테이블.
	 */
	UPROPERTY(
		EditDefaultsOnly,
		BlueprintReadOnly,
		Category = "FM|Weapon|Data"
	)
	TObjectPtr<UDataTable> WeaponDataTable;

	/**
	 * WeaponDataTable에서 사용할 행 이름.
	 *
	 * 예:
	 * M4
	 * Pistol
	 * Knife
	 */
	UPROPERTY(
		EditDefaultsOnly,
		BlueprintReadOnly,
		Category = "FM|Weapon|Data"
	)
	FName WeaponRowName = NAME_None;

	/* =========================================================
	 * Weapon Definition
	 * ========================================================= */

	UPROPERTY(
		VisibleInstanceOnly,
		BlueprintReadOnly,
		Category = "FM|Weapon"
	)
	FName WeaponID = NAME_None;

	UPROPERTY(
		VisibleInstanceOnly,
		BlueprintReadOnly,
		Category = "FM|Weapon"
	)
	EFMWeaponSlot WeaponSlot = EFMWeaponSlot::Primary;

	UPROPERTY(
		VisibleInstanceOnly,
		BlueprintReadOnly,
		Category = "FM|Weapon|Attachment"
	)
	FName FirstPersonSocketName = TEXT("Grip_M4");

	UPROPERTY(
		VisibleInstanceOnly,
		BlueprintReadOnly,
		Category = "FM|Weapon|Attachment"
	)
	FName ThirdPersonSocketName = TEXT("WeaponSocket_R");

	/* =========================================================
	 * Fire Settings
	 * ========================================================= */

	/**
	 * 한 발당 피해량.
	 */
	UPROPERTY(
		VisibleInstanceOnly,
		BlueprintReadOnly,
		Category = "FM|Weapon|Fire"
	)
	float Damage = 30.0f;

	/**
	 * 발사 사거리.
	 */
	UPROPERTY(
		VisibleInstanceOnly,
		BlueprintReadOnly,
		Category = "FM|Weapon|Fire"
	)
	float FireRange = 10000.0f;

	/**
	 * 다음 발사까지 필요한 시간.
	 *
	 * 예:
	 * 0.1초 = 약 600 RPM
	 */
	UPROPERTY(
		VisibleInstanceOnly,
		BlueprintReadOnly,
		Category = "FM|Weapon|Fire"
	)
	float FireInterval = 0.1f;

	UPROPERTY(
		VisibleInstanceOnly,
		BlueprintReadOnly,
		Category = "FM|Weapon|Fire"
	)
	EFMFireMode FireMode = EFMFireMode::FullAuto;

	/**
	 * DamageType은 현재 DataTable이 아니라 무기 BP에서 설정한다.
	 */
	UPROPERTY(
		EditDefaultsOnly,
		BlueprintReadOnly,
		Category = "FM|Weapon|Fire"
	)
	TSubclassOf<UDamageType> DamageTypeClass;

	/**
	 * LineTrace 채널은 현재 무기 BP에서 설정한다.
	 */
	UPROPERTY(
		EditDefaultsOnly,
		BlueprintReadOnly,
		Category = "FM|Weapon|Fire"
	)
	TEnumAsByte<ECollisionChannel> FireTraceChannel =
		ECC_Visibility;

	/* =========================================================
	 * Ammo Settings
	 * ========================================================= */

	UPROPERTY(
		VisibleInstanceOnly,
		BlueprintReadOnly,
		Category = "FM|Weapon|Ammo"
	)
	int32 MagazineSize = 30;

	UPROPERTY(
		VisibleInstanceOnly,
		BlueprintReadOnly,
		Category = "FM|Weapon|Ammo"
	)
	int32 InitialReserveAmmo = 90;

	UPROPERTY(
		ReplicatedUsing = OnRep_CurrentAmmo,
		VisibleInstanceOnly,
		BlueprintReadOnly,
		Category = "FM|Weapon|Ammo"
	)
	int32 CurrentAmmo = 0;

	UPROPERTY(
		ReplicatedUsing = OnRep_ReserveAmmo,
		VisibleInstanceOnly,
		BlueprintReadOnly,
		Category = "FM|Weapon|Ammo"
	)
	int32 ReserveAmmo = 0;

	/* =========================================================
	 * Reload Settings
	 * ========================================================= */

	UPROPERTY(
		VisibleInstanceOnly,
		BlueprintReadOnly,
		Category = "FM|Weapon|Reload"
	)
	float ReloadDuration = 2.0f;

	/* =========================================================
	 * Runtime State
	 * ========================================================= */

	UPROPERTY(
		ReplicatedUsing = OnRep_WeaponState,
		VisibleInstanceOnly,
		BlueprintReadOnly,
		Category = "FM|Weapon"
	)
	EFMWeaponState WeaponState = EFMWeaponState::Idle;

	bool bWantsToFire = false;

	float LastFireTime = -1000.0f;

	FTimerHandle FireTimerHandle;
	FTimerHandle ReloadTimerHandle;
};