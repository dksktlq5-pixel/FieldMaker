#pragma once
#include "GameFramework/Character.h"
#include "FMCombatData.h"
#include "FMWeaponVisualData.h"
#include "FMCharacter.generated.h"

struct FInputActionValue;
struct FFMGripTargets;

UENUM()
enum class EFMWeaponMotion : uint8
{
	Ready, Equip, Fire, Reload, Melee, Throw
};

USTRUCT()
struct FFMWeaponMotionState
{
	GENERATED_BODY()

	UPROPERTY()
	EFMWeaponMotion Motion = EFMWeaponMotion::Ready;

	UPROPERTY()
	double StartTime = 0.0;
};

UENUM(BlueprintType)
enum class EFMWeaponSlot : uint8
{
 Primary, Secondary, Melee, Throwable
};

UCLASS()
class FIELDMAKER_API AFMCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	AFMCharacter(const FObjectInitializer& ObjectInitializer);
	static constexpr float MaxHealth = 100.f;
	UFUNCTION(BlueprintPure, Category = "Combat")
	float GetHealth() const { return Health; }
	UFUNCTION(BlueprintPure, Category="Combat")
	float GetArmor() const { return Armor; }
	UFUNCTION(BlueprintPure, Category="Combat")
	bool HasArmor() const { return Armor > 0.f; }
	UFUNCTION(BlueprintPure, Category="Combat")
	FFMArmorRow GetArmorRules() const;
 UFUNCTION(BlueprintPure, Category="Combat") FFMArmorRow GetCatalogArmorRules(FName Id) const;
 UFUNCTION(BlueprintPure, Category="Combat") FName GetArmorId() const {return ArmorId;}
 UFUNCTION(Server, Reliable) void ServerEquipTestArmor(FName Id);
	UFUNCTION(BlueprintPure, Category="Combat")
	FFMWeaponRow GetWeaponData(EFMWeaponSlot Slot) const;
	UFUNCTION(BlueprintPure, Category="Combat") float GetCrosshairSpreadDegrees() const;
 UFUNCTION(BlueprintPure, Category="Presentation") FFMWeaponVisualRow GetWeaponVisualData() const;
 UFUNCTION(BlueprintPure, Category="Presentation") bool IsScopeViewActive() const;
 UFUNCTION(BlueprintPure, Category="Presentation") FFMWeaponVisualRow GetCatalogWeaponVisual(FName Id) const;
 UFUNCTION(BlueprintPure, Category="Catalog") TArray<FName> GetCatalogWeaponIds() const;
 UFUNCTION(BlueprintPure, Category="Catalog") FFMWeaponRow GetCatalogWeaponData(FName Id) const;
 UFUNCTION(BlueprintPure, Category="Catalog") FName GetWeaponId(EFMWeaponSlot Slot) const;
 UFUNCTION(Server, Reliable) void ServerSelectCatalogWeapon(FName Id);
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category="Combat")
	void RefillArmor();
	UFUNCTION(Server, Reliable)
	void ServerToggleTestArmor();
	UFUNCTION(BlueprintPure, Category = "Combat")
	EFMWeaponSlot GetWeaponSlot() const { return WeaponSlot; }
	UFUNCTION(BlueprintPure, Category = "Combat")
	int32 GetLoadedAmmo() const;
	UFUNCTION(BlueprintPure, Category = "Combat")
	int32 GetReserveAmmo() const;
	UFUNCTION(BlueprintPure, Category = "Combat")
	int32 GetGrenadeCount() const { return Grenades; }
	void ResetForRound(bool bResetEquipment);
	bool CanPurchase() const;
	UFUNCTION(Server, Reliable) void ServerBuyStructure(int32 Index);
	UFUNCTION(Server, Reliable) void ServerPlaceStructure(int32 Index, int32 QuarterTurns);
	bool FindStructurePlacement(int32 Index, int32 QuarterTurns, FTransform& Transform, class AFMStructure*& Support, const TCHAR** Reason = nullptr) const;
	virtual float TakeDamage(float Damage, const FDamageEvent& Event, AController* InstigatorController, AActor* Causer) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UFUNCTION(Server, Reliable)
	void ServerEquipWeapon(EFMWeaponSlot Slot);

	UFUNCTION(Server, Reliable)
	void ServerUseWeapon();

	UFUNCTION(Server, Reliable)
	void ServerReload();

	UFUNCTION(Server, Reliable)
	void ServerStartFire();

	UFUNCTION(Server, Reliable)
	void ServerStopFire();

	UFUNCTION(BlueprintPure, Category = "Combat")
	bool IsReloading() const { return WeaponMotion.Motion == EFMWeaponMotion::Reload; }
	UFUNCTION(BlueprintPure, Category="Combat")
	bool IsAiming() const { return bAiming; }
	UFUNCTION(BlueprintPure, Category="Combat")
	bool IsSprinting() const { return bSprinting; }
	UFUNCTION(BlueprintPure, Category="Combat")
	bool CanAttack() const;
	UFUNCTION(BlueprintPure, Category="Combat")
	bool CanAim() const;
	UFUNCTION(Server, Reliable)
	void ServerSetAiming(bool bNewAiming);
	UFUNCTION(Server, Reliable)
	void ServerSetSprint(bool bSprint);

	bool GetWeaponGripTargets(const USkeletalMeshComponent& PoseMesh, FFMGripTargets& Targets) const;
	void ApplyAppearance(uint32 AppearanceId);
	virtual void OnStartCrouch(float HalfHeightAdjust, float ScaledHalfHeightAdjust) override;
	virtual void OnEndCrouch(float HalfHeightAdjust, float ScaledHalfHeightAdjust) override;

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void OnRep_PlayerState() override;
	virtual void UnPossessed() override;
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;

private:
	UPROPERTY(VisibleAnywhere, Category = "Camera")
	TObjectPtr<class UCameraComponent> FirstPersonCamera;

	UPROPERTY(VisibleAnywhere, Category = "Camera")
	TObjectPtr<class USkeletalMeshComponent> FirstPersonArms;

	UPROPERTY(VisibleAnywhere, Category = "Appearance")
	TArray<TObjectPtr<class USkeletalMeshComponent>> OutfitParts;

	UPROPERTY(VisibleAnywhere, Category = "Appearance")
	TObjectPtr<class UStaticMeshComponent> OutfitCap;

	UPROPERTY(Replicated)
	float Health = MaxHealth;
	UPROPERTY(Replicated)
	float Armor = 0.f;
 UPROPERTY(Replicated) FName ArmorId = TEXT("LightArmor");
	UPROPERTY()
	TObjectPtr<UDataTable> WeaponDataTable;
	UPROPERTY()
	TObjectPtr<UDataTable> ArmorDataTable;
 UPROPERTY() TObjectPtr<UDataTable> WeaponVisualDataTable;
	const FFMWeaponVisualRow& GetActiveWeaponVisualRow() const;
	static EFMHitZone HitZoneForBone(FName Bone);
	UPROPERTY(ReplicatedUsing=OnRep_CombatStance)
	bool bSprinting = false;
	bool bSprintInputHeld = false;
	UPROPERTY(ReplicatedUsing=OnRep_CombatStance)
	bool bAiming = false;
	UFUNCTION()
	void OnRep_CombatStance();
	bool IsCombatPhaseAllowed(bool bForAttack = true) const;
	void UpdateStanceSpeed();
	float ADSAlpha = 0.f;
	float MoveBobAlpha = 0.f;
	float SprintPoseAlpha = 0.f;

	UPROPERTY(ReplicatedUsing = OnRep_WeaponSlot) FName PrimaryWeaponId = TEXT("M4");
	UPROPERTY(ReplicatedUsing = OnRep_WeaponSlot) FName SecondaryWeaponId = TEXT("Pistol");
	UPROPERTY(ReplicatedUsing = OnRep_WeaponSlot)
	EFMWeaponSlot WeaponSlot = EFMWeaponSlot::Primary;

	UPROPERTY(Replicated)
	int32 PrimaryAmmo = 30;
	UPROPERTY(Replicated)
	int32 PrimaryReserve = 90;
	UPROPERTY(Replicated)
	int32 SecondaryAmmo = 12;
	UPROPERTY(Replicated)
	int32 SecondaryReserve = 36;
	UPROPERTY(Replicated)
	int32 Grenades = 2;

	UPROPERTY(Replicated)
	FFMWeaponMotionState WeaponMotion;

	UPROPERTY()
	TArray<TObjectPtr<class UStaticMesh>> WeaponModels;
	UPROPERTY()
	TArray<TObjectPtr<UStaticMesh>> MagazineModels;

	UPROPERTY()
	TObjectPtr<UStaticMeshComponent> ViewWeapon;
	UPROPERTY()
	TObjectPtr<UStaticMeshComponent> WorldWeapon;
	UPROPERTY()
	TObjectPtr<UStaticMeshComponent> ViewMagazine;
	UPROPERTY()
	TObjectPtr<UStaticMeshComponent> WorldMagazine;
 UPROPERTY() TObjectPtr<UStaticMeshComponent> ViewAccessory;
 UPROPERTY() TObjectPtr<UStaticMeshComponent> WorldAccessory;
	UPROPERTY()
	TObjectPtr<UStaticMeshComponent> ViewFlash;
	UPROPERTY()
	TObjectPtr<UStaticMeshComponent> WorldFlash;

	FTimerHandle ReloadTimer;
	FTimerHandle AutoFireTimer;
	FTimerHandle ThrowTimer;
	double NextAttackTime = 0.0;
	float MoveBobTime = 0.f;
	FTransform ViewMeleeMotion;
	FTransform WorldMeleeMotion;
	double LastShotTime = -1.0;
	double LastBallisticShotTime = -1.0;
	FName LastBallisticWeapon;
	int32 BurstShotIndex = 0;
	int32 LocalBurstShotIndex = 0;
	float CalculateShotSpread(const FFMWeaponRow& Data, int32 ShotIndex) const;
	FRotator CameraRecoil = FRotator::ZeroRotator;
	EFMWeaponSlot LastShotSlot = EFMWeaponSlot::Primary;

	float GetEquipDuration() const { return GetWeaponData(WeaponSlot).EquipTime; }
	static constexpr float ThrowReleaseTime = 0.23f;
	float GetReloadDuration() const { return GetWeaponData(WeaponSlot).ReloadTime; }

	void ReleaseGrenade();
	void CancelWeaponActions();
	void FinishReload();
	void CancelReload();
	void RepeatFire();
	void SetWeaponMotion(EFMWeaponMotion Motion);
	double GetWeaponTime() const;
	float GetMotionAge() const;
	void InitializeWeaponPresentation();
	void UpdateWeaponPresentation(float DeltaSeconds);

	UFUNCTION()
	void OnRep_WeaponSlot();

	UFUNCTION(NetMulticast, Unreliable)
	void MulticastWeaponEffects(EFMWeaponMotion Motion, EFMWeaponSlot Slot, FVector2D CameraKick, int32 ShotIndex);

	UPROPERTY()
	TObjectPtr<class USoundBase> RifleShot;
 UPROPERTY() TObjectPtr<class USoundAttenuation> FireAttenuation;
 UPROPERTY() TObjectPtr<USoundBase> ShotgunShot;
 UPROPERTY() TObjectPtr<USoundBase> PistolReloadOut;
 UPROPERTY() TObjectPtr<USoundBase> ShotgunReloadOut;
 UPROPERTY() TObjectPtr<USoundBase> RifleReloadIn;
 UPROPERTY() TObjectPtr<USoundBase> PistolReloadIn;
 UPROPERTY() TObjectPtr<USoundBase> ShotgunReloadIn;
 FTimerHandle ReloadSoundTimer;
 void PlayReloadInsert();
 UFUNCTION(BlueprintPure, Category="Audio") USoundBase* GetCurrentFireSound() const;
	UPROPERTY()
	TObjectPtr<USoundBase> PistolShot;
	UPROPERTY()
	TObjectPtr<USoundBase> ReloadSound;
	UPROPERTY()
	TObjectPtr<USoundBase> MeleeSound;
	UPROPERTY()
	TObjectPtr<USoundBase> ThrowSound;

	void RefreshAppearance();
	void Move(const FInputActionValue& Value);
	void Look(const FInputActionValue& Value);
	void ToggleCrouch();
	void StartSprint();
	void StopSprint();
	void SetSprint(bool bSprint);

};
