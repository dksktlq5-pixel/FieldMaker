#include "FMCharacter.h"
#include "Misc/PackageName.h"

#include "Camera/CameraComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "FMWeaponPoseMesh.h"
#include "GameFramework/GameStateBase.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Math/RotationMatrix.h"
#include "Materials/MaterialInterface.h"
#include "Sound/SoundBase.h"
#include "Sound/SoundAttenuation.h"
#include "TimerManager.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
float FadeInOut(float Age, float Duration)
{
	return FMath::Sin(PI * FMath::Clamp(Age / Duration, 0.f, 1.f));
}

float Phase(float Age, float Start, float End)
{
	const float T = FMath::Clamp((Age - Start) / (End - Start), 0.f, 1.f);
	return FMath::SmoothStep(0.f, 1.f, T);
}

constexpr float PistolMagazineInsertStart = 0.65f;
constexpr float PistolMagazineSeated = 0.82f;

FFMHandTarget Grip(const FVector& PalmContact, const FVector& BoneX, const FVector& BoneY,
	const FVector2D& Fingers, const FVector2D& Index, float Thumb, bool bRight)
{
	const FQuat Rotation = FRotationMatrix::MakeFromXY(BoneX, BoneY).ToQuat();
	const FVector PalmOffset = bRight ? FVector(-6, 1.6, 0) : FVector(6, -1.6, 0);
	return {FTransform(Rotation, PalmContact - Rotation.RotateVector(PalmOffset)), Fingers, Index, Thumb};
}

FFMHandTarget KnifeGrip()
{
	// The knuckle row follows the handle, allowing all four digits to wrap its cross-section.
	return Grip(FVector(0.1, 0.1, 0.4), FVector(-0.10, 0.99, 0.05), FVector(0.02, -0.05, 1),
		FVector2D(68, 121), FVector2D(59, 109), 0.72f, true);
}

FFMHandTarget BlendHands(const FFMHandTarget& From, const FFMHandTarget& To, float Alpha)
{
	FFMHandTarget Result;
	Result.Wrist.Blend(From.Wrist, To.Wrist, Alpha);
	Result.FingerBend = FMath::Lerp(From.FingerBend, To.FingerBend, Alpha);
	Result.IndexBend = FMath::Lerp(From.IndexBend, To.IndexBend, Alpha);
	Result.ThumbOpposition = FMath::Lerp(From.ThumbOpposition, To.ThumbOpposition, Alpha);
	Result.ThumbAlongFrame = FMath::Lerp(From.ThumbAlongFrame, To.ThumbAlongFrame, Alpha);
 Result.bWeaponGrip=From.bWeaponGrip || To.bWeaponGrip;
 Result.ElbowPole=FMath::Lerp(From.ElbowPole,To.ElbowPole,Alpha);
	return Result;
}

bool IsFirearm(EFMWeaponSlot Slot)
{
	return Slot == EFMWeaponSlot::Primary || Slot == EFMWeaponSlot::Secondary;
}
}

void AFMCharacter::InitializeWeaponPresentation()
{
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Rifle(TEXT("/Game/FieldMaker/Weapons/Realistic/SM_FM_AR4.SM_FM_AR4"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Pistol(TEXT("/Game/FieldMaker/Weapons/Realistic/SM_FM_Pistol.SM_FM_Pistol"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Knife(TEXT("/Game/FieldMaker/Weapons/Realistic/SM_FM_M9.SM_FM_M9"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Grenade(TEXT("/Game/FieldMaker/Weapons/Realistic/SM_FM_G67.SM_FM_G67"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> RifleMag(TEXT("/Game/FPS_Weapon_Bundle/Weapons/Meshes/AR4/SM_AR4_Mag.SM_AR4_Mag"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> FlamePlane(TEXT("/Engine/BasicShapes/Plane.Plane"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> FlashMaterial(TEXT("/Game/FieldMaker/Weapons/M_FMMuzzleFlame.M_FMMuzzleFlame"));
	static ConstructorHelpers::FObjectFinder<USoundBase> RifleAudio(TEXT("/Game/FreeWeaponSFX/Wav/AssaultRifle/Gunshots/sfx_wpn_ar_gunshot_01.sfx_wpn_ar_gunshot_01"));
	static ConstructorHelpers::FObjectFinder<USoundBase> PistolAudio(TEXT("/Game/FreeWeaponSFX/Wav/Handgun/Gunshots/sfx_wpn_hgn_gunshot_01.sfx_wpn_hgn_gunshot_01"));
	static ConstructorHelpers::FObjectFinder<USoundBase> ReloadAudio(TEXT("/Game/FreeWeaponSFX/Wav/AssaultRifle/Reload/sfx_wpn_ar_reload_01_magOut.sfx_wpn_ar_reload_01_magOut"));
	static ConstructorHelpers::FObjectFinder<USoundBase> MeleeAudio(TEXT("/Game/FieldMaker/Weapons/A_FMKnifeWhoosh.A_FMKnifeWhoosh"));
	static ConstructorHelpers::FObjectFinder<USoundBase> ThrowAudio(TEXT("/Game/FieldMaker/Weapons/A_FMThrow.A_FMThrow"));
	WeaponModels = {Rifle.Object, Pistol.Object, Knife.Object, Grenade.Object};
	MagazineModels = {RifleMag.Object, nullptr};
	RifleShot = RifleAudio.Object;
 static ConstructorHelpers::FObjectFinder<USoundAttenuation> Attenuation(TEXT("/Game/FieldMaker/Weapons/A_FMFireAttenuation.A_FMFireAttenuation"));
 FireAttenuation=Attenuation.Object;
	PistolShot = PistolAudio.Object;
	ReloadSound = ReloadAudio.Object;
 static ConstructorHelpers::FObjectFinder<USoundBase> Sgn(TEXT("/Game/FreeWeaponSFX/Wav/Shotgun/Gunshots/sfx_wpn_sgn_gunshot_01.sfx_wpn_sgn_gunshot_01"));
 static ConstructorHelpers::FObjectFinder<USoundBase> ArIn(TEXT("/Game/FreeWeaponSFX/Wav/AssaultRifle/Reload/sfx_wpn_ar_reload_03_magIn.sfx_wpn_ar_reload_03_magIn"));
 static ConstructorHelpers::FObjectFinder<USoundBase> HgnIn(TEXT("/Game/FreeWeaponSFX/Wav/Handgun/Reload/sfx_wpn_hgn_reload_04_magIn.sfx_wpn_hgn_reload_04_magIn"));
 static ConstructorHelpers::FObjectFinder<USoundBase> SgnIn(TEXT("/Game/FreeWeaponSFX/Wav/Shotgun/Reload/sfx_wpn_sgn_reload_04_chambering.sfx_wpn_sgn_reload_04_chambering"));
 static ConstructorHelpers::FObjectFinder<USoundBase> HgnOut(TEXT("/Game/FreeWeaponSFX/Wav/Handgun/Reload/sfx_wpn_hgn_reload_02_magOut.sfx_wpn_hgn_reload_02_magOut"));
 static ConstructorHelpers::FObjectFinder<USoundBase> SgnOut(TEXT("/Game/FreeWeaponSFX/Wav/Shotgun/Reload/sfx_wpn_sgn_reload_01_charge.sfx_wpn_sgn_reload_01_charge"));
 PistolReloadOut=HgnOut.Object;ShotgunReloadOut=SgnOut.Object;
 ShotgunShot=Sgn.Object;RifleReloadIn=ArIn.Object;PistolReloadIn=HgnIn.Object;ShotgunReloadIn=SgnIn.Object;
	MeleeSound = MeleeAudio.Object;
	ThrowSound = ThrowAudio.Object;

	const auto AddPart = [this](FName Name, USceneComponent* Parent, bool bFirstPerson)
	{
		auto* Part = CreateDefaultSubobject<UStaticMeshComponent>(Name);
		Part->SetupAttachment(Parent);
		Part->SetOnlyOwnerSee(bFirstPerson);
		Part->SetOwnerNoSee(!bFirstPerson);
		Part->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Part->SetGenerateOverlapEvents(false);
		Part->SetCastShadow(!bFirstPerson);
		Part->SetVisibility(false);
		return Part;
	};

	ViewWeapon = AddPart(TEXT("ViewWeapon"), FirstPersonCamera, true);
	WorldWeapon = AddPart(TEXT("WorldWeapon"), GetMesh(), false);
	ViewMagazine = AddPart(TEXT("ViewMagazine"), ViewWeapon, true);
	WorldMagazine = AddPart(TEXT("WorldMagazine"), WorldWeapon, false);
 ViewAccessory = AddPart(TEXT("ViewAccessory"), ViewWeapon, true);
 WorldAccessory = AddPart(TEXT("WorldAccessory"), WorldWeapon, false);
	ViewFlash = AddPart(TEXT("ViewMuzzleFlash"), ViewWeapon, true);
	WorldFlash = AddPart(TEXT("WorldMuzzleFlash"), WorldWeapon, false);
	for (UStaticMeshComponent* Flash : {ViewFlash.Get(), WorldFlash.Get()})
	{
		Flash->SetStaticMesh(FlamePlane.Object);
  Flash->SetRelativeRotation(FRotator(0,0,90));
  auto* Cross=AddPart(Flash==ViewFlash?TEXT("ViewFlameCross"):TEXT("WorldFlameCross"),Flash,Flash==ViewFlash);
  Cross->SetStaticMesh(FlamePlane.Object);Cross->SetMaterial(0,FlashMaterial.Object);Cross->SetRelativeRotation(FRotator(0,0,90));
		Flash->SetMaterial(0, FlashMaterial.Object);
		Flash->SetCastShadow(false);
	}
	GetMesh()->AddTickPrerequisiteActor(this);
	FirstPersonArms->AddTickPrerequisiteActor(this);
}

double AFMCharacter::GetWeaponTime() const
{
	const AGameStateBase* State = GetWorld()->GetGameState();
	return State ? State->GetServerWorldTimeSeconds() : GetWorld()->GetTimeSeconds();
}

float AFMCharacter::GetMotionAge() const
{
	return FMath::Max(0.0, GetWeaponTime() - WeaponMotion.StartTime);
}

void AFMCharacter::SetWeaponMotion(EFMWeaponMotion Motion)
{
	WeaponMotion.Motion = Motion;
	WeaponMotion.StartTime = GetWeaponTime();
	ForceNetUpdate();
}

void AFMCharacter::OnRep_WeaponSlot()
{
	const int32 Index = static_cast<int32>(WeaponSlot);
	if (!WeaponModels.IsValidIndex(Index))
	{
		return;
	}

 UStaticMesh* Model=WeaponModels[Index];
 const auto Visual=GetWeaponVisualData();
 if(auto* Detailed=Visual.Mesh.LoadSynchronous())Model=Detailed;
 ViewWeapon->SetStaticMesh(Model);
 WorldWeapon->SetStaticMesh(Model);
	const bool bKnife = WeaponSlot == EFMWeaponSlot::Melee;
	const FName Socket = bKnife ? FName(TEXT("hand_r")) : NAME_None;
	ViewWeapon->AttachToComponent(bKnife ? static_cast<USceneComponent*>(FirstPersonArms.Get()) : FirstPersonCamera.Get(),
		FAttachmentTransformRules::KeepRelativeTransform, Socket);
	WorldWeapon->AttachToComponent(GetMesh(), FAttachmentTransformRules::KeepRelativeTransform, Socket);
	if (bKnife)
	{
		const FTransform KnifeInHand = KnifeGrip().Wrist.Inverse();
		ViewWeapon->SetRelativeTransform(KnifeInHand);
		WorldWeapon->SetRelativeTransform(KnifeInHand);
	}
	UStaticMesh* Magazine = MagazineModels.IsValidIndex(Index) ? MagazineModels[Index].Get() : nullptr;
 if(!Visual.Mesh.IsNull())Magazine=Visual.MagazineMesh.LoadSynchronous();
 for(auto* Accessory:{ViewAccessory.Get(),WorldAccessory.Get()})
 {
  Accessory->SetStaticMesh(Visual.AccessoryMesh.LoadSynchronous());
  Accessory->SetRelativeTransform(FTransform(FQuat::Identity,Visual.AccessoryLocation,Visual.AccessoryScale));
  Accessory->SetVisibility(Accessory->GetStaticMesh()!=nullptr);
 }
	ViewMagazine->SetStaticMesh(Magazine);
	WorldMagazine->SetStaticMesh(Magazine);
}

void AFMCharacter::UpdateWeaponPresentation(float DeltaSeconds)
{
	const auto& Visual = GetActiveWeaponVisualRow();
 const bool bDetailed=!Visual.Mesh.IsNull();
	const bool bPrimary = WeaponSlot == EFMWeaponSlot::Primary;
	const bool bFirearm = IsFirearm(WeaponSlot);
	const bool bVisible = GetPlayerState() && Health > 0.f;
	const bool bLocalView = IsLocallyControlled();
	const bool bScoped = bLocalView && IsScopeViewActive();
	FVector HipFraming(-3.f, 3.f, -4.f);
	FRotator HipAngle(0.f, -7.f, 0.f);
	FVector HipArmAdjustment = FVector::ZeroVector;
	const FVector PrimaryReloadArmFraming = GetWeaponId(WeaponSlot) == FName(TEXT("M4"))
		? FVector(-18.f, 7.f, -11.f) : FVector(-10.f, 15.f, -1.f);
	if (bLocalView && bPrimary)
	{
		switch (GetWeaponData(WeaponSlot).Category)
		{
		case EFMWeaponCategory::Rifle:
			HipFraming = FVector(-14.f, 7.f, -11.f);
			HipAngle = FRotator(10.f, -5.f, -4.f);
			// The AR-15 mesh has a different receiver and sight layout.
			if (GetWeaponId(WeaponSlot) == FName(TEXT("M4")))
			{
				HipFraming.X = -18.f;
				HipAngle.Yaw = -11.f;
			}
			else
			{
				HipArmAdjustment.X = 4.f;
			}
			break;
		case EFMWeaponCategory::Sniper:
			HipFraming = FVector(-6.f, 10.f, -14.f);
			HipAngle = FRotator(5.f, -6.f, -3.f);
			break;
		case EFMWeaponCategory::SMG:
			HipFraming = FVector(-9.f, 9.f, -8.f);
			HipAngle = FRotator(8.f, -7.f, -4.f);
			break;
		default: // Shotguns and machine guns need clearance around their larger receivers.
			HipFraming = FVector(-10.f, 10.f, -11.f);
			HipAngle = FRotator(8.f, -6.f, -4.f);
			break;
		}
	}
	const FVector HipViewOffset = bLocalView && bFirearm ? HipFraming * (1.f - ADSAlpha) : FVector::ZeroVector;
	// Keep the running weapon close to the lowered shoulders instead of an idle aim pose.
	SprintPoseAlpha = FMath::FInterpTo(SprintPoseAlpha, bSprinting && GetVelocity().Size2D() > 100.f ? 1.f : 0.f, DeltaSeconds, 10.f);
	// Retract the knife stance's shoulders while keeping both hands anchored in camera space.
	if (bLocalView)
	{
		FirstPersonArms->SetVisibility(bVisible && !bScoped);
		const float ReloadBlend = bPrimary && IsReloading() ? FadeInOut(GetMotionAge(), GetReloadDuration()) : 0.f;
		const FVector ArmOffset = FVector(WeaponSlot == EFMWeaponSlot::Melee ? -12.f : 8.f, 0.f, -170.f)
			+ (bDetailed ? FMath::Lerp(Visual.ArmOffset, Visual.ADSArmOffset, bFirearm ? ADSAlpha : 0.f) : FVector::ZeroVector)
			+ FMath::Lerp(HipViewOffset + HipArmAdjustment * (1.f - ADSAlpha),
				PrimaryReloadArmFraming * (1.f - ADSAlpha), ReloadBlend);
		FirstPersonArms->SetRelativeLocation(DeltaSeconds > 0.f
			? FMath::VInterpTo(FirstPersonArms->GetRelativeLocation(), ArmOffset, DeltaSeconds, 18.f) : ArmOffset);
	}
	const float Speed = GetVelocity().Size2D();
	const bool bOnGround = GetCharacterMovement()->IsMovingOnGround();
	MoveBobAlpha = FMath::FInterpTo(MoveBobAlpha, bOnGround ? FMath::Clamp(Speed / 250.f, 0.f, 1.f) : 0.f, DeltaSeconds, 10.f);
	const float Bob = MoveBobAlpha * FMath::Lerp(0.32f, 0.07f, ADSAlpha);
	// Advance by ground distance; stopping no longer changes the gait phase or snaps the hands.
	if (bOnGround) MoveBobTime = FMath::Fmod(MoveBobTime + DeltaSeconds * Speed * (2.f * PI / 180.f), 2.f * PI);
	const float ShotAge = GetWorld()->GetTimeSeconds() - LastShotTime;
	const bool bShotEvent = LastShotSlot == WeaponSlot && ShotAge < 0.23f &&
		(WeaponMotion.Motion == EFMWeaponMotion::Ready || WeaponMotion.Motion == EFMWeaponMotion::Fire);
	const EFMWeaponMotion Motion = bShotEvent ? EFMWeaponMotion::Fire : WeaponMotion.Motion;
	const float Age = bShotEvent ? ShotAge : GetMotionAge();
	FVector Offset(0.f, FMath::Sin(MoveBobTime) * Bob, FMath::Cos(MoveBobTime * 2.f) * Bob);
	FRotator Rotation = FRotator::ZeroRotator;
	float ReloadTilt = 0.f;
	float MagazineTravel = 0.f;
	float PistolMagazineExchange = 0.f;
	const bool bPistolMagazine = WeaponSlot == EFMWeaponSlot::Secondary && ViewMagazine->GetStaticMesh();

	switch (Motion)
	{
	case EFMWeaponMotion::Equip:
	{
		const float Lowered = 1.f - FMath::InterpEaseOut(0.f, 1.f, FMath::Clamp(Age / GetEquipDuration(), 0.f, 1.f), 2.f);
		Offset.Z -= 25.f * Lowered;
		Rotation.Pitch -= 40.f * Lowered;
		break;
	}
	case EFMWeaponMotion::Fire:
	{
		const float Kick = FMath::Square(1.f - FMath::Clamp(Age / (bPrimary ? 0.12f : 0.21f), 0.f, 1.f));
		const auto Category=GetWeaponData(WeaponSlot).Category;
  const float Weight=Category==EFMWeaponCategory::SMG?.65f:Category==EFMWeaponCategory::Sniper || Category==EFMWeaponCategory::Shotgun?1.5f:1.f;
  Offset.X -= Kick * (bPrimary ? 1.8f*Weight : 1.6f);
  Rotation.Pitch += Kick * (bPrimary ? 2.8f*Weight : 5.f);
		break;
	}
	case EFMWeaponMotion::Reload:
	{
		const float Progress = FMath::Clamp(Age / GetReloadDuration(), 0.f, 1.f);
		ReloadTilt = FadeInOut(Progress, 1.f);
		MagazineTravel = FadeInOut((Progress - 0.15f) / 0.67f, 1.f);
		Offset += FVector(-5.f, -4.f, -6.f) * ReloadTilt;
		Rotation = (bPrimary ? FRotator(18, -12, 32) : FRotator(13, -10, 22)) * ReloadTilt;
		if (bPistolMagazine)
		{
			MagazineTravel = Phase(Progress, 0.22f, 0.40f) * (1.f - Phase(Progress, PistolMagazineInsertStart, PistolMagazineSeated));
			PistolMagazineExchange = Phase(Progress, 0.40f, 0.49f) * (1.f - Phase(Progress, 0.54f, PistolMagazineInsertStart));
			Rotation = FRotator(18, -16, 26) * ReloadTilt;
		}
		break;
	}
	case EFMWeaponMotion::Melee:
	{
		const float Windup = Phase(Age, 0.f, 0.10f);
		const float Cut = Phase(Age, 0.10f, 0.26f);
		const float Recover = Phase(Age, 0.26f, 0.54f);
		Offset += FMath::Lerp(FVector(-11, 9, 2) * Windup, FVector(7, -23, -3), Cut) * (1.f - Recover);
		Rotation = FMath::Lerp(FRotator(14, 31, -12) * Windup, FRotator(7, -48, 22), Cut) * (1.f - Recover);
		break;
	}
	case EFMWeaponMotion::Throw:
	{
		const float Windup = Phase(Age, 0.f, 0.13f);
		const float Release = Phase(Age, 0.13f, ThrowReleaseTime);
		const float Recover = Phase(Age, ThrowReleaseTime, 0.55f);
		Offset += FMath::Lerp(FVector(-15, 10, 22) * Windup, FVector(9, -9, 12), Release) * (1.f - Recover);
		Rotation = FMath::Lerp(FRotator(51, 23, 30) * Windup, FRotator(-12, -10, -16), Release) * (1.f - Recover);
		break;
	}
	default:
		break;
	}

	const FVector ViewGrips[] = {FVector(49, 16, -22), FVector(49, 11, -16),
		FVector(45, 22, -15), FVector(46, 16, -18)};
	const FRotator ViewAngles[] = {HipAngle, HipAngle,
		FRotator(0, -10, 0), FRotator(6, -10, 3)};
	const int32 Slot = static_cast<int32>(WeaponSlot);
	const FRotator ViewAngle = FMath::Lerp(ViewAngles[Slot], FRotator::ZeroRotator, bFirearm ? ADSAlpha : 0.f)
		+ FRotator(-12.f, 0.f, 0.f) * SprintPoseAlpha;
	if (bLocalView)
	{
		// Keep the stock beyond the near clip plane and the top of the sights at eye level.
		const FVector AimGrip = (bPrimary ? FVector(55, 0, -18) : FVector(49, 0, -10))
			+ (bDetailed ? Visual.ADSOffset : FVector::ZeroVector);
		const FVector ViewGrip = FMath::Lerp(ViewGrips[Slot] + (bDetailed ? Visual.ViewOffset : FVector::ZeroVector),
			AimGrip, bFirearm ? ADSAlpha : 0.f) + FVector(-5.f, 0.f, -5.f) * SprintPoseAlpha;
		const FVector ViewReloadLift = FVector(4.f, 0.f, bPistolMagazine ? 12.f : 20.f) * ReloadTilt;
		// Bring the magazine and support hand into view for the exchange.
		const FVector Framing = bPrimary ? FMath::Lerp(HipViewOffset, FVector(-2.f, 0.f, -3.f), ReloadTilt)
			: HipViewOffset + FVector(6.f, -5.f, 4.f) * ReloadTilt;
		const FTransform ViewMotion(Rotation + ViewAngle, ViewGrip + Offset + ViewReloadLift + Framing);
		if (WeaponSlot == EFMWeaponSlot::Melee) ViewMeleeMotion = ViewMotion;
		else ViewWeapon->SetRelativeTransform(ViewMotion);
	}
	// The character mesh faces local +Y. Convert from actor-facing coordinates to that mesh's frame.
	const FRotator MeshFacing(0, 90, 0);
	const float AimPitch = FMath::Clamp(FRotator::NormalizeAxis(GetBaseAimRotation().Pitch), -55.f, 55.f);
	const FVector WorldGrips[] = {FVector(27, 10, 137), FVector(37, 6, 136),
		FVector(30, 23, 121), FVector(24, 18, 125)};
	const FRotator AimOffset(bFirearm ? AimPitch * 0.72f : 0.f, 0.f, 0.f);
	const FVector ShoulderPivot(8, 0, 140);
	const FVector RunningGrip = bPrimary ? FVector(19, 10, 115) : FVector(25, 8, 118);
	const FVector StanceGrip = FMath::Lerp(WorldGrips[Slot], RunningGrip, bFirearm ? SprintPoseAlpha : 0.f);
	const FVector WorldGrip = ShoulderPivot + AimOffset.RotateVector(StanceGrip - ShoulderPivot);
	const bool bWorldKnife = WeaponSlot == EFMWeaponSlot::Melee;
	const FRotator WorldAngle = Rotation + (bFirearm ? AimOffset + FRotator(-18.f, 0.f, 0.f) * SprintPoseAlpha :
		bWorldKnife ? FRotator(0.f, -20.f, 0.f) : ViewAngle);
	const FTransform WorldMotion(MeshFacing.Quaternion() * WorldAngle.Quaternion(),
		MeshFacing.RotateVector(WorldGrip + Offset * 0.7f));
	if (bWorldKnife)
	{
		// Drive the arm from the motion track. The visible knife is attached to the solved hand.
		WorldMeleeMotion = WorldMotion;
	}
	else
	{
		WorldWeapon->SetRelativeTransform(WorldMotion);
	}

	const FVector MagBase = bDetailed?Visual.MagazineLocation:(bPrimary ? FVector(11.f, 0.f, -3.f) : FVector(0.f, 0.f, -2.f));
	// The pistol magazine follows the angle of the grip until it clears the well.
	const FVector MagOffset = bPistolMagazine
		? FVector(-6.2f, 0.f, -12.f) * MagazineTravel + FVector(-2.f, -5.f, -6.f) * PistolMagazineExchange
		: FVector(-3.f, -6.f, -18.f) * MagazineTravel;
	const FRotator MagTurn = bPistolMagazine
		? FRotator(8.f, -10.f, 10.f) * PistolMagazineExchange
		: FRotator(15.f, -12.f, 7.f) * MagazineTravel;
	for (UStaticMeshComponent* Magazine : {ViewMagazine.Get(), WorldMagazine.Get()})
	{
		if (!bLocalView && Magazine == ViewMagazine) continue;
		Magazine->SetRelativeLocationAndRotation(MagBase + MagOffset, Visual.MagazineRotation + MagTurn);
		Magazine->SetVisibility(bVisible && bFirearm && Magazine->GetStaticMesh()!=nullptr && (Magazine!=ViewMagazine || !bScoped));
	}

	const bool bGrenadeReady = WeaponSlot != EFMWeaponSlot::Throwable ||
		(Grenades > 0 && (Motion != EFMWeaponMotion::Throw || Age < ThrowReleaseTime || Age > 0.53f));
	if (bLocalView) ViewWeapon->SetVisibility(bVisible && bGrenadeReady && !bScoped);
	WorldWeapon->SetVisibility(bVisible && bGrenadeReady);
	WorldAccessory->SetVisibility(bVisible && WorldAccessory->GetStaticMesh() != nullptr);
	if (bLocalView) ViewAccessory->SetVisibility(bVisible && ViewAccessory->GetStaticMesh() != nullptr && !bScoped);

	const bool bFlash = bVisible && bFirearm && Motion == EFMWeaponMotion::Fire && Age < 0.045f;
	for (UStaticMeshComponent* Flash : {ViewFlash.Get(), WorldFlash.Get()})
	{
		if (!bLocalView && Flash == ViewFlash) continue;
		if (bFlash)
		{
			float MuzzleX=bPrimary?62.f:21.f;
   if(bPrimary && GetWeaponData(WeaponSlot).Category==EFMWeaponCategory::Sniper)MuzzleX=104.f;
   else if(bPrimary && PrimaryWeaponId==TEXT("M4A1S"))MuzzleX=83.f;
   Flash->SetRelativeLocation((bDetailed?Visual.Muzzle:FVector(MuzzleX, 0.f, bPrimary ? 9.f : 7.f))+FVector(bPrimary?9.f:6.f,0,0));
			Flash->SetRelativeScale3D(FVector(bPrimary ? 0.18f : 0.12f, 0.06f, 1.f));
		}
		Flash->SetVisibility(bFlash && (Flash!=ViewFlash || !bScoped),true);
	}
}

bool AFMCharacter::GetWeaponGripTargets(const USkeletalMeshComponent& PoseMesh, FFMGripTargets& Targets) const
{
	if (!GetPlayerState() || Health <= 0.f || GetNetMode() == NM_DedicatedServer)
	{
		return false;
	}

	const bool bView = &PoseMesh == FirstPersonArms;
	if (bView && !IsLocallyControlled())
	{
		return false;
	}
	const UStaticMeshComponent* Weapon = bView ? ViewWeapon.Get() : WorldWeapon.Get();
	const UStaticMeshComponent* Magazine = bView ? ViewMagazine.Get() : WorldMagazine.Get();
	if (!Weapon || !Weapon->GetStaticMesh())
	{
		return false;
	}

	const FTransform MeshTransform = PoseMesh.GetComponentTransform();
	const FTransform WeaponInMesh = WeaponSlot == EFMWeaponSlot::Melee
		? (bView ? ViewMeleeMotion * FirstPersonCamera->GetComponentTransform().GetRelativeTransform(MeshTransform) : WorldMeleeMotion)
		: Weapon->GetComponentTransform().GetRelativeTransform(MeshTransform);
	const bool bFirearm = IsFirearm(WeaponSlot);
	const auto& Visual = GetActiveWeaponVisualRow();
 const bool bDetailed=!Visual.Mesh.IsNull();
	const float Age = GetMotionAge();
	const bool bTrigger = WeaponMotion.Motion == EFMWeaponMotion::Fire && Age < 0.16f;
	const bool bThrowRelease = WeaponMotion.Motion == EFMWeaponMotion::Throw &&
		Age >= ThrowReleaseTime && Age < 0.55f;

	switch (WeaponSlot)
	{
	case EFMWeaponSlot::Primary:
		Targets.Right = Grip(bDetailed?Visual.RightPalm:FVector(-1, 0.7, -0.4), FVector(-1, 0, 0.06), FVector(0, -1, 0),
			FVector2D(74, 130), bTrigger ? FVector2D(28, 67) : FVector2D(8, 28), 0.8f, true);
		Targets.Left = Grip(bDetailed?Visual.LeftPalm:FVector(21, -1.1, 4), bDetailed?Visual.LeftBoneX:FVector(1, 0.12, 0.19), bDetailed?Visual.LeftBoneY:FVector(0, -0.40, -0.91),
			FVector2D(51, 105), FVector2D(46, 96), 0.54f, false);
		Targets.Right.ThumbAlongFrame = 0.85f;
		break;
	case EFMWeaponSlot::Secondary:
		Targets.Right = Grip(bDetailed?Visual.RightPalm:FVector(-1.4, 0.8, -0.1), FVector(-1, 0, 0.18), FVector(0, -1, 0),
			FVector2D(69, 118), bTrigger ? FVector2D(30, 68) : FVector2D(5, 15), 0.31f, true);
		Targets.Left = Grip(bDetailed?Visual.LeftPalm:FVector(-0.8, -3.0, -3.7), FVector(0.89, 0.19, 0.42), FVector(0, -0.96, -0.28),
			FVector2D(58, 109), FVector2D(47, 93), 0.39f, false);
		Targets.Right.ThumbAlongFrame = 1.f;
		Targets.Left.ThumbAlongFrame = 0.72f;
		break;
	case EFMWeaponSlot::Melee:
		Targets.Right = KnifeGrip();
		Targets.bAlignKnifeWrist = true;
		Targets.bFirstPersonKnife = bView;
		break;
	case EFMWeaponSlot::Throwable:
	{
		const float Release = Grenades == 0 ? 1.f : bThrowRelease ? 1.f - Phase(Age, ThrowReleaseTime, 0.52f) : 0.f;
		const FVector2D Bend = FMath::Lerp(FVector2D(51, 105), FVector2D(12, 26), Release);
		Targets.Right = Grip(FVector(-0.8, 1.0, 0.6), FVector(-0.94, 0.25, -0.20), FVector(0, -0.92, 0.39),
			Bend, Bend, 0.57f * (1.f - Release), true);
		break;
	}
	}

 // Model-specific contact and finger closure, shared by first-person and replicated full-body meshes.
 if(bFirearm && bDetailed && Visual.bUseCustomHands)
 {
  Targets.Right=Grip(Visual.RightPalm,Visual.RightBoneX,Visual.RightBoneY,Visual.RightFingerBend,
   bTrigger?Visual.RightIndexFire:Visual.RightIndexIdle,FMath::Clamp(Visual.RightThumb,0.f,1.f),true);
  Targets.Left=Grip(Visual.LeftPalm,Visual.LeftBoneX,Visual.LeftBoneY,Visual.LeftFingerBend,
   Visual.LeftIndexBend,FMath::Clamp(Visual.LeftThumb,0.f,1.f),false);
  Targets.Right.bWeaponGrip=true;Targets.Left.bWeaponGrip=true;Targets.Left.ElbowPole=Visual.LeftElbowPole;
  Targets.Right.ThumbAlongFrame=FMath::Clamp(Visual.RightThumbAlong,0.f,1.f);
  Targets.Left.ThumbAlongFrame=FMath::Clamp(Visual.LeftThumbAlong,0.f,1.f);
 }

	Targets.Right.Wrist *= WeaponInMesh;
	if (bFirearm)
	{
		Targets.Left.Wrist *= WeaponInMesh;
		if (IsReloading())
		{
			const float Progress = Age / GetReloadDuration();
			// Wrap across the magazine. Aligning the fingers up its length leaves a gap under the palm.
			const bool bSeparateMagazine=Magazine && Magazine->GetStaticMesh();
			const bool bPistolMagazine = WeaponSlot == EFMWeaponSlot::Secondary && bSeparateMagazine;
			const float Reach = bPistolMagazine
				? Phase(Progress, 0.06f, 0.22f) * (1.f - Phase(Progress, PistolMagazineSeated, 0.97f))
				: Phase(Progress, 0.04f, 0.25f) * (1.f - Phase(Progress, 0.74f, 0.96f));
   const FVector ReloadContact=bSeparateMagazine?(Visual.bUseCustomHands?Visual.MagazineHand:(WeaponSlot==EFMWeaponSlot::Primary?FVector(-2.6,-1.9,-.9):FVector(-2,-1.35,-2.8))):Visual.ReloadPalm;
   FFMHandTarget MagazineHand = Grip(ReloadContact,
				FVector(0.97, 0.04, 0.23), FVector(0, -1, 0),
				FVector2D(68, 121), FVector2D(60, 110), 0.76f, false);
			MagazineHand.bWeaponGrip=Visual.bUseCustomHands;MagazineHand.ElbowPole=Visual.LeftElbowPole;
			if (bPistolMagazine)
			{
				MagazineHand = Grip(ReloadContact, FVector(0.85, 0.42, 0.32), FVector(0, -0.58, -0.81),
					FVector2D(64, 112), FVector2D(53, 94), 0.57f, false);
				MagazineHand.bWeaponGrip = true;
				MagazineHand.ElbowPole = FVector(22, -8, -18);
				MagazineHand.ThumbAlongFrame = 0.55f;
			}
   MagazineHand.Wrist *= bSeparateMagazine?Magazine->GetComponentTransform().GetRelativeTransform(MeshTransform):WeaponInMesh;
			Targets.Left = BlendHands(Targets.Left, MagazineHand, Reach);
		}
	}
	else
	{
		const bool bKnife = WeaponSlot == EFMWeaponSlot::Melee;
		const float Lower = WeaponMotion.Motion == EFMWeaponMotion::Melee ? FadeInOut(Age, 0.52f) :
			WeaponMotion.Motion == EFMWeaponMotion::Throw ? FadeInOut(Age, 0.53f) :
			WeaponMotion.Motion == EFMWeaponMotion::Equip ? 1.f - Phase(Age, 0.f, GetEquipDuration()) : 0.f;
		const FVector ViewGuard = bKnife ? FVector(39, -20, -17) : FVector(38, -17, -32);
		const FVector WorldGuard = bKnife ? FVector(24, -20, 119) : FVector(18, -17, 111);
		const FVector Palm = (bView ? ViewGuard : WorldGuard) - FVector(0, 0, 8 * Lower);
		Targets.Left = Grip(Palm, FVector(1, 0.26, 0.20),
			bKnife ? FVector(0, 0.38, 0.92) : FVector(0, -0.83, -0.56),
			bKnife ? FVector2D(37, 72) : FVector2D(42, 79),
			bKnife ? FVector2D(25, 58) : FVector2D(35, 70), 0.6f, false);
		const FTransform GuardSpace = bView ? FirstPersonCamera->GetComponentTransform().GetRelativeTransform(MeshTransform) :
			FTransform(FRotator(0, 90, 0));
		Targets.Left.Wrist *= GuardSpace;

		if (!bKnife && WeaponMotion.Motion == EFMWeaponMotion::Throw && Age < ThrowReleaseTime)
		{
			FFMHandTarget PinHand = Grip(FVector(-1, -3.3, 7),
				FVector(0.76, 0.55, 0.34), FVector(0.15, -0.68, -0.72),
				FVector2D(46, 83), FVector2D(25, 65), 0.8f, false);
			PinHand.Wrist *= WeaponInMesh;
			const float Pull = Phase(Age, 0.f, 0.08f) * (1.f - Phase(Age, 0.12f, ThrowReleaseTime));
			Targets.Left = BlendHands(Targets.Left, PinHand, Pull);
		}

	}
	return true;
}

void AFMCharacter::MulticastWeaponEffects_Implementation(EFMWeaponMotion Motion, EFMWeaponSlot Slot)
{
	if (GetNetMode() == NM_DedicatedServer)
	{
		return;
	}

	if (Motion == EFMWeaponMotion::Fire)
	{
		LastShotSlot = Slot;
		LastShotTime = GetWorld()->GetTimeSeconds();
	}

	USoundBase* Sound = nullptr;
	switch (Motion)
	{
	case EFMWeaponMotion::Fire: Sound = GetCurrentFireSound(); break;
	case EFMWeaponMotion::Reload: Sound = Slot==EFMWeaponSlot::Secondary?PistolReloadOut.Get():GetWeaponData(Slot).Category==EFMWeaponCategory::Shotgun?ShotgunReloadOut.Get():ReloadSound.Get();
 GetWorld()->GetTimerManager().SetTimer(ReloadSoundTimer,this,&AFMCharacter::PlayReloadInsert,GetReloadDuration()*.65f,false);break;
	case EFMWeaponMotion::Melee: Sound = MeleeSound; break;
	case EFMWeaponMotion::Throw: Sound = ThrowSound; break;
	default: break;
	}
	if (Sound && IsLocallyControlled())
	{
		UGameplayStatics::PlaySound2D(this, Sound, 0.6f);
	}
	else if (Sound)
	{
		const APlayerController* Listener = GetWorld()->GetFirstPlayerController();
		const APawn* ListenerPawn = Listener ? Listener->GetPawn() : nullptr;
		const float Distance = ListenerPawn ? FVector::Distance(GetActorLocation(), ListenerPawn->GetActorLocation()) : 0.f;
		const float Volume = 0.55f * (1.f - FMath::Clamp(Distance / 15000.f, 0.f, 1.f));
		if (Volume > 0.f)
		{
			UGameplayStatics::PlaySoundAtLocation(this, Sound, GetActorLocation(), Volume, 1.f, 0.f, FireAttenuation);
		}
	}
}

USoundBase* AFMCharacter::GetCurrentFireSound() const
{
 const auto Category=GetWeaponData(WeaponSlot).Category;
 return Category==EFMWeaponCategory::Pistol?PistolShot.Get():Category==EFMWeaponCategory::Shotgun?ShotgunShot.Get():RifleShot.Get();
}
void AFMCharacter::PlayReloadInsert()
{
 if(!IsReloading() || Health<=0)return;
 const auto Category=GetWeaponData(WeaponSlot).Category;
 auto* S=Category==EFMWeaponCategory::Pistol?PistolReloadIn.Get():Category==EFMWeaponCategory::Shotgun?ShotgunReloadIn.Get():RifleReloadIn.Get();
 if(S){if(IsLocallyControlled())UGameplayStatics::PlaySound2D(this,S,.6f);else {const auto* PC=GetWorld()->GetFirstPlayerController();const APawn* P=PC?PC->GetPawn():nullptr;const float D=P?FVector::Distance(P->GetActorLocation(),GetActorLocation()):0;const float V=.45f*(1.f-FMath::Clamp(D/1800.f,0.f,1.f));if(V>0)UGameplayStatics::PlaySoundAtLocation(this,S,GetActorLocation(),V);}}
}

bool AFMCharacter::IsScopeViewActive() const
{
 return bAiming && ADSAlpha>.92f && WeaponSlot==EFMWeaponSlot::Primary && Health>0 &&
  (GetWeaponData(WeaponSlot).Category==EFMWeaponCategory::Sniper || GetActiveWeaponVisualRow().bScopeOverlay);
}
