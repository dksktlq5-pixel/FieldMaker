#include "FMCharacter.h"
#include "Materials/MaterialInterface.h"
#include "FMPlayerState.h"
#include "FMGameState.h"
#include "FMGrenade.h"
#include "FMWeaponPoseMesh.h"
#include "TimerManager.h"
#include "Kismet/GameplayStatics.h"
#include "Net/UnrealNetwork.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/DamageEvents.h"
#include "DrawDebugHelpers.h"
#include "HAL/IConsoleManager.h"
#include "Misc/PackageName.h"
#include "FMUserPreferences.h"
#include "Animation/AnimSequence.h"
#include "Camera/CameraComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/SkeletalMesh.h"
#include "UObject/ConstructorHelpers.h"
#include "Components/CapsuleComponent.h"
#include "EnhancedInputComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "InputAction.h"
#include "InputActionValue.h"

namespace
{
#if !UE_BUILD_SHIPPING
TAutoConsoleVariable<int32> CVarDrawImpacts(TEXT("fm.DrawImpacts"), 0,
	TEXT("Draw authoritative firearm impacts for five seconds."));
#endif

constexpr float WalkSpeed = 250.0f;
constexpr float SprintSpeed = 550.0f;

float WeaponMovementScale(EFMWeaponSlot Slot)
{
	switch (Slot)
	{
	case EFMWeaponSlot::Melee: return 1.28f;
	case EFMWeaponSlot::Secondary:
	case EFMWeaponSlot::Throwable: return 1.10f;
	default: return 1.f;
	}
}
constexpr float CameraHeight = 64.0f;
constexpr float CrouchedCapsuleHalfHeight = 60.0f;
constexpr float CrouchedEyeClearance = 8.0f;

const UInputAction* LoadAction(const TCHAR* Name)
{
	const FString Path = FString::Printf(TEXT("/Game/Input/%s.%s"), Name, Name);
	return LoadObject<UInputAction>(nullptr, *Path);
}
}

AFMCharacter::AFMCharacter(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer.SetDefaultSubobjectClass<UFMWeaponPoseMesh>(ACharacter::MeshComponentName))
{
	PrimaryActorTick.bCanEverTick = true;
	// Bound movement replication work as the listen server renders at a higher frame rate.
	SetNetUpdateFrequency(60.f);
	bUseControllerRotationYaw = true;
	FirstPersonCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FirstPersonCamera"));
	FirstPersonCamera->SetupAttachment(GetCapsuleComponent());
	FirstPersonCamera->SetRelativeLocation(FVector(0, 0, CameraHeight));
	FirstPersonCamera->bUsePawnControlRotation = true;

	USkeletalMeshComponent* Body = GetMesh();
	Body->SetRelativeLocation(FVector(0.0, 0.0, -GetCapsuleComponent()->GetUnscaledCapsuleHalfHeight()));
	Body->SetRelativeRotation(FRotator(0.0, -90.0, 0.0));
	Body->SetOwnerNoSee(true);
	// Use the fixed base body's physics asset for bullets, never cosmetic modules.
	Body->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	Body->SetCollisionResponseToAllChannels(ECR_Ignore);
	Body->SetCollisionResponseToChannel(ECC_GameTraceChannel1, ECR_Block);
	Body->SetGenerateOverlapEvents(false);
	GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
	GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_GameTraceChannel1, ECR_Ignore);
	// Modular outfits render instead of this leader mesh, so render visibility cannot gate its pose.
	Body->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones;

	const TCHAR* PartNames[] = {
		TEXT("Head"), TEXT("Arms"), TEXT("Jeans"), TEXT("Shirt_RolledUp_Blue"),
		TEXT("Bulletproof_Bege"), TEXT("Drops_1_Bege"), TEXT("Holster_Hard_Bege"), TEXT("Patch_Back")
	};
	for (const TCHAR* PartName : PartNames)
	{
		auto* Part = CreateDefaultSubobject<USkeletalMeshComponent>(FName(FString(TEXT("Outfit_")) + PartName));
		Part->SetupAttachment(Body);
		Part->SetOwnerNoSee(true);
		Part->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Part->SetGenerateOverlapEvents(false);
		Part->bUseAttachParentBound = true;
		const FString Path = FString::Printf(TEXT("/Game/QuantumCharacter/Mesh/Modules/SKM_%s.SKM_%s"), PartName, PartName);
		Part->SetSkeletalMeshAsset(LoadObject<USkeletalMesh>(nullptr, *Path));
		OutfitParts.Add(Part);
	}
	OutfitCap = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Outfit_Cap"));
	OutfitCap->SetupAttachment(Body, TEXT("head"));
	OutfitCap->SetOwnerNoSee(true);
	OutfitCap->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	OutfitCap->SetGenerateOverlapEvents(false);
	static ConstructorHelpers::FObjectFinder<UStaticMesh> CapMesh(
		TEXT("/Game/QuantumCharacter/Mesh/Modules/SM_Cap_Bege.SM_Cap_Bege"));
	OutfitCap->SetStaticMesh(CapMesh.Object);

	FirstPersonArms = CreateDefaultSubobject<UFMWeaponPoseMesh>(TEXT("FirstPersonArms"));
	FirstPersonArms->PrimaryComponentTick.bStartWithTickEnabled = false;
	FirstPersonArms->SetupAttachment(FirstPersonCamera);
	FirstPersonArms->SetRelativeLocation(FVector(8.0, 0.0, -170.0));
	FirstPersonArms->SetRelativeRotation(FRotator(0.0, -90.0, 0.0));
	// The imported mesh bounds describe the reference pose, below the animated hands.
	FirstPersonArms->SetBoundsScale(2.0f);
 FirstPersonArms->VisibilityBasedAnimTickOption=EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones;
	FirstPersonArms->SetOnlyOwnerSee(true);
	FirstPersonArms->SetCastShadow(false);
	FirstPersonArms->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	FirstPersonArms->SetGenerateOverlapEvents(false);

	static ConstructorHelpers::FObjectFinder<USkeletalMesh> BodyMesh(
		TEXT("/Game/QuantumCharacter/Mesh/SKM_QuantumCharacter.SKM_QuantumCharacter"));
	static ConstructorHelpers::FObjectFinder<USkeletalMesh> ArmsMesh(
		TEXT("/Game/FieldMaker/Character/SKM_FMFirstPersonArms.SKM_FMFirstPersonArms"));
	static ConstructorHelpers::FObjectFinder<UAnimSequence> ArmsIdle(
		TEXT("/Game/QuantumCharacter/Demo/Animations/A_MM_Idle.A_MM_Idle"));
	Body->SetSkeletalMeshAsset(BodyMesh.Object);
	if (BodyMesh.Object && Body->GetBoneIndex(TEXT("head")) != INDEX_NONE)
	{
		// The cap was modeled in mesh space, while the head bone has rotated local axes.
		const FMatrix HeadReferencePose = BodyMesh.Object->GetComposedRefPoseMatrix(TEXT("head"));
		OutfitCap->SetRelativeRotation(FQuat(HeadReferencePose).Inverse());
	}
	FirstPersonArms->SetSkeletalMeshAsset(ArmsMesh.Object);
	FirstPersonArms->SetAnimationMode(EAnimationMode::AnimationSingleNode);
	FirstPersonArms->AnimationData.AnimToPlay = ArmsIdle.Object;
	FirstPersonArms->AnimationData.bSavedLooping = true;
	FirstPersonArms->AnimationData.bSavedPlaying = true;

	InitializeWeaponPresentation();

	auto* Movement = GetCharacterMovement();
	Movement->GetNavAgentPropertiesRef().bCanCrouch = true;
	Movement->SetCrouchedHalfHeight(CrouchedCapsuleHalfHeight);
	Movement->MaxWalkSpeed = WalkSpeed;
}

void AFMCharacter::BeginPlay()
{
	Super::BeginPlay();
	if (FPackageName::DoesPackageExist(TEXT("/Game/FieldMaker/Data/DT_FMWeapons")))
		WeaponDataTable = LoadObject<UDataTable>(nullptr,FMCatalog::Weapons);
	if (FPackageName::DoesPackageExist(TEXT("/Game/FieldMaker/Data/DT_FMArmor")))
		ArmorDataTable = LoadObject<UDataTable>(nullptr,TEXT("/Game/FieldMaker/Data/DT_FMArmor.DT_FMArmor"));
	if (FPackageName::DoesPackageExist(TEXT("/Game/FieldMaker/Data/DT_FMWeaponVisuals")))
  WeaponVisualDataTable = LoadObject<UDataTable>(nullptr,FMCatalog::Visuals);
	if (HasAuthority())
	{
		const FFMWeaponRow Primary = GetWeaponData(EFMWeaponSlot::Primary);
		const FFMWeaponRow Secondary = GetWeaponData(EFMWeaponSlot::Secondary);
		PrimaryAmmo = Primary.MagazineSize; PrimaryReserve = Primary.ReserveAmmo;
		SecondaryAmmo = Secondary.MagazineSize; SecondaryReserve = Secondary.ReserveAmmo;
		Grenades = GetWeaponData(EFMWeaponSlot::Throwable).MagazineSize;
	}

	if (UClass* LocomotionClass = LoadClass<UAnimInstance>(nullptr,
		TEXT("/Game/FieldMaker/Character/ABP_FMLocomotion.ABP_FMLocomotion_C")))
	{
		GetMesh()->SetAnimInstanceClass(LocomotionClass);
	}
	RefreshAppearance();
	OnRep_WeaponSlot();
	UpdateWeaponPresentation(0.f);
}

void AFMCharacter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (HasAuthority())
	{
		if (!IsCombatPhaseAllowed() || Health <= 0.f)
		{
			CancelWeaponActions();
			bAiming = false;
		}
		if (Health <= 0.f) bSprinting = false;
		UpdateStanceSpeed();
	}
	const bool bLocalView = IsLocallyControlled() && GetNetMode() != NM_DedicatedServer;
	const bool bTickViewArms = bLocalView && Health > 0.f;
	if (FirstPersonArms->IsComponentTickEnabled() != bTickViewArms)
	{
		FirstPersonArms->SetComponentTickEnabled(bTickViewArms);
	}
	if (bLocalView)
	{
		ADSAlpha = FMath::FInterpTo(ADSAlpha, bAiming ? 1.f : 0.f, DeltaSeconds, 14.f);
		FirstPersonCamera->SetFieldOfView(FMath::Lerp(90.f, GetWeaponData(WeaponSlot).AimFOV, ADSAlpha));
		CameraRecoil = Health > 0.f ? FMath::RInterpTo(CameraRecoil, FRotator::ZeroRotator, DeltaSeconds, 15.f) : FRotator::ZeroRotator;
		FirstPersonCamera->ClearAdditiveOffset();
		if (!CameraRecoil.IsNearlyZero()) FirstPersonCamera->AddAdditiveOffset(FTransform(CameraRecoil), 0.f);
	}
	if (GetNetMode() != NM_DedicatedServer && GetPlayerState())
	{
		UpdateWeaponPresentation(DeltaSeconds);
	}
}

void AFMCharacter::OnRep_PlayerState()
{
	Super::OnRep_PlayerState();
	RefreshAppearance();
}

void AFMCharacter::RefreshAppearance()
{
	const auto* State = GetPlayerState<AFMPlayerState>();
	ApplyAppearance(State ? State->GetAppearanceId() : FMAppearance::Default);
}

void AFMCharacter::ApplyAppearance(uint32 AppearanceId)
{
	if (!FMAppearance::IsValid(AppearanceId))
	{
		return;
	}

	USkeletalMeshComponent* Body = GetMesh();
	const USkeletalMesh* BodyAsset = Body->GetSkeletalMeshAsset();
	const bool bModularReady = BodyAsset && OutfitParts.Num() == 8 && !OutfitParts.ContainsByPredicate(
		[BodyAsset](const USkeletalMeshComponent* Part)
		{
			const USkeletalMesh* Asset = Part ? Part->GetSkeletalMeshAsset() : nullptr;
			return !Asset || Asset->GetSkeleton() != BodyAsset->GetSkeleton();
		});
	Body->SetVisibility(!bModularReady);

	const uint32 OptionalParts[] = {FMAppearance::Vest, FMAppearance::Drops, FMAppearance::Holster, FMAppearance::Patch};
	for (int32 Index = 0; Index < OutfitParts.Num(); ++Index)
	{
		auto* Part = OutfitParts[Index].Get();
		if (Part)
		{
			if (bModularReady)
			{
				Part->SetLeaderPoseComponent(Body);
			}
			Part->SetVisibility(bModularReady && (Index < 4 || (AppearanceId & OptionalParts[Index - 4])));
		}
	}
	OutfitCap->SetVisibility(bModularReady && Body->DoesSocketExist(TEXT("head")) && (AppearanceId & FMAppearance::Cap));
 const auto StyleMaterial=[&](const TCHAR* PartName,int32 Category)->UMaterialInterface*
 {
  const FString Name=FString::Printf(TEXT("MI_FM_%s_%d"),PartName,FMAppearance::GetStyle(AppearanceId,Category));
  return LoadObject<UMaterialInterface>(nullptr,*(TEXT("/Game/FieldMaker/Character/Styles/")+Name+TEXT(".")+Name));
 };
 if(auto* M=StyleMaterial(TEXT("Cap"),0))OutfitCap->SetMaterial(0,M);
 if(OutfitParts.Num()==8)
 {
  if(auto* M=StyleMaterial(TEXT("Shirt"),2))
  {
   OutfitParts[3]->SetMaterial(0,M);
   const auto Slots=FirstPersonArms->GetMaterialSlotNames();
   for(int32 I=0;I<Slots.Num();++I)if((Slots[I].ToString().Contains(TEXT("Shirt")) || Slots[I].ToString().Contains(TEXT("Sleeve"))))FirstPersonArms->SetMaterial(I,M);
  }
  if(auto* M=StyleMaterial(TEXT("Jeans"),6))OutfitParts[2]->SetMaterial(0,M);
  if(auto* M=StyleMaterial(TEXT("Vest"),3))OutfitParts[4]->SetMaterial(0,M);
 }

}

void AFMCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	auto* Input = Cast<UEnhancedInputComponent>(PlayerInputComponent);
	if (!Input)
	{
		return;
	}

	if (const auto* Action = LoadAction(TEXT("IA_FMMove")))
	{
		Input->BindAction(Action, ETriggerEvent::Triggered, this, &AFMCharacter::Move);
	}
	if (const auto* Action = LoadAction(TEXT("IA_FMLook")))
	{
		Input->BindAction(Action, ETriggerEvent::Triggered, this, &AFMCharacter::Look);
	}
	if (const auto* Action = LoadAction(TEXT("IA_FMJump")))
	{
		Input->BindAction(Action, ETriggerEvent::Started, this, &AFMCharacter::Jump);
		Input->BindAction(Action, ETriggerEvent::Completed, this, &AFMCharacter::StopJumping);
		Input->BindAction(Action, ETriggerEvent::Canceled, this, &AFMCharacter::StopJumping);
	}
	if (const auto* Action = LoadAction(TEXT("IA_FMSprint")))
	{
		Input->BindAction(Action, ETriggerEvent::Triggered, this, &AFMCharacter::StartSprint);
		Input->BindAction(Action, ETriggerEvent::Completed, this, &AFMCharacter::StopSprint);
		Input->BindAction(Action, ETriggerEvent::Canceled, this, &AFMCharacter::StopSprint);
	}
	if (const auto* Action = LoadAction(TEXT("IA_FMCrouch")))
	{
		Input->BindAction(Action, ETriggerEvent::Started, this, &AFMCharacter::ToggleCrouch);
	}
}

void AFMCharacter::Move(const FInputActionValue& Value)
{
	const FVector2D Axis = Value.Get<FVector2D>();
	AddMovementInput(GetActorForwardVector(), Axis.Y);
	AddMovementInput(GetActorRightVector(), Axis.X);
}

void AFMCharacter::Look(const FInputActionValue& Value)
{
	const FVector2D Axis = Value.Get<FVector2D>();
	const FMLocalPreferences& Preferences = FMLocalPreferences::Get();
	const float AimSensitivity = FMath::Lerp(1.f, Preferences.ADSSensitivity, ADSAlpha);
	AddControllerYawInput(Axis.X * Preferences.Sensitivity * AimSensitivity);
	AddControllerPitchInput(Axis.Y * Preferences.Sensitivity * AimSensitivity * (Preferences.bInvertY ? -1.0f : 1.0f));
}

void AFMCharacter::ToggleCrouch()
{
	if (GetCharacterMovement()->bWantsToCrouch)
	{
		UnCrouch();
	}
	else
	{
		Crouch();
	}
}

void AFMCharacter::OnStartCrouch(float HalfHeightAdjust, float ScaledHalfHeightAdjust)
{
	Super::OnStartCrouch(HalfHeightAdjust, ScaledHalfHeightAdjust);
	FirstPersonCamera->SetRelativeLocation(FVector(0, 0, FMath::Min(CameraHeight, GetCapsuleComponent()->GetUnscaledCapsuleHalfHeight() - CrouchedEyeClearance)));
}

void AFMCharacter::OnEndCrouch(float HalfHeightAdjust, float ScaledHalfHeightAdjust)
{
	Super::OnEndCrouch(HalfHeightAdjust, ScaledHalfHeightAdjust);
	FirstPersonCamera->SetRelativeLocation(FVector(0, 0, CameraHeight));
}

void AFMCharacter::StartSprint()
{
	if (!bSprintInputHeld)
	{
		bSprintInputHeld = true;
		SetSprint(true);
	}
}

void AFMCharacter::StopSprint()
{
	bSprintInputHeld = false;
	SetSprint(false);
}

void AFMCharacter::SetSprint(bool bSprint)
{
	ServerSetSprint(bSprint);
}

void AFMCharacter::ServerSetSprint_Implementation(bool bSprint)
{
	bSprinting = bSprint && Health > 0.f;
	if (bSprinting)
	{
		bAiming = false;
		if (WeaponSlot <= EFMWeaponSlot::Secondary) CancelReload();
		else CancelWeaponActions();
	}
	UpdateStanceSpeed();
	ForceNetUpdate();
}

void AFMCharacter::UpdateStanceSpeed()
{
	GetCharacterMovement()->MaxWalkSpeed = (bSprinting ? SprintSpeed : bAiming ? 180.f : WalkSpeed) * WeaponMovementScale(WeaponSlot);
}

void AFMCharacter::OnRep_CombatStance() { UpdateStanceSpeed(); }

bool AFMCharacter::IsCombatPhaseAllowed(bool bForAttack) const
{
	const auto* State = GetWorld()->GetGameState<AFMGameState>();
	if (!State) return false;
	switch (State->GetMatchSnapshot().Phase)
	{
	case EFMMatchPhase::Practice: case EFMMatchPhase::CombatTest: return true;
	case EFMMatchPhase::Combat: return State->GetPhaseTimeRemaining() > 0.f;
	case EFMMatchPhase::KnifeCombat: return State->GetPhaseTimeRemaining() > 0.f && (!bForAttack || WeaponSlot == EFMWeaponSlot::Melee);
	default: return false;
	}
}

bool AFMCharacter::CanAttack() const
{
	return Health > 0.f && Controller && (!bSprinting || WeaponSlot <= EFMWeaponSlot::Secondary) && !IsReloading() && IsCombatPhaseAllowed()
		&& !(WeaponMotion.Motion == EFMWeaponMotion::Equip && GetMotionAge() < GetEquipDuration());
}

bool AFMCharacter::CanAim() const
{
	return !bSprinting && CanAttack() && (WeaponSlot == EFMWeaponSlot::Primary || WeaponSlot == EFMWeaponSlot::Secondary);
}

void AFMCharacter::ServerSetAiming_Implementation(bool bNewAiming)
{
	bAiming = bNewAiming && CanAim();
	UpdateStanceSpeed();
	ForceNetUpdate();
}

void AFMCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AFMCharacter, Health);
	DOREPLIFETIME(AFMCharacter, Armor);
 DOREPLIFETIME(AFMCharacter, ArmorId);
	DOREPLIFETIME(AFMCharacter, bSprinting);
	DOREPLIFETIME(AFMCharacter, bAiming);
	DOREPLIFETIME(AFMCharacter, WeaponSlot);
 DOREPLIFETIME(AFMCharacter, PrimaryWeaponId);
 DOREPLIFETIME(AFMCharacter, SecondaryWeaponId);
	DOREPLIFETIME(AFMCharacter, WeaponMotion);
	DOREPLIFETIME_CONDITION(AFMCharacter, PrimaryAmmo, COND_OwnerOnly);
	DOREPLIFETIME_CONDITION(AFMCharacter, PrimaryReserve, COND_OwnerOnly);
	DOREPLIFETIME_CONDITION(AFMCharacter, SecondaryAmmo, COND_OwnerOnly);
	DOREPLIFETIME_CONDITION(AFMCharacter, SecondaryReserve, COND_OwnerOnly);
	DOREPLIFETIME(AFMCharacter, Grenades);
}

int32 AFMCharacter::GetLoadedAmmo() const
{
	switch (WeaponSlot)
	{
	case EFMWeaponSlot::Primary: return PrimaryAmmo;
	case EFMWeaponSlot::Secondary: return SecondaryAmmo;
	case EFMWeaponSlot::Throwable: return Grenades;
	default: return 0;
	}
}

int32 AFMCharacter::GetReserveAmmo() const
{
	switch (WeaponSlot)
	{
	case EFMWeaponSlot::Primary: return PrimaryReserve;
	case EFMWeaponSlot::Secondary: return SecondaryReserve;
	default: return 0;
	}
}

float AFMCharacter::TakeDamage(float Damage, const FDamageEvent& Event, AController* InstigatorController, AActor* Causer)
{
	if (!HasAuthority() || Health <= 0.f || !IsCombatPhaseAllowed(false) || !FMath::IsFinite(Damage) || Damage <= 0.f)
	{
		return 0.f;
	}

	const auto* Victim = GetPlayerState<AFMPlayerState>();
	const auto* Attacker = InstigatorController ? InstigatorController->GetPlayerState<AFMPlayerState>() : nullptr;
	if (Victim && Attacker && Victim != Attacker && Victim->GetTeam() < AFMPlayerState::UnassignedTeam
		&& Victim->GetTeam() == Attacker->GetTeam())
	{
		return 0.f;
	}

	float HealthDamage = Damage;
	if (Event.IsOfType(FPointDamageEvent::ClassID) && Event.DamageTypeClass &&
		Event.DamageTypeClass->IsChildOf(UFMFirearmDamageType::StaticClass()))
	{
		const auto& Point = static_cast<const FPointDamageEvent&>(Event);
		if (HitZoneForBone(Point.HitInfo.BoneName) != EFMHitZone::Head && HasArmor())
		{
			const float Absorbed = FMath::Min(Armor, Damage * GetArmorRules().AbsorptionFraction);
			Armor -= Absorbed;
			HealthDamage -= Absorbed;
		}
	}
	const float Applied = Super::TakeDamage(FMath::Min(HealthDamage, Health), Event, InstigatorController, Causer);
	Health = FMath::Max(0.f, Health - Applied);
	if (Health <= 0.f)
	{
		GetCharacterMovement()->DisableMovement();
		if (auto* State = GetWorld()->GetGameState<AFMGameState>(); State && State->GetMatchSnapshot().bPlayerRoom)
		{
			SetActorEnableCollision(false);
			SetActorHiddenInGame(true);
			if (Attacker && Victim && Attacker != Victim && State->GetMatchSnapshot().Phase == EFMMatchPhase::Combat)
			{
				InstigatorController->GetPlayerState<AFMPlayerState>()->AddCredits(300);
				FString WeaponName = TEXT("ELIM");
				if (const auto* AttackerPawn = Cast<AFMCharacter>(Attacker->GetPawn()))
					WeaponName = AttackerPawn->GetWeaponData(AttackerPawn->GetWeaponSlot()).DisplayName.ToString();
				State->AddKillFeed(Attacker->GetPlayerName(), Victim->GetPlayerName(), WeaponName);
			}
		}
		bSprinting = false;
		bAiming = false;
		CancelWeaponActions();
	}
	ForceNetUpdate();
	return Applied;
}

void AFMCharacter::ServerEquipWeapon_Implementation(EFMWeaponSlot Slot)
{
	if (Health <= 0.f || Slot > EFMWeaponSlot::Throwable || Slot == WeaponSlot)
	{
		return;
	}

	const auto* State = GetWorld()->GetGameState<AFMGameState>();
	if (State && State->GetMatchSnapshot().bPlayerRoom &&
		((Slot == EFMWeaponSlot::Primary && PrimaryWeaponId.IsNone()) ||
		 (Slot == EFMWeaponSlot::Secondary && SecondaryWeaponId.IsNone()) ||
		 (Slot == EFMWeaponSlot::Throwable && Grenades <= 0))) return;

	CancelWeaponActions();
	WeaponSlot = Slot;
	NextAttackTime = FMath::Max(NextAttackTime, GetWorld()->GetTimeSeconds() + GetEquipDuration());
	SetWeaponMotion(EFMWeaponMotion::Equip);
	OnRep_WeaponSlot();
}

float AFMCharacter::CalculateShotSpread(const FFMWeaponRow& Data, int32 ShotIndex) const
{
	const float Speed = GetVelocity().Size2D() / WeaponMovementScale(WeaponSlot);
	const float Movement = FMath::Clamp(Speed / WalkSpeed, 0.f, 1.f);
	const float SprintSpread = bSprinting ? FMath::Clamp((Speed - WalkSpeed) / (SprintSpeed - WalkSpeed), 0.f, 1.f) * 1.5f : 0.f;
	const float ExtraSpread = 0.12f + FMath::Min(ShotIndex, 10) * 0.08f
		+ Movement * 0.75f + SprintSpread + (GetCharacterMovement()->IsFalling() ? 1.4f : 0.f);
	return Data.SpreadDegrees + ExtraSpread * (bAiming ? 0.55f : 1.f);
}

float AFMCharacter::GetCrosshairSpreadDegrees() const
{
	if (WeaponSlot != EFMWeaponSlot::Primary && WeaponSlot != EFMWeaponSlot::Secondary) return 0.f;
	const auto Data = GetWeaponData(WeaponSlot);
	const double Age = GetWorld()->GetTimeSeconds() - LastShotTime;
	const float Hold = FMath::Max(0.30f, Data.AttackInterval * 1.35f);
	const float Recovery = LastShotSlot == WeaponSlot && !IsReloading()
		? 1.f - FMath::Clamp((Age - Hold) / 0.16, 0.0, 1.0) : 0.f;
	return FMath::Lerp(CalculateShotSpread(Data, 0), CalculateShotSpread(Data, LocalBurstShotIndex), Recovery);
}

void AFMCharacter::ServerUseWeapon_Implementation()
{
	const double Now = GetWorld()->GetTimeSeconds();
	if (!CanAttack() || Now + UE_DOUBLE_SMALL_NUMBER < NextAttackTime || (WeaponSlot != EFMWeaponSlot::Melee && GetLoadedAmmo() <= 0))
	{
		return;
	}

	const FVector Start = FirstPersonCamera->GetComponentLocation();
	const FRotator Aim = GetBaseAimRotation();
	if (WeaponSlot == EFMWeaponSlot::Throwable)
	{
		NextAttackTime = Now + GetWeaponData(WeaponSlot).AttackInterval;
		SetWeaponMotion(EFMWeaponMotion::Throw);
		MulticastWeaponEffects(EFMWeaponMotion::Throw, WeaponSlot, FVector2D::ZeroVector, 0);
		GetWorldTimerManager().SetTimer(ThrowTimer, this, &AFMCharacter::ReleaseGrenade, ThrowReleaseTime, false);
		return;
	}

	const FFMWeaponRow Data = GetWeaponData(WeaponSlot);
	switch (WeaponSlot)
	{
	case EFMWeaponSlot::Primary:
		--PrimaryAmmo;
		break;
	case EFMWeaponSlot::Secondary:
		--SecondaryAmmo;
		break;
	case EFMWeaponSlot::Melee:
		break;
	default:
		return;
	}
	NextAttackTime = Now + Data.AttackInterval;

	const EFMWeaponMotion Motion = WeaponSlot == EFMWeaponSlot::Melee ? EFMWeaponMotion::Melee : EFMWeaponMotion::Fire;
	SetWeaponMotion(Motion);
	FRotator ShotAim = Aim;
	FVector2D CameraKick = FVector2D::ZeroVector;
	float Spread = Data.SpreadDegrees;
	if (Motion == EFMWeaponMotion::Fire)
	{
		const FName Id = GetWeaponId(WeaponSlot);
		const float ResetDelay = FMath::Max(0.30f, Data.AttackInterval * 1.35f);
		BurstShotIndex = Id == LastBallisticWeapon && Now - LastBallisticShotTime <= ResetDelay
			? FMath::Min(BurstShotIndex + 1, 16) : 0;
		LastBallisticWeapon = Id;
		LastBallisticShotTime = Now;

		float Strength = 1.f;
		switch (Data.Category)
		{
		case EFMWeaponCategory::SMG: Strength = 0.72f; break;
		case EFMWeaponCategory::Pistol: Strength = 0.85f; break;
		case EFMWeaponCategory::Sniper: Strength = 1.50f; break;
		case EFMWeaponCategory::Shotgun: Strength = 1.65f; break;
		case EFMWeaponCategory::MachineGun: Strength = 1.20f; break;
		default: break;
		}
		const float AimScale = bAiming ? 0.55f : 1.f;
		const float Rise = FMath::Min(BurstShotIndex, 9) * 0.35f;
		const float Sway = FMath::Sin(BurstShotIndex * 0.7f) * FMath::Min(BurstShotIndex * 0.18f, 1.15f);
		ShotAim.Pitch += Rise * Strength * AimScale;
		ShotAim.Yaw += Sway * Strength * AimScale;
		CameraKick = FVector2D((0.95f + Rise * 0.20f) * Strength * AimScale,
			(Sway * 0.24f + 0.10f) * Strength * AimScale);
		Spread = CalculateShotSpread(Data, BurstShotIndex);
	}
	MulticastWeaponEffects(Motion, WeaponSlot, CameraKick, BurstShotIndex);

	for (int32 Pellet = 0; Pellet < Data.PelletCount; ++Pellet)
	{
		const FVector Direction = Spread > 0.f
			? FMath::VRandCone(ShotAim.Vector(), FMath::DegreesToRadians(Spread)) : ShotAim.Vector();
	FHitResult Hit;
	FCollisionQueryParams Query(SCENE_QUERY_STAT(FMWeapon), false, this);
	if (GetWorld()->LineTraceSingleByChannel(Hit, Start, Start + Direction * Data.Range, ECC_GameTraceChannel1, Query))
	{
#if !UE_BUILD_SHIPPING
		if (Motion == EFMWeaponMotion::Fire && CVarDrawImpacts.GetValueOnGameThread() > 0)
			DrawDebugPoint(GetWorld(), Hit.ImpactPoint + Hit.ImpactNormal * 2.f, 8.f, FColor::Orange, false, 5.f);
#endif
		const EFMHitZone Zone = HitZoneForBone(Hit.BoneName);
		float Damage = Data.Damage * (Zone == EFMHitZone::Head ? Data.HeadMultiplier : Zone == EFMHitZone::Limb ? Data.LimbMultiplier : 1.f);
		if (Data.FalloffStart < Data.Range)
			Damage *= FMath::GetMappedRangeValueClamped(FVector2D(Data.FalloffStart,Data.Range),
				FVector2D(1.f,Data.MinimumDamageFraction),Hit.Distance);
		UGameplayStatics::ApplyPointDamage(Hit.GetActor(), Damage, Direction, Hit, GetController(), this,
			WeaponSlot == EFMWeaponSlot::Melee ? UDamageType::StaticClass() : UFMFirearmDamageType::StaticClass());
	}
 }
}

void AFMCharacter::ServerStartFire_Implementation()
{
	ServerUseWeapon_Implementation();
	if (WeaponSlot <= EFMWeaponSlot::Secondary && GetWeaponData(WeaponSlot).bAutomatic && CanAttack() && GetLoadedAmmo() > 0)
	{
		RepeatFire();
	}
}

void AFMCharacter::RepeatFire()
{
	if (GetWorld()->GetTimeSeconds() + UE_DOUBLE_SMALL_NUMBER >= NextAttackTime)
	{
		ServerUseWeapon_Implementation();
	}
	if (CanAttack() && WeaponSlot <= EFMWeaponSlot::Secondary && GetWeaponData(WeaponSlot).bAutomatic && GetLoadedAmmo() > 0)
	{
		const float Delay = FMath::Max(0.001, NextAttackTime - GetWorld()->GetTimeSeconds());
		GetWorldTimerManager().SetTimer(AutoFireTimer, this, &AFMCharacter::RepeatFire, Delay, false);
	}
}

void AFMCharacter::ServerStopFire_Implementation()
{
	GetWorldTimerManager().ClearTimer(AutoFireTimer);
}

void AFMCharacter::ServerReload_Implementation()
{
	const bool bPrimary = WeaponSlot == EFMWeaponSlot::Primary;
	if (Health <= 0.f || bSprinting || !IsCombatPhaseAllowed() || IsReloading() ||
		(WeaponMotion.Motion == EFMWeaponMotion::Equip && GetMotionAge() < GetEquipDuration()) ||
		(!bPrimary && WeaponSlot != EFMWeaponSlot::Secondary) || GetReserveAmmo() <= 0 ||
		GetLoadedAmmo() >= GetWeaponData(WeaponSlot).MagazineSize)
	{
		return;
	}

	ServerStopFire_Implementation();
	bAiming = false;
	SetWeaponMotion(EFMWeaponMotion::Reload);
	BurstShotIndex = 0;
	LastBallisticWeapon = NAME_None;
	MulticastWeaponEffects(EFMWeaponMotion::Reload, WeaponSlot, FVector2D::ZeroVector, 0);
	GetWorldTimerManager().SetTimer(ReloadTimer, this, &AFMCharacter::FinishReload, GetReloadDuration(), false);
}

void AFMCharacter::FinishReload()
{
	if (!HasAuthority() || Health <= 0.f || WeaponSlot > EFMWeaponSlot::Secondary || !IsCombatPhaseAllowed())
	{
		CancelReload(); return;
	}
	const bool bPrimary = WeaponSlot == EFMWeaponSlot::Primary;
	int32& Loaded = bPrimary ? PrimaryAmmo : SecondaryAmmo;
	int32& Reserve = bPrimary ? PrimaryReserve : SecondaryReserve;
	const int32 Count = FMath::Clamp(GetWeaponData(WeaponSlot).MagazineSize - Loaded, 0, Reserve);
	Loaded += Count;
	Reserve -= Count;
	SetWeaponMotion(EFMWeaponMotion::Ready);
}

void AFMCharacter::UnPossessed()
{
	CancelWeaponActions();
	Super::UnPossessed();
}

void AFMCharacter::CancelWeaponActions()
{
	bAiming = false;
	ServerStopFire_Implementation();
	GetWorldTimerManager().ClearTimer(ThrowTimer);
	CancelReload();
}

void AFMCharacter::ReleaseGrenade()
{
	if (!CanAttack() || WeaponSlot != EFMWeaponSlot::Throwable || Grenades <= 0) return;
	FActorSpawnParameters Spawn;
	Spawn.Owner = this;
	Spawn.Instigator = this;
	Spawn.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	const FRotator Aim = GetBaseAimRotation();
	const FVector Start = FirstPersonCamera->GetComponentLocation();
	if (GetWorld()->SpawnActor<AFMGrenade>(Start, Aim, Spawn))
	{
		--Grenades;
	}
}

void AFMCharacter::CancelReload()
{
	if (IsReloading())
	{
		GetWorldTimerManager().ClearTimer(ReloadTimer);
		SetWeaponMotion(EFMWeaponMotion::Ready);
	}
}
