#include "FMCombatHUD.h"

#include "Engine/Canvas.h"
#include "Engine/Texture2D.h"
#include "UObject/ConstructorHelpers.h"
#include "Engine/Engine.h"
#include "FMCharacter.h"
#include "FMPlayerState.h"
#include "FMGameState.h"
#include "FMPlayerController.h"
#include "FMStructure.h"
#include "GameFramework/GameStateBase.h"

namespace
{
const TCHAR* PhaseLabel(EFMMatchPhase Phase)
{
	switch (Phase)
	{
	case EFMMatchPhase::Deploying: return TEXT("DEPLOYING");
	case EFMMatchPhase::Ready: return TEXT("READY");
	case EFMMatchPhase::KnifePreparation: return TEXT("KNIFE PREP");
	case EFMMatchPhase::KnifeCombat: return TEXT("KNIFE");
	case EFMMatchPhase::Purchase: return TEXT("BUY");
	case EFMMatchPhase::Building: return TEXT("BUILD");
	case EFMMatchPhase::Countdown: return TEXT("COUNTDOWN");
	case EFMMatchPhase::Combat: return TEXT("COMBAT");
	case EFMMatchPhase::CombatTest: return TEXT("FIRE TEST");
	case EFMMatchPhase::Result: return TEXT("RESULT");
	case EFMMatchPhase::SideSwap: return TEXT("SIDE SWAP");
	case EFMMatchPhase::Finished: return TEXT("FINISHED");
	default: return TEXT("WAITING");
	}
}
}

AFMCombatHUD::AFMCombatHUD()
{
 static ConstructorHelpers::FObjectFinder<UTexture2D> Mask(TEXT("/Game/FieldMaker/Weapons/T_FMScopeMask.T_FMScopeMask"));
 ScopeMask=Mask.Object;
}

void AFMCombatHUD::DrawHUD()
{
	Super::DrawHUD();

	const auto* Character = GetOwningPawn() ? Cast<AFMCharacter>(GetOwningPawn()) : nullptr;
	if (!Canvas || !Character)
	{
		return;
	}

 const bool bScoped=Character->IsScopeViewActive();
 if(bScoped && ScopeMask)
 {
  const float W=Canvas->ClipX,H=Canvas->ClipY,D=FMath::Min(W,H)*.94f;
  const float X=(W-D)*.5f,Y=(H-D)*.5f;
  DrawRect(FLinearColor::Black,0,0,X,H);DrawRect(FLinearColor::Black,X+D,0,W-X-D,H);
  DrawRect(FLinearColor::Black,X,0,D,Y);DrawRect(FLinearColor::Black,X,Y+D,D,H-Y-D);
  DrawTexture(ScopeMask,X,Y,D,D,0,0,1,1,FLinearColor::White,BLEND_Translucent);
  DrawLine(W*.5f,Y+8,W*.5f,Y+D-8,FLinearColor::Black,1.5f);
  DrawLine(X+8,H*.5f,X+D-8,H*.5f,FLinearColor::Black,1.5f);
 }

	const float Scale = FMath::Min(Canvas->ClipX / 960.f, Canvas->ClipY / 540.f);
	const float Width = Canvas->ClipX / Scale;
	const float Height = Canvas->ClipY / Scale;
	const FLinearColor Ink(0.012f, 0.017f, 0.022f, 0.84f);
	const FLinearColor White(0.94f, 0.96f, 1.f);
	const FLinearColor Gold(0.95f, 0.67f, 0.27f);
	const FLinearColor Blue(0.35f, 0.76f, 1.f);
	const FLinearColor Red(1.f, 0.39f, 0.35f);
	UFont* Font = GEngine->GetMediumFont();

	const auto Box = [&](FLinearColor Color, float X, float Y, float W, float H)
	{
		DrawRect(Color, X * Scale, Y * Scale, W * Scale, H * Scale);
	};
	const auto Label = [&](const FString& Text, FLinearColor Color, float X, float Y, float Size = 1.f, bool bCentered = false)
	{
		if (bCentered)
		{
			float TextWidth, TextHeight;
			GetTextSize(Text, TextWidth, TextHeight, Font, Size);
			X -= TextWidth * 0.5f;
		}
		DrawText(Text, Color, X * Scale, Y * Scale, Font, Size * Scale);
	};

	int32 Teams[2] = {};
	if (const auto* State = GetWorld()->GetGameState())
	{
		for (const APlayerState* Player : State->PlayerArray)
		{
			if (const auto* Member = Cast<AFMPlayerState>(Player);
				Member && Member->GetTeam() < AFMPlayerState::UnassignedTeam)
			{
				++Teams[Member->GetTeam()];
			}
		}
	}

	const float Center = Width * 0.5f;
	Box(Ink, Center - 85.f, 15.f, 170.f, 48.f);
	Box(Blue, Center - 85.f, 15.f, 85.f, 2.f);
	Box(Red, Center, 15.f, 85.f, 2.f);
	const auto* MatchState = GetWorld()->GetGameState<AFMGameState>();
	const FFMMatchSnapshot Match = MatchState ? MatchState->GetMatchSnapshot() : FFMMatchSnapshot();
	const auto* Controller = Cast<AFMPlayerController>(GetOwningPlayerController());
	const bool bBuildMode = Controller && Controller->IsBuildMode();
	const bool bMatch = Match.Phase != EFMMatchPhase::Practice;
	const FString Header = !bMatch ? (Teams[1] == 0 ? TEXT("PRACTICE") : TEXT("TEAM SIZE"))
		: FString::Printf(TEXT("ROUND %d | %s"), Match.RoundNumber,
			PhaseLabel(Match.Phase));
	Label(Header, White, Center, 19.f, 0.67f, true);
	Label(FString::FromInt(bMatch ? Match.Team0Score : Teams[0]), Blue, Center - 43.f, 35.f, 1.15f, true);
	Label(TEXT("VS"), White, Center, 38.f, 0.8f, true);
	Label(FString::FromInt(bMatch ? Match.Team1Score : Teams[1]), Red, Center + 43.f, 35.f, 1.15f, true);
	if (bMatch)
	{
		const auto* Player = GetOwningPlayerController()->GetPlayerState<AFMPlayerState>();
		const FString Time = Match.PhaseEndServerTime > 0.0
			? FString::FromInt(FMath::CeilToInt(MatchState->GetPhaseTimeRemaining())) : TEXT("--");
		Label(FString::Printf(TEXT("TEAM %s | PLAYERS %d:%d | TIME %s"),
			Player && Player->GetTeam() < 2 ? (Player->GetTeam() == 0 ? TEXT("BLUE") : TEXT("RED")) : TEXT("--"),
			Teams[0], Teams[1], *Time), White, Center, 69.f, 0.62f, true);
		Label(Character->IsSprinting() ? TEXT("SPRINT - ATTACK LOCKED") : Character->IsAiming() ? TEXT("ADS") :
			Match.Phase == EFMMatchPhase::Ready || Match.Phase == EFMMatchPhase::WaitingForPlayers
			? TEXT("WAITING FOR PLAYERS") : Match.Phase == EFMMatchPhase::CombatTest ? TEXT("TEAM COMBAT TEST")
			: Match.Phase == EFMMatchPhase::Purchase ? TEXT("B  WEAPON / STRUCTURE SHOP")
			: Match.Phase == EFMMatchPhase::Building ? (bBuildMode ? TEXT("LMB PLACE  |  R ROTATE  |  C EXIT") : TEXT("C  BUILD MODE"))
			: (Match.Phase == EFMMatchPhase::Result || Match.Phase == EFMMatchPhase::Finished)
				? (Match.RoundWinner < 0 ? TEXT("DRAW / NO WINNER") : Match.RoundWinner == 0 ? TEXT("BLUE WINS") : TEXT("RED WINS"))
				: TEXT(""), Gold, Center, 82.f, 0.58f, true);
		if (Match.Phase == EFMMatchPhase::Finished)
			Label(TEXT("F10  RETURN TO MENU"), Gold, Center, 96.f, 0.62f, true);
		if (Player && Match.bPlayerRoom)
			Label(FString::Printf(TEXT("$%d"), Player->GetCredits()), Gold, 30.f, Height - 113.f, 0.65f);
	}

	const float Health = Character->GetHealth();
	const float Armor = Character->GetArmor();
	const float MaximumArmor = Character->GetArmorRules().MaximumArmor;
	Box(Ink,20.f,Height - 98.f,146.f,27.f);
	Label(Character->HasArmor() ? FString::Printf(TEXT("ARMOR %d / %d"),FMath::CeilToInt(Armor),FMath::CeilToInt(MaximumArmor))
		: TEXT("ARMOR  NONE"),Character->HasArmor() ? Gold : White,30.f,Height - 95.f,.70f);
	if (MaximumArmor > 0) Box(Gold,30.f,Height - 77.f,126.f * Armor / MaximumArmor,2.f);
	if (Match.Phase == EFMMatchPhase::CombatTest)
		Label(TEXT("V  TEST ARMOR ON / OFF"),Gold,30.f,Height - 111.f,.58f);
	const FLinearColor HealthColor = Health <= 25.f ? Red : Blue;
	Box(Ink, 20.f, Height - 67.f, 146.f, 47.f);
	Label(TEXT("HEALTH"), HealthColor, 30.f, Height - 62.f, 0.74f);
	Label(FString::Printf(TEXT("%d / %d"), FMath::CeilToInt(Health), int32(AFMCharacter::MaxHealth)),
		White, 30.f, Height - 48.f, 1.05f);
	Box(FLinearColor(0.16f, 0.19f, 0.24f), 30.f, Height - 29.f, 126.f, 3.f);
	Box(HealthColor, 30.f, Height - 29.f, 126.f * Health / AFMCharacter::MaxHealth, 3.f);

	const int32 Slot = bBuildMode ? Controller->GetBuildSelection() : static_cast<int32>(Character->GetWeaponSlot());
	if (bBuildMode)
	{
		const auto* Item = AFMStructure::GetItem(Slot);
		FTransform Placement;
		AFMStructure* Support = nullptr;
		const bool bValid = Item && Character->FindStructurePlacement(Slot, Controller->GetBuildRotation(), Placement, Support);
		const FLinearColor Cyan(0.f, 0.95f, 1.f);
		Label(bValid ? TEXT("PLACE") : TEXT("CANNOT PLACE"), bValid ? Cyan : Red, Center, Height * 0.5f + 18.f, 0.66f, true);
		if (bValid)
		{
			FVector2D Screen[8];
			bool bProjected = true;
			for (int32 Corner = 0; Corner < 8; ++Corner)
			{
				const FVector Sign(Corner & 1 ? 1 : -1, Corner & 2 ? 1 : -1, Corner & 4 ? 1 : -1);
				bProjected &= Controller->ProjectWorldLocationToScreen(Placement.TransformPosition(Item->HalfSize * Sign), Screen[Corner]);
			}
			if (bProjected)
			{
				for (int32 Corner = 0; Corner < 8; ++Corner)
					for (const int32 Axis : {1, 2, 4})
						if (!(Corner & Axis))
							DrawLine(Screen[Corner].X, Screen[Corner].Y, Screen[Corner | Axis].X, Screen[Corner | Axis].Y, Cyan, 1.5f);
			}
		}
	}
	const TCHAR* Slots[] = {TEXT("1  PRIMARY"), TEXT("2  SECONDARY"), TEXT("3  MELEE"), TEXT("4  THROWABLE")};
	for (int32 Index = 0; Index < UE_ARRAY_COUNT(Slots); ++Index)
	{
		const float X = Center - 202.f + Index * 102.f;
		Box(Ink, X, Height - 41.f, 100.f, 21.f);
		if (Slot == Index)
		{
			Box(Gold, X, Height - 41.f, 100.f, 2.f);
		}
		const auto* Member = Controller ? Controller->GetPlayerState<AFMPlayerState>() : nullptr;
		const TCHAR* BuildNames[] = {TEXT("SANDBAGS"), TEXT("WALL"), TEXT("CRATE"), TEXT("CONTAINER")};
		const FString SlotText = bBuildMode
			? FString::Printf(TEXT("%d %s [%d]"), Index + 1, BuildNames[Index], Member ? Member->GetStructureCount(Index) : 0)
			: FString(Slots[Index]);
		Label(SlotText, Slot == Index ? Gold : White, X + 50.f, Height - 37.f, 0.72f, true);
	}
	Box(Ink, Width - 166.f, Height - 67.f, 146.f, 47.f);
	const bool bFirearm = Slot <= static_cast<int32>(EFMWeaponSlot::Secondary);
	const TCHAR* AmmoLabel = Character->IsReloading() ? TEXT("RELOADING") : bFirearm ? TEXT("AMMO") : Slots[Slot];
	Label(bFirearm && !Character->IsReloading() ? Character->GetWeaponData(Character->GetWeaponSlot()).DisplayName.ToString() : FString(AmmoLabel), Gold, Width - 156.f, Height - 62.f, 0.74f);
	const FString Ammo = bFirearm
		? FString::Printf(TEXT("%02d / %02d"), Character->GetLoadedAmmo(), Character->GetReserveAmmo())
		: Slot == static_cast<int32>(EFMWeaponSlot::Throwable) ? FString::FromInt(Character->GetLoadedAmmo()) : TEXT("READY");
	Label(Ammo, White, Width - 156.f, Height - 47.f, 1.25f);

	const float AimY = Height * 0.5f;
	if (Health > 0.f && !bScoped)
	{
		Box(Ink, Center - 2.f, AimY - 2.f, 4.f, 4.f);
		Box(White, Center - 1.f, AimY - 1.f, 2.f, 2.f);
		if (!Character->IsAiming())
		{
			Box(White, Center - 9.f, AimY - 0.5f, 5.f, 1.f);
			Box(White, Center + 4.f, AimY - 0.5f, 5.f, 1.f);
			Box(White, Center - 0.5f, AimY - 9.f, 1.f, 5.f);
			Box(White, Center - 0.5f, AimY + 4.f, 1.f, 5.f);
		}
	}
	else if(Health<=0.f)
	{
		Box(Ink, Center - 80.f, AimY - 17.f, 160.f, 34.f);
		Label(TEXT("ELIMINATED"), Red, Center, AimY - 10.f, 1.2f, true);
	}
}
