#include "FMCharacter.h"
#include "FMGameState.h"
#include "FMGameMode.h"
#include "FMPlayerState.h"
#include "FMStructure.h"
#include "Camera/CameraComponent.h"
#include "EngineUtils.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Engine/World.h"

namespace
{
float SafeValue(float Value, float Fallback, float Minimum, float Maximum)
{
	return FMath::IsFinite(Value) ? FMath::Clamp(Value,Minimum,Maximum) : Fallback;
}
}

FFMWeaponRow AFMCharacter::GetWeaponData(EFMWeaponSlot Slot) const
{
	FFMWeaponRow Data;
	if (Slot == EFMWeaponSlot::Secondary)
	{
		Data.Category=EFMWeaponCategory::Pistol; Data.Damage = 22; Data.AttackInterval = .25f; Data.ReloadTime = 1.2f;
		Data.MagazineSize = 12; Data.ReserveAmmo = 36; Data.bAutomatic = false;
	}
	else if (Slot == EFMWeaponSlot::Melee || Slot == EFMWeaponSlot::Throwable)
	{
		Data.Damage = Slot == EFMWeaponSlot::Melee ? 50.f : 0.f;
		Data.AttackInterval = Slot == EFMWeaponSlot::Melee ? .6f : .8f;
		Data.Range = 175; Data.FalloffStart = 175;
		Data.HeadMultiplier = Data.LimbMultiplier = 1.f;
		Data.MagazineSize = Slot == EFMWeaponSlot::Melee ? 0 : 2;
		Data.ReserveAmmo = 0; Data.bAutomatic = false;
	}
	if (WeaponDataTable)
		if (const auto* Row = WeaponDataTable->FindRow<FFMWeaponRow>(GetWeaponId(Slot),TEXT("Weapon"),false)) Data = *Row;
	Data.PelletCount = FMath::Clamp(Data.PelletCount,1,16);
	Data.SpreadDegrees = SafeValue(Data.SpreadDegrees,0,0,10);
	Data.AimFOV = SafeValue(Data.AimFOV,65,20,85);
	Data.Damage = SafeValue(Data.Damage,28,0,1000);
	Data.HeadMultiplier = SafeValue(Data.HeadMultiplier,1,0,10);
	Data.LimbMultiplier = SafeValue(Data.LimbMultiplier,1,0,10);
	Data.Range = SafeValue(Data.Range,175,50,50000);
	Data.FalloffStart = SafeValue(Data.FalloffStart,Data.Range,0,Data.Range);
	Data.MinimumDamageFraction = SafeValue(Data.MinimumDamageFraction,1,0,1);
	Data.AttackInterval = SafeValue(Data.AttackInterval,.12f,.05f,5);
	Data.ReloadTime = SafeValue(Data.ReloadTime,1.55f,.1f,10);
	Data.EquipTime = SafeValue(Data.EquipTime,.28f,.05f,5);
	Data.MagazineSize = FMath::Clamp(Data.MagazineSize,0,300);
	Data.ReserveAmmo = FMath::Clamp(Data.ReserveAmmo,0,10000);
	return Data;
}

FFMArmorRow AFMCharacter::GetArmorRules() const {return GetCatalogArmorRules(ArmorId);}

FFMArmorRow AFMCharacter::GetCatalogArmorRules(FName Id) const
{
	FFMArmorRow Rules;
	if (ArmorDataTable)
		if (const auto* Row = ArmorDataTable->FindRow<FFMArmorRow>(Id,TEXT("Armor"),false)) Rules = *Row;
	Rules.MaximumArmor = SafeValue(Rules.MaximumArmor,50,0,1000);
	Rules.AbsorptionFraction = SafeValue(Rules.AbsorptionFraction,.5f,0,1);
	return Rules;
}

EFMHitZone AFMCharacter::HitZoneForBone(FName Bone)
{
	const FString Name = Bone.ToString().ToLower();
	if (Name == TEXT("head")) return EFMHitZone::Head;
	const TCHAR* LimbPrefixes[] = {TEXT("clavicle"),TEXT("upperarm"),TEXT("lowerarm"),TEXT("hand"),
		TEXT("thigh"),TEXT("calf"),TEXT("foot"),TEXT("ball"),TEXT("thumb"),TEXT("index"),
		TEXT("middle"),TEXT("ring"),TEXT("pinky")};
	for (const TCHAR* Prefix : LimbPrefixes) if (Name.StartsWith(Prefix)) return EFMHitZone::Limb;
	return EFMHitZone::Body;
}

void AFMCharacter::RefillArmor()
{
	const auto* State = GetWorld()->GetGameState<AFMGameState>();
	if (!HasAuthority() || Health <= 0 || !State) return;
	const EFMMatchPhase Phase = State->GetMatchSnapshot().Phase;
	if (Phase != EFMMatchPhase::Purchase && Phase != EFMMatchPhase::CombatTest) return;
	Armor = GetArmorRules().MaximumArmor;
	ForceNetUpdate();
}

void AFMCharacter::ServerEquipTestArmor_Implementation(FName Id)
{
 const auto* State=GetWorld()->GetGameState<AFMGameState>();
 const bool bTest = State && State->GetMatchSnapshot().Phase == EFMMatchPhase::CombatTest;
 if (Health <= 0 || (!bTest && !CanPurchase())) return;
 if(Id!=TEXT("LightArmor") && Id!=TEXT("MediumArmor"))return;
 if(!ArmorDataTable || !ArmorDataTable->FindRow<FFMArmorRow>(Id,TEXT("ArmorSelection"),false))return;
 if (!bTest)
 {
  auto* Member = GetPlayerState<AFMPlayerState>();
  const auto Rules = GetCatalogArmorRules(Id);
  if (!Member || (ArmorId == Id && Armor >= Rules.MaximumArmor) || !Member->SpendCredits(Rules.Price)) return;
 }
 ArmorId=Id;RefillArmor();ForceNetUpdate();
}

void AFMCharacter::ServerToggleTestArmor_Implementation()
{
	const auto* State = GetWorld()->GetGameState<AFMGameState>();
	if (!State || State->GetMatchSnapshot().Phase != EFMMatchPhase::CombatTest || Health <= 0) return;
	if (HasArmor()) Armor = 0;
	else RefillArmor();
	ForceNetUpdate();
}

FName AFMCharacter::GetWeaponId(EFMWeaponSlot Slot) const
{
 switch(Slot) {case EFMWeaponSlot::Primary:return PrimaryWeaponId;case EFMWeaponSlot::Secondary:return SecondaryWeaponId;case EFMWeaponSlot::Melee:return TEXT("Knife");default:return TEXT("Grenade");}
}
TArray<FName> AFMCharacter::GetCatalogWeaponIds() const
{
 TArray<FName> Ids=WeaponDataTable?WeaponDataTable->GetRowNames():TArray<FName>();
 Ids.RemoveAll([this](const FName& Id){return !GetCatalogWeaponData(Id).bInCatalog || Id==TEXT("Knife");});
 Ids.Sort([this](const FName& A,const FName& B){const auto X=GetCatalogWeaponData(A),Y=GetCatalogWeaponData(B);return X.Price!=Y.Price?X.Price<Y.Price:A.LexicalLess(B);});
 return Ids;
}
FFMWeaponRow AFMCharacter::GetCatalogWeaponData(FName Id) const
{
 if(WeaponDataTable)if(const auto* Row=WeaponDataTable->FindRow<FFMWeaponRow>(Id,TEXT("Catalog"),false))return *Row;
 return FFMWeaponRow();
}
void AFMCharacter::ServerSelectCatalogWeapon_Implementation(FName Id)
{
 const auto* State=GetWorld()->GetGameState<AFMGameState>();
 const bool bTest = State && State->GetMatchSnapshot().Phase == EFMMatchPhase::CombatTest;
 if ((!bTest && !CanPurchase()) || Health <= 0 || !WeaponDataTable) return;
 const auto* Row=WeaponDataTable->FindRow<FFMWeaponRow>(Id,TEXT("Selection"),false);
 if(!GetCatalogWeaponVisual(Id).bAvailable)return;
 if (!Row || !Row->bInCatalog || Row->Category == EFMWeaponCategory::Melee) return;
 auto* Member = GetPlayerState<AFMPlayerState>();
 if (Row->Category == EFMWeaponCategory::Grenade)
 {
  if (!bTest && Member && Grenades < 2 && Member->SpendCredits(Row->Price)) { ++Grenades; ForceNetUpdate(); }
  return;
 }
 const auto Slot=Row->Category==EFMWeaponCategory::Pistol?EFMWeaponSlot::Secondary:EFMWeaponSlot::Primary;
 FName& Current=Slot==EFMWeaponSlot::Primary?PrimaryWeaponId:SecondaryWeaponId;
 if(Current==Id){ServerEquipWeapon_Implementation(Slot);return;}
 if (!bTest && (!Member || !Member->SpendCredits(Row->Price))) return;
 CancelWeaponActions();Current=Id;WeaponSlot=Slot;
 const auto Data=GetWeaponData(Slot);
 if(Slot==EFMWeaponSlot::Primary){PrimaryAmmo=Data.MagazineSize;PrimaryReserve=Data.ReserveAmmo;}
 else{SecondaryAmmo=Data.MagazineSize;SecondaryReserve=Data.ReserveAmmo;}
 NextAttackTime=GetWorld()->GetTimeSeconds()+Data.EquipTime;
 SetWeaponMotion(EFMWeaponMotion::Equip);OnRep_WeaponSlot();ForceNetUpdate();
}

FFMWeaponVisualRow AFMCharacter::GetWeaponVisualData() const
{
	return GetActiveWeaponVisualRow();
}

const FFMWeaponVisualRow& AFMCharacter::GetActiveWeaponVisualRow() const
{
	if (WeaponVisualDataTable)
	{
		if (const auto* Row = WeaponVisualDataTable->FindRow<FFMWeaponVisualRow>(GetWeaponId(WeaponSlot), TEXT("Presentation"), false))
		{
			return *Row;
		}
	}
	static const FFMWeaponVisualRow Fallback;
	return Fallback;
}
FFMWeaponVisualRow AFMCharacter::GetCatalogWeaponVisual(FName Id) const
{
 if(WeaponVisualDataTable)if(const auto* Row=WeaponVisualDataTable->FindRow<FFMWeaponVisualRow>(Id,TEXT("Presentation"),false))return *Row;
 return FFMWeaponVisualRow();
}

bool AFMCharacter::CanPurchase() const
{
	const auto* State = GetWorld()->GetGameState<AFMGameState>();
	const auto* Mode = GetWorld()->GetAuthGameMode<AFMGameMode>();
	return HasAuthority() && Health > 0.f && State && Mode && State->GetMatchSnapshot().bPlayerRoom &&
		State->GetMatchSnapshot().Phase == EFMMatchPhase::Purchase && State->GetPhaseTimeRemaining() > 0.f &&
		Mode->IsInPreparationZone(GetController(), GetActorLocation(), true);
}

void AFMCharacter::ResetForRound(bool bResetEquipment)
{
	if (!HasAuthority()) return;
	const bool bLostEquipment = bResetEquipment || Health <= 0.f;
	CancelWeaponActions();
	if (bLostEquipment)
	{
		PrimaryWeaponId = NAME_None;
		SecondaryWeaponId = NAME_None;
		Armor = 0;
		Grenades = 0;
		WeaponSlot = EFMWeaponSlot::Melee;
	}
	PrimaryAmmo = PrimaryWeaponId.IsNone() ? 0 : GetWeaponData(EFMWeaponSlot::Primary).MagazineSize;
	PrimaryReserve = PrimaryWeaponId.IsNone() ? 0 : GetWeaponData(EFMWeaponSlot::Primary).ReserveAmmo;
	SecondaryAmmo = SecondaryWeaponId.IsNone() ? 0 : GetWeaponData(EFMWeaponSlot::Secondary).MagazineSize;
	SecondaryReserve = SecondaryWeaponId.IsNone() ? 0 : GetWeaponData(EFMWeaponSlot::Secondary).ReserveAmmo;
	Health = MaxHealth;
	bSprinting = false;
	bAiming = false;
	NextAttackTime = 0;
	SetWeaponMotion(EFMWeaponMotion::Ready);
	SetActorHiddenInGame(false);
	SetActorEnableCollision(true);
	GetCharacterMovement()->SetMovementMode(MOVE_Walking);
	OnRep_WeaponSlot();
	ForceNetUpdate();
}

void AFMCharacter::ServerBuyStructure_Implementation(int32 Index)
{
	const auto* Item = AFMStructure::GetItem(Index);
	auto* Member = GetPlayerState<AFMPlayerState>();
	if (!Item || !Member || !CanPurchase() || Member->GetStructureCount(Index) >= 3 || !Member->SpendCredits(Item->Price)) return;
	Member->ChangeStructureStock(Index, 1);
}

void AFMCharacter::ServerPlaceStructure_Implementation(int32 Index)
{
	const auto* Item = AFMStructure::GetItem(Index);
	auto* Member = GetPlayerState<AFMPlayerState>();
	const auto* State = GetWorld()->GetGameState<AFMGameState>();
	const auto* Mode = GetWorld()->GetAuthGameMode<AFMGameMode>();
	if (!Item || !Member || Health <= 0 || Member->GetTeam() >= 2 || Member->GetStructureCount(Index) <= 0 ||
		!State || !Mode || State->GetMatchSnapshot().Phase != EFMMatchPhase::Building ||
		State->GetPhaseTimeRemaining() <= 0.f || !Mode->IsInPreparationZone(GetController(), GetActorLocation(), false)) return;

	int32 Placed = 0;
	for (TActorIterator<AFMStructure> It(GetWorld()); It; ++It) if (++Placed >= 40) return;

	const FVector Eye = FirstPersonCamera->GetComponentLocation();
	const FCollisionQueryParams Query(SCENE_QUERY_STAT(FMStructurePlacement), false, this);
	FCollisionObjectQueryParams Ground(ECC_WorldStatic);
	FHitResult Hit;
	if (!GetWorld()->LineTraceSingleByObjectType(Hit, Eye, Eye + GetBaseAimRotation().Vector() * 500.f, Ground, Query) ||
		Hit.ImpactNormal.Z < 0.95f) return;

	const FRotator Facing(0, FMath::GridSnap(GetActorRotation().Yaw + 90.f, 15.f), 0);
	FVector Floor = Hit.ImpactPoint;
	Floor.X = FMath::GridSnap(Floor.X, 25.f);
	Floor.Y = FMath::GridSnap(Floor.Y, 25.f);
	if (!Mode->IsInPreparationZone(GetController(), Floor, false) || Mode->IsInPreparationZone(GetController(), Floor, true)) return;

	for (const float X : {-1.f, 1.f})
	{
		for (const float Y : {-1.f, 1.f})
		{
			const FVector Corner = Floor + Facing.RotateVector(FVector(Item->HalfSize.X * X, Item->HalfSize.Y * Y, 0));
			FHitResult Support;
			if (!Mode->IsInPreparationZone(GetController(), Corner, false) || Mode->IsInPreparationZone(GetController(), Corner, true) ||
				!GetWorld()->LineTraceSingleByObjectType(Support, Corner + FVector(0, 0, 15), Corner - FVector(0, 0, 15), Ground, Query) ||
				Support.ImpactNormal.Z < 0.95f) return;
		}
	}
	const FVector Center = Floor + FVector(0, 0, Item->HalfSize.Z + 2.f);
	FCollisionObjectQueryParams Blockers = Ground;
	Blockers.AddObjectTypesToQuery(ECC_WorldDynamic);
	Blockers.AddObjectTypesToQuery(ECC_Pawn);
	if (GetWorld()->OverlapAnyTestByObjectType(Center, Facing.Quaternion(), Blockers, FCollisionShape::MakeBox(Item->HalfSize))) return;
	const FTransform Transform(Facing, Center);
	auto* Structure = GetWorld()->SpawnActorDeferred<AFMStructure>(AFMStructure::StaticClass(), Transform);
	if (!Structure) return;
	Structure->Initialize(Index, Member->GetTeam());
	Structure->FinishSpawning(Transform);
	Member->ChangeStructureStock(Index, -1);
}
