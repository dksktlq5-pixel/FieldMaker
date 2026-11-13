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
#include "Components/StaticMeshComponent.h"
#include "EngineUtils.h"

namespace
{
const TCHAR* ResultLabel(EFMResultReason Reason)
{
	switch (Reason)
	{
	case EFMResultReason::Elimination: return TEXT("TEAM ELIMINATED");
	case EFMResultReason::DoubleElimination: return TEXT("BOTH TEAMS ELIMINATED");
	case EFMResultReason::Capture: return TEXT("CENTER CAPTURED");
	case EFMResultReason::TimeExpired: return TEXT("TIME EXPIRED");
	case EFMResultReason::TeamLeft: return TEXT("TEAM DISCONNECTED");
	default: return TEXT("");
	}
}

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
 static const TCHAR* IconNames[] = {TEXT("Pistol"),TEXT("SMG"),TEXT("Rifle"),TEXT("Sniper"),TEXT("MachineGun"),TEXT("Shotgun"),TEXT("Knife"),TEXT("Grenade")};
 for (const TCHAR* Name : IconNames)
 {
  const FString Path = FString::Printf(TEXT("/Game/FieldMaker/UI/WeaponIcons/T_UIWeapon_%s.T_UIWeapon_%s"), Name, Name);
  WeaponIcons.Add(LoadObject<UTexture2D>(nullptr, *Path));
 }
 static const TCHAR* ItemNames[] = {
  TEXT("Glock"),TEXT("Pistol"),TEXT("P250"),TEXT("FiveSeven"),TEXT("Deagle"),
  TEXT("MAC10"),TEXT("UMP45"),TEXT("MP5SD"),TEXT("MP7"),TEXT("P90"),
  TEXT("Nova"),TEXT("Negev"),TEXT("Grenade"),TEXT("Smoke"),TEXT("Fire"),TEXT("Flash"),TEXT("Decoy")};
 for(const TCHAR* Name : ItemNames)
 {
  const FString Path=FString::Printf(TEXT("/Game/FieldMaker/Weapons/BlenderModeled/Icons/T_UIWeapon_%s.T_UIWeapon_%s"),Name,Name);
  if(UTexture2D* Texture=LoadObject<UTexture2D>(nullptr,*Path))ItemIcons.Add(FName(Name),Texture);
 }
}

void AFMCombatHUD::BeginPlay()
{
	Super::BeginPlay();
	MinimapFootprints.Reset();
	MinimapWorldBounds = FBox(ForceInit);
	if (!GetWorld()) return;
	for (TActorIterator<AActor> It(GetWorld()); It; ++It)
	{
		if (It->IsA<AFMCharacter>() || It->IsA<AFMStructure>()) continue;
		TArray<UStaticMeshComponent*> Components;
		It->GetComponents<UStaticMeshComponent>(Components);
		for (const UStaticMeshComponent* Component : Components)
		{
			if (!Component || Component->GetMobility() != EComponentMobility::Static ||
				Component->GetCollisionEnabled() == ECollisionEnabled::NoCollision || !Component->IsVisible()) continue;
			const FBox Bounds = Component->Bounds.GetBox();
			const FVector Extent = Bounds.GetExtent();
			if (!Bounds.IsValid || Extent.X < 65.f || Extent.Y < 65.f || Extent.X > 1400.f || Extent.Y > 1400.f || Extent.Z > 1600.f) continue;
			MinimapFootprints.Add(Bounds);
			MinimapWorldBounds += FVector(Bounds.Min.X, Bounds.Min.Y, 0.f);
			MinimapWorldBounds += FVector(Bounds.Max.X, Bounds.Max.Y, 0.f);
		}
	}
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
	const FLinearColor Ink(0.012f, 0.017f, 0.022f, 0.56f);
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
	int32 AliveTeams[2] = {};
	if (const auto* State = GetWorld()->GetGameState())
	{
		for (const APlayerState* Player : State->PlayerArray)
		{
			if (const auto* Member = Cast<AFMPlayerState>(Player);
				Member && Member->GetTeam() < AFMPlayerState::UnassignedTeam)
			{
				++Teams[Member->GetTeam()];
				if (const auto* Pawn = Cast<AFMCharacter>(Member->GetPawn()); Pawn && Pawn->GetHealth() > 0.f)
					++AliveTeams[Member->GetTeam()];
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
		const FString Time = Match.PhaseEndServerTime > 0.0
			? FString::FromInt(FMath::CeilToInt(MatchState->GetPhaseTimeRemaining())) : TEXT("--");
		Label(FString::Printf(TEXT("ALIVE %d"), AliveTeams[0]), Blue, Center - 92.f, 69.f, 0.62f, true);
		Label(FString::Printf(TEXT("ALIVE %d"), AliveTeams[1]), Red, Center + 92.f, 69.f, 0.62f, true);
		Label(FString::Printf(TEXT("%s  |  %s"), Match.Phase == EFMMatchPhase::Combat ? TEXT("LIVE") : PhaseLabel(Match.Phase), *Time), White, Center, 69.f, 0.62f, true);
		Label(Character->IsSprinting() ? TEXT("SPRINT - ADS LOCKED") : Character->IsAiming() ? TEXT("ADS") :
			Match.Phase == EFMMatchPhase::Ready || Match.Phase == EFMMatchPhase::WaitingForPlayers
			? TEXT("WAITING FOR PLAYERS") : Match.Phase == EFMMatchPhase::CombatTest ? TEXT("TEAM COMBAT TEST")
			: Match.Phase == EFMMatchPhase::Purchase ? TEXT("B  WEAPON / STRUCTURE SHOP")
			: Match.Phase == EFMMatchPhase::Building ? (bBuildMode ? TEXT("LMB PLACE  |  R ROTATE  |  C EXIT") : TEXT("C  BUILD MODE"))
			: (Match.Phase == EFMMatchPhase::Result || Match.Phase == EFMMatchPhase::Finished)
				? (Match.RoundWinner < 0 ? TEXT("DRAW / NO WINNER") : Match.RoundWinner == 0 ? TEXT("BLUE WINS") : TEXT("RED WINS"))
				: TEXT(""), Gold, Center, 82.f, 0.58f, true);
		if (Match.Phase == EFMMatchPhase::Result || Match.Phase == EFMMatchPhase::Finished)
			Label(ResultLabel(Match.ResultReason), White, Center, 96.f, 0.65f, true);
		if (Match.Phase == EFMMatchPhase::Finished)
			Label(TEXT("F10  RETURN TO MENU"), Gold, Center, 110.f, 0.62f, true);
	}

	const auto* LocalMember = GetOwningPlayerController()
		? GetOwningPlayerController()->GetPlayerState<AFMPlayerState>() : nullptr;
	if (LocalMember)
	{
		Label(FString::Printf(TEXT("$%d"), LocalMember->GetCredits()), Gold, 22.f, Height - 40.f, 0.92f);
	}

	// Small, transparent top-down map. Static collision footprints provide full-level map geometry.
	const float RadarSize = 118.f * Scale;
	const float RadarCx = 79.f * Scale;
	const float RadarCy = 82.f * Scale;
	const float RadarRadius = RadarSize * 0.48f;
	const FVector MapCenter = MinimapWorldBounds.IsValid ? MinimapWorldBounds.GetCenter() : Character->GetActorLocation();
	const FVector MapExtent = MinimapWorldBounds.IsValid ? MinimapWorldBounds.GetExtent() : FVector(3000.f, 3000.f, 0.f);
	const float MapSpan = FMath::Max(FMath::Max(MapExtent.X, MapExtent.Y) * 2.15f, 1.f);
	const FLinearColor MapLine(0.94f, 0.68f, 0.34f, 0.78f);
	const auto ToRadar = [&](const FVector& WorldPoint)
	{
		const FVector Relative = WorldPoint - MapCenter;
		return FVector2D(RadarCx + Relative.X / MapSpan * (RadarRadius * 2.f),
			RadarCy - Relative.Y / MapSpan * (RadarRadius * 2.f));
	};
	const auto InRadar = [&](const FVector2D& Point)
	{
		return FVector2D::DistSquared(Point, FVector2D(RadarCx, RadarCy)) <= FMath::Square(RadarRadius);
	};
	const auto MapLineIfVisible = [&](const FVector2D& A, const FVector2D& B)
	{
		if (InRadar(A) || InRadar(B)) DrawLine(A.X, A.Y, B.X, B.Y, MapLine, 1.f);
	};
	for (const FBox& Bounds : MinimapFootprints)
	{
		if (!Bounds.IsValid) continue;
		const FVector P0(Bounds.Min.X, Bounds.Min.Y, MapCenter.Z);
		const FVector P1(Bounds.Max.X, Bounds.Min.Y, MapCenter.Z);
		const FVector P2(Bounds.Max.X, Bounds.Max.Y, MapCenter.Z);
		const FVector P3(Bounds.Min.X, Bounds.Max.Y, MapCenter.Z);
		const FVector2D A = ToRadar(P0), B = ToRadar(P1), C = ToRadar(P2), D = ToRadar(P3);
		MapLineIfVisible(A, B); MapLineIfVisible(B, C); MapLineIfVisible(C, D); MapLineIfVisible(D, A);
	}
	const auto DrawRadarRing = [&](float Radius, FLinearColor Color)
	{
		constexpr int32 Segments = 40;
		for (int32 Index = 0; Index < Segments; ++Index)
		{
			const float A0 = 2.f * PI * Index / Segments;
			const float A1 = 2.f * PI * (Index + 1) / Segments;
			DrawLine(RadarCx + FMath::Cos(A0) * Radius, RadarCy + FMath::Sin(A0) * Radius,
				RadarCx + FMath::Cos(A1) * Radius, RadarCy + FMath::Sin(A1) * Radius, Color, 0.8f);
		}
	};
	DrawRadarRing(RadarRadius, FLinearColor(1.0f, 0.72f, 0.34f, 0.52f));
	DrawRadarRing(RadarRadius * 0.52f, FLinearColor(0.24f, 0.82f, 0.92f, 0.38f));
	if (LocalMember && Character->GetController())
	{
		const auto* WorldState = GetWorld()->GetGameState();
		if (WorldState)
		{
			const FVector CameraLocation = Character->GetPawnViewLocation();
			for (const APlayerState* Player : WorldState->PlayerArray)
			{
				const auto* OtherMember = Cast<AFMPlayerState>(Player);
				const auto* OtherPawn = OtherMember ? Cast<AFMCharacter>(OtherMember->GetPawn()) : nullptr;
				if (!OtherMember || OtherMember == LocalMember || !OtherPawn || OtherPawn->GetHealth() <= 0.f) continue;
				FLinearColor DotColor;
				if (OtherMember->GetTeam() == LocalMember->GetTeam()) DotColor = Blue;
				else
				{
					const FVector Target = OtherPawn->GetActorLocation() + FVector(0, 0, 65.f);
					FVector2D TargetScreen;
					if (!Controller || !Controller->ProjectWorldLocationToScreen(Target, TargetScreen, false) ||
						TargetScreen.X < 0.f || TargetScreen.X > Canvas->ClipX || TargetScreen.Y < 0.f || TargetScreen.Y > Canvas->ClipY ||
						!Controller->LineOfSightTo(OtherPawn, CameraLocation, true)) continue;
					DotColor = Red;
				}
				const FVector2D Point = ToRadar(OtherPawn->GetActorLocation());
				if (!InRadar(Point)) continue;
				DrawRect(DotColor, Point.X - 2.f * Scale, Point.Y - 2.f * Scale, 4.f * Scale, 4.f * Scale);
			}
			const FVector2D PlayerPoint = ToRadar(Character->GetActorLocation());
			const FVector Heading = Character->GetControlRotation().Vector();
			const FVector2D Nose = ToRadar(Character->GetActorLocation() + Heading * 260.f);
			const FVector2D Left = ToRadar(Character->GetActorLocation() + FVector(-Heading.Y, Heading.X, 0.f) * 100.f);
			const FVector2D Right = ToRadar(Character->GetActorLocation() + FVector(Heading.Y, -Heading.X, 0.f) * 100.f);
			const FLinearColor PlayerColor(0.25f, 0.96f, 1.0f);
			DrawLine(PlayerPoint.X, PlayerPoint.Y, Nose.X, Nose.Y, PlayerColor, 1.4f);
			DrawLine(Nose.X, Nose.Y, Left.X, Left.Y, PlayerColor, 1.4f);
			DrawLine(Left.X, Left.Y, Right.X, Right.Y, PlayerColor, 1.4f);
			DrawLine(Right.X, Right.Y, Nose.X, Nose.Y, PlayerColor, 1.4f);
		}
	}

	if (MatchState && Match.bPlayerRoom)
	{
		const float ServerNow = MatchState->GetServerWorldTimeSeconds();
		int32 VisibleRows = 0;
		for (const FFMKillFeedEntry& Entry : MatchState->GetKillFeed())
		{
			if (ServerNow - Entry.ServerTime > 8.f || VisibleRows >= 5) continue;
			const float Y = 24.f + VisibleRows * 27.f;
			const FString Row = FString::Printf(TEXT("%s   >   %s   %s"), *Entry.Killer, *Entry.Weapon, *Entry.Victim);
			float TextWidth = 0.f, TextHeight = 0.f;
			GetTextSize(Row, TextWidth, TextHeight, Font, 0.67f * Scale);
			const float X = FMath::Max(Width - TextWidth / Scale - 18.f, Width * 0.57f);
			DrawLine(X, (Y + 18.f) * Scale, X, (Y + 22.f) * Scale, Red, 2.f);
			Label(Row, White, X + 6.f, Y + 3.f, 0.67f);
			++VisibleRows;
		}
	}

	if (Match.Phase == EFMMatchPhase::Combat && Match.bPlayerRoom)
	{
		const FLinearColor CaptureColor = Match.bCaptureContested ? Gold
			: Match.CapturingTeam == 0 ? Blue : Match.CapturingTeam == 1 ? Red : White;
		const FString CaptureStatus = !Match.bCaptureActive
			? FString::Printf(TEXT("CENTER OPENS IN %ds"), FMath::Max(0, FMath::CeilToInt(MatchState->GetPhaseTimeRemaining() - 60.f)))
			: Match.bCaptureContested ? TEXT("CENTER CONTESTED")
			: Match.CapturingTeam == INDEX_NONE ? TEXT("CAPTURE CENTER")
			: FString::Printf(TEXT("%s CAPTURING  %d%%"), Match.CapturingTeam == 0 ? TEXT("BLUE") : TEXT("RED"), Match.CapturePercent);
		Label(CaptureStatus, CaptureColor, Center, 102.f, 0.68f, true);
		if (Match.bCaptureActive && Controller)
		{
			FVector2D Marker;
			if (Controller->ProjectWorldLocationToScreen(Match.CaptureCenter + FVector(0, 0, 100), Marker) &&
				Marker.X >= 0 && Marker.X <= Canvas->ClipX && Marker.Y >= 0 && Marker.Y <= Canvas->ClipY)
			{
				Label(TEXT("[ CENTER ]"), CaptureColor, Marker.X / Scale, Marker.Y / Scale, 0.7f, true);
			}
		}
	}

	const float Health = Character->GetHealth();
	const float Armor = Character->GetArmor();
	const float MaximumArmor = Character->GetArmorRules().MaximumArmor;
	const FLinearColor HealthColor = Health <= 25.f ? Red : Blue;
	const float IconY = (Height - 45.f) * Scale;
	DrawLine((Center - 155.f) * Scale, IconY - 5.f * Scale, (Center - 155.f) * Scale, IconY + 5.f * Scale, HealthColor, 2.f);
	DrawLine((Center - 160.f) * Scale, IconY, (Center - 150.f) * Scale, IconY, HealthColor, 2.f);
	Label(FString::Printf(TEXT("%d"), FMath::CeilToInt(Health)), White, Center - 145.f, Height - 53.f, 1.40f);
	Label(TEXT("HP"), HealthColor, Center - 145.f, Height - 32.f, 0.55f);
	DrawLine((Center - 95.f) * Scale, (Height - 31.f) * Scale, (Center - 22.f) * Scale, (Height - 31.f) * Scale, FLinearColor(1.f, 1.f, 1.f, 0.24f), 2.f);
	DrawLine((Center - 95.f) * Scale, (Height - 31.f) * Scale,
		(Center - 95.f + 73.f * Health / AFMCharacter::MaxHealth) * Scale, (Height - 31.f) * Scale, HealthColor, 2.f);
	const float ShieldX = (Center + 14.f) * Scale;
	const float ShieldY = (Height - 46.f) * Scale;
	DrawLine(ShieldX, ShieldY - 6.f * Scale, ShieldX + 6.f * Scale, ShieldY - 3.f * Scale, Character->HasArmor() ? Gold : White, 1.5f);
	DrawLine(ShieldX + 6.f * Scale, ShieldY - 3.f * Scale, ShieldX + 4.f * Scale, ShieldY + 4.f * Scale, Character->HasArmor() ? Gold : White, 1.5f);
	DrawLine(ShieldX + 4.f * Scale, ShieldY + 4.f * Scale, ShieldX, ShieldY + 7.f * Scale, Character->HasArmor() ? Gold : White, 1.5f);
	DrawLine(ShieldX, ShieldY + 7.f * Scale, ShieldX - 4.f * Scale, ShieldY + 4.f * Scale, Character->HasArmor() ? Gold : White, 1.5f);
	DrawLine(ShieldX - 4.f * Scale, ShieldY + 4.f * Scale, ShieldX - 6.f * Scale, ShieldY - 3.f * Scale, Character->HasArmor() ? Gold : White, 1.5f);
	DrawLine(ShieldX - 6.f * Scale, ShieldY - 3.f * Scale, ShieldX, ShieldY - 6.f * Scale, Character->HasArmor() ? Gold : White, 1.5f);
	Label(Character->HasArmor() ? FString::Printf(TEXT("%d"), FMath::CeilToInt(Armor)) : TEXT("0"), White, Center + 27.f, Height - 53.f, 1.40f);
	Label(TEXT("AR"), Character->HasArmor() ? Gold : White, Center + 27.f, Height - 32.f, 0.55f);
	if (MaximumArmor > 0.f)
	{
		DrawLine((Center + 76.f) * Scale, (Height - 31.f) * Scale, (Center + 149.f) * Scale, (Height - 31.f) * Scale, FLinearColor(1.f, 1.f, 1.f, 0.24f), 2.f);
		DrawLine((Center + 76.f) * Scale, (Height - 31.f) * Scale,
			(Center + 76.f + 73.f * Armor / MaximumArmor) * Scale, (Height - 31.f) * Scale, Gold, 2.f);
	}
	if (Match.Phase == EFMMatchPhase::CombatTest)
		Label(TEXT("V  TEST ARMOR ON / OFF"), Gold, Center, Height - 72.f, 0.58f, true);

	const int32 Slot = bBuildMode ? Controller->GetBuildSelection() : static_cast<int32>(Character->GetWeaponSlot());
	if (bBuildMode)
	{
		const auto* Item = AFMStructure::GetItem(Slot);
		FTransform Placement;
		AFMStructure* Support = nullptr;
		const TCHAR* Reason = TEXT("BUILD UNAVAILABLE");
		const bool bValid = Item && Character->FindStructurePlacement(Slot, Controller->GetBuildRotation(), Placement, Support, &Reason);
		const FLinearColor Cyan(0.f, 0.95f, 1.f);
		Label(bValid ? TEXT("PLACE") : Reason, bValid ? Cyan : Red, Center, Height * 0.5f + 18.f, 0.66f, true);
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
		const float X = Width - 118.f;
		const float Y = Height - 230.f + Index * 43.f;
		const auto* Member = Controller ? Controller->GetPlayerState<AFMPlayerState>() : nullptr;
		const TCHAR* BuildNames[] = {TEXT("SANDBAGS"), TEXT("WALL"), TEXT("CRATE"), TEXT("CONTAINER")};
		FString SlotText;
		if (bBuildMode)
			SlotText = FString::Printf(TEXT("%s %d"), BuildNames[Index], Member ? Member->GetStructureCount(Index) : 0);
		else if (Index <= static_cast<int32>(EFMWeaponSlot::Secondary))
			SlotText = Character->GetWeaponData(static_cast<EFMWeaponSlot>(Index)).DisplayName.ToString();
		else
			SlotText = Index == static_cast<int32>(EFMWeaponSlot::Melee) ? TEXT("KNIFE") : FString::Printf(TEXT("GRENADE x%d"), Character->GetGrenadeCount());
		const FLinearColor SlotColor = Slot == Index ? Gold : FLinearColor(White.R, White.G, White.B, 0.72f);
		int32 IconIndex = INDEX_NONE;
		if (Index < 2)
		{
			const FFMWeaponRow WeaponData = Character->GetWeaponData(static_cast<EFMWeaponSlot>(Index));
			IconIndex = 2;
			switch (WeaponData.Category)
			{
			case EFMWeaponCategory::Pistol: IconIndex = 0; break;
			case EFMWeaponCategory::SMG: IconIndex = 1; break;
			case EFMWeaponCategory::Sniper: IconIndex = 3; break;
			case EFMWeaponCategory::MachineGun: IconIndex = 4; break;
			case EFMWeaponCategory::Shotgun: IconIndex = 5; break;
			default: break;
			}
			Label(FString::FromInt(Index + 1), SlotColor, X + 5.f, Y + 3.f, 0.52f);
		}
		else
		{
			IconIndex = Index == static_cast<int32>(EFMWeaponSlot::Melee) ? 6 : 7;
		}
		UTexture2D* SlotIcon = nullptr;
		if(!bBuildMode && Index < 2)
		{
			const FName Id=Character->GetWeaponId(static_cast<EFMWeaponSlot>(Index));
			if(const TObjectPtr<UTexture2D>* Found=ItemIcons.Find(Id))SlotIcon=Found->Get();
		}
		else if(!bBuildMode && Index == static_cast<int32>(EFMWeaponSlot::Throwable))
		{
			if(const TObjectPtr<UTexture2D>* Found=ItemIcons.Find(Character->GetWeaponId(EFMWeaponSlot::Throwable)))SlotIcon=Found->Get();
		}
		if(SlotIcon)
		{
			const bool bFirearmSlot = Index <= static_cast<int32>(EFMWeaponSlot::Secondary);
			const float IconWidth = bFirearmSlot ? 82.f : 36.f;
			const float IconHeight = bFirearmSlot ? 25.f : 24.f;
			const float IconX = bFirearmSlot ? X + 22.f : X + 4.f;
			DrawTexture(SlotIcon,IconX*Scale,(Y+1.f)*Scale,IconWidth*Scale,IconHeight*Scale,
				0.f,0.f,1.f,1.f,FLinearColor::White,BLEND_Translucent);
		}
		else if (WeaponIcons.IsValidIndex(IconIndex) && WeaponIcons[IconIndex])
		{
			const bool bFirearmSlot = Index <= static_cast<int32>(EFMWeaponSlot::Secondary);
			const float IconWidth = bFirearmSlot ? 82.f : 36.f;
			const float IconHeight = bFirearmSlot ? 25.f : 24.f;
			const float IconX = bFirearmSlot ? X + 22.f : X + 4.f;
			DrawTexture(WeaponIcons[IconIndex], IconX * Scale, (Y + 1.f) * Scale, IconWidth * Scale, IconHeight * Scale,
				0.f, 0.f, 1.f, 1.f, FLinearColor::White, BLEND_Translucent);
		}
		else if (Index == 2)
		{
			const float GX = (X + 5.f) * Scale, GY = (Y + 8.f) * Scale;
			DrawLine(GX, GY, GX + 9.f * Scale, GY + 2.f * Scale, SlotColor, 1.5f);
			DrawLine(GX, GY, GX + 5.f * Scale, GY + 6.f * Scale, SlotColor, 1.f);
			DrawLine(GX + 9.f * Scale, GY + 2.f * Scale, GX + 7.f * Scale, GY + 5.f * Scale, SlotColor, 1.f);
			DrawLine(GX + 5.f * Scale, GY + 6.f * Scale, GX + 7.f * Scale, GY + 9.f * Scale, SlotColor, 1.5f);
			DrawLine(GX + 7.f * Scale, GY + 9.f * Scale, GX + 11.f * Scale, GY + 12.f * Scale, SlotColor, 1.5f);
		}
		else
		{
			const float GX = (X + 8.f) * Scale, GY = (Y + 12.f) * Scale, R = 4.f * Scale;
			for (int32 Segment = 0; Segment < 10; ++Segment)
			{
				const float A0 = 2.f * PI * Segment / 10.f, A1 = 2.f * PI * (Segment + 1) / 10.f;
				DrawLine(GX + FMath::Cos(A0) * R, GY + FMath::Sin(A0) * R,
					GX + FMath::Cos(A1) * R, GY + FMath::Sin(A1) * R, SlotColor, 1.f);
			}
			DrawLine(GX, GY - 5.f * Scale, GX, GY - 3.f * Scale, SlotColor, 1.2f);
			DrawLine(GX - 2.f * Scale, GY - 5.f * Scale, GX + 2.f * Scale, GY - 5.f * Scale, SlotColor, 1.2f);
			DrawLine(GX - 2.f * Scale, GY, GX + 2.f * Scale, GY, SlotColor, 0.8f);
		}
		if (Index < 2)
			Label(SlotText, SlotColor, X + 54.f, Y + 28.f, 0.50f, true);
		else
		{
			Label(FString::Printf(TEXT("%d"), Index + 1), SlotColor, X + 20.f, Y + 1.f, 0.52f);
			Label(SlotText, SlotColor, X + 8.f, Y + 18.f, 0.56f, true);
		}
		if (Slot == Index) DrawLine((X - 2.f) * Scale, (Y + 40.f) * Scale, (X + (Index < 2 ? 108.f : 26.f)) * Scale, (Y + 40.f) * Scale, Gold, 1.3f);
	}
	const bool bFirearm = Slot <= static_cast<int32>(EFMWeaponSlot::Secondary);
	const TCHAR* AmmoLabel = Character->IsReloading() ? TEXT("RELOADING") : bFirearm ? TEXT("AMMO") : Slots[Slot];
	Label(AmmoLabel, Gold, Width - 88.f, Height - 65.f, 0.58f);
	const FString Ammo = bFirearm
		? FString::Printf(TEXT("%02d / %02d"), Character->GetLoadedAmmo(), Character->GetReserveAmmo())
		: Slot == static_cast<int32>(EFMWeaponSlot::Throwable) ? FString::FromInt(Character->GetLoadedAmmo()) : TEXT("READY");
	Label(Ammo, White, Width - 88.f, Height - 45.f, 1.18f);

	const float AimY = Height * 0.5f;
	if (Health > 0.f && !bScoped)
	{
		Box(Ink, Center - 2.f, AimY - 2.f, 4.f, 4.f);
		Box(White, Center - 1.f, AimY - 1.f, 2.f, 2.f);
		if (!bBuildMode)
		{
			const float Gap = FMath::Clamp(3.f + Character->GetCrosshairSpreadDegrees() * 7.f, 3.f, 28.f);
			const float Length = Character->IsAiming() ? 3.f : 5.f;
			Box(White, Center - Gap - Length, AimY - 0.5f, Length, 1.f);
			Box(White, Center + Gap, AimY - 0.5f, Length, 1.f);
			Box(White, Center - 0.5f, AimY - Gap - Length, 1.f, Length);
			Box(White, Center - 0.5f, AimY + Gap, 1.f, Length);
		}
	}
	else if(Health<=0.f)
	{
		Box(Ink, Center - 80.f, AimY - 17.f, 160.f, 34.f);
		Label(TEXT("ELIMINATED"), Red, Center, AimY - 10.f, 1.2f, true);
	}
}
