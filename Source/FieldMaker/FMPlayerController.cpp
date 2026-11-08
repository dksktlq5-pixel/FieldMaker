#include "FMPlayerController.h"
#include "FMPlayerState.h"
#include "EnhancedInputSubsystems.h"
#include "Engine/LocalPlayer.h"
#include "InputMappingContext.h"
#include "Components/InputComponent.h"
#include "FMUserPreferences.h"
#include "FMShopWidget.h"
#include "FMGameMode.h"
#include "FMGameState.h"
#include "FMLobbyWidget.h"
#include "FMSteamRoomSubsystem.h"
#include "Engine/GameInstance.h"
#include "Engine/NetDriver.h"
#include "IPAddress.h"
#include "Kismet/GameplayStatics.h"
#include "SocketSubsystem.h"
#include "Styling/CoreStyle.h"

void AFMPlayerController::BeginPlay()
{
	Super::BeginPlay();
	if (IsLocalController())
	{
		SetInputMode(FInputModeGameOnly());
		bShowMouseCursor = false;
	}
}

void AFMPlayerController::PlayerTick(float DeltaTime)
{
	Super::PlayerTick(DeltaTime);
	if (!IsLocalController()) return;
	const auto* State = GetWorld()->GetGameState<AFMGameState>();
	if (!State) return;

	const auto Match = State->GetMatchSnapshot();
	if (ShopWidget && Match.Phase != EFMMatchPhase::Purchase && Match.Phase != EFMMatchPhase::CombatTest) CloseShop();
	const bool bWaitingInRoom = Match.bPlayerRoom &&
		(Match.Phase == EFMMatchPhase::WaitingForPlayers || Match.Phase == EFMMatchPhase::Ready);
	if (bWaitingInRoom && !RoomWidget)
	{
		RoomWidget = CreateWidget<UFMLobbyWidget>(this, UFMLobbyWidget::StaticClass());
		if (!RoomWidget) return;
		RoomWidget->bInRoom = true;
		RoomWidget->bSteamRoom = Match.bSteamRoom;
		RoomWidget->MenuFont = FCoreStyle::GetDefaultFontStyle("Regular", 16);
		RoomWidget->OnStart.BindUObject(this, &AFMPlayerController::StartRoomMatch);
		RoomWidget->OnClose.BindUObject(this, &AFMPlayerController::LeaveRoom);
		if (HasAuthority() && Match.bSteamRoom)
		{
			RoomWidget->OnInviteFriends.BindUObject(GetGameInstance()->GetSubsystem<UFMSteamRoomSubsystem>(),
				&UFMSteamRoomSubsystem::InviteFriends);
		}
		else if (HasAuthority())
		{
			const auto BoundAddress = GetNetDriver() ? GetNetDriver()->GetLocalAddr() : nullptr;
			const int32 Port = BoundAddress ? BoundAddress->GetPort() : GetWorld()->URL.Port;
			FString LocalIP = TEXT("127.0.0.1");
			if (auto* Sockets = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM))
			{
				bool bCanBindAll = false;
				const auto LocalAddress = Sockets->GetLocalHostAddr(*GLog, bCanBindAll);
				if (LocalAddress->IsValid()) LocalIP = LocalAddress->ToString(false);
			}
			RoomWidget->HostAddress = FText::Format(
				NSLOCTEXT("FMLobby", "HostAddress", "참가 주소   {0}:{1}\n같은 PC에서는 127.0.0.1:{1}"),
				FText::FromString(LocalIP), FText::AsNumber(Port, &FNumberFormattingOptions::DefaultNoGrouping()));
		}
		RoomWidget->AddToViewport(100);
		bShowMouseCursor = true;
		FInputModeUIOnly Mode;
		Mode.SetWidgetToFocus(RoomWidget->TakeWidget());
		SetInputMode(Mode);
	}
	else if (!bWaitingInRoom && RoomWidget)
	{
		RoomWidget->RemoveFromParent();
		RoomWidget = nullptr;
		bShowMouseCursor = false;
		SetInputMode(FInputModeGameOnly());
	}
}

void AFMPlayerController::StartRoomMatch()
{
	if (HasAuthority() && IsLocalController())
	{
		if (auto* Mode = GetWorld()->GetAuthGameMode<AFMGameMode>()) Mode->StartPlayerRoom(this);
	}
}

void AFMPlayerController::LeaveRoom()
{
	const auto* State = GetWorld()->GetGameState<AFMGameState>();
	if (IsLocalController() && State && State->GetMatchSnapshot().bPlayerRoom &&
		(RoomWidget || State->GetMatchSnapshot().Phase == EFMMatchPhase::Finished))
	{
		if (State->GetMatchSnapshot().bSteamRoom) GetGameInstance()->GetSubsystem<UFMSteamRoomSubsystem>()->LeaveRoom();
		UGameplayStatics::OpenLevel(this, TEXT("/Game/Map/L_FMFrontend"));
	}
}

void AFMPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	ULocalPlayer* LocalPlayer = GetLocalPlayer();
	if (!LocalPlayer)
	{
		return;
	}

	InputComponent->BindKey(EKeys::One, IE_Pressed, this, &AFMPlayerController::EquipWeapon<EFMWeaponSlot::Primary>);
	InputComponent->BindKey(EKeys::Two, IE_Pressed, this, &AFMPlayerController::EquipWeapon<EFMWeaponSlot::Secondary>);
	InputComponent->BindKey(EKeys::Three, IE_Pressed, this, &AFMPlayerController::EquipWeapon<EFMWeaponSlot::Melee>);
	InputComponent->BindKey(EKeys::Four, IE_Pressed, this, &AFMPlayerController::EquipWeapon<EFMWeaponSlot::Throwable>);
	InputComponent->BindKey(EKeys::Five, IE_Pressed, this, &AFMPlayerController::PlaceStructure<0>);
	InputComponent->BindKey(EKeys::Six, IE_Pressed, this, &AFMPlayerController::PlaceStructure<1>);
	InputComponent->BindKey(EKeys::LeftMouseButton, IE_Pressed, this, &AFMPlayerController::UseWeapon);
	InputComponent->BindKey(EKeys::LeftMouseButton, IE_Released, this, &AFMPlayerController::StopUsingWeapon);
	InputComponent->BindKey(EKeys::R, IE_Pressed, this, &AFMPlayerController::ReloadWeapon);
	InputComponent->BindKey(EKeys::RightMouseButton, IE_Pressed, this, &AFMPlayerController::StartAiming);
	InputComponent->BindKey(EKeys::RightMouseButton, IE_Released, this, &AFMPlayerController::StopAiming);
	InputComponent->BindKey(EKeys::B, IE_Pressed, this, &AFMPlayerController::ToggleShop);
	InputComponent->BindKey(EKeys::F10, IE_Pressed, this, &AFMPlayerController::LeaveRoom);
	InputComponent->BindKey(EKeys::V, IE_Pressed, this, &AFMPlayerController::ToggleTestArmor);

	auto* Input = LocalPlayer->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>();
	const auto* Context = LoadObject<UInputMappingContext>(nullptr, TEXT("/Game/Input/IMC_FMPlayer.IMC_FMPlayer"));
	if (Input && Context && !Input->HasMappingContext(Context))
	{
		Input->AddMappingContext(Context, 0);
	}
}

void AFMPlayerController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);
	if (IsLocalController())
	{
		ServerSetAppearance(UFMAppearanceSaveGame::Load());
	}
}

void AFMPlayerController::AcknowledgePossession(APawn* InPawn)
{
	Super::AcknowledgePossession(InPawn);
	if (IsLocalController() && !HasAuthority())
	{
		ServerSetAppearance(UFMAppearanceSaveGame::Load());
	}
}

void AFMPlayerController::ServerSetAppearance_Implementation(uint32 AppearanceId)
{
	if (auto* State = GetPlayerState<AFMPlayerState>())
	{
		State->SetAppearanceId(AppearanceId);
	}
}

template <EFMWeaponSlot Slot>
void AFMPlayerController::EquipWeapon()
{
	if (IsShopOpen()) return;
	if (auto* CombatPawn = GetPawn<AFMCharacter>())
	{
		CombatPawn->ServerEquipWeapon(Slot);
	}
}

void AFMPlayerController::UseWeapon()
{
	if (IsShopOpen()) return;
	if (auto* CombatPawn = GetPawn<AFMCharacter>())
	{
		CombatPawn->ServerStartFire();
	}
}

void AFMPlayerController::StopUsingWeapon()
{
	if (auto* CombatPawn = GetPawn<AFMCharacter>())
	{
		CombatPawn->ServerStopFire();
	}
}

void AFMPlayerController::ReloadWeapon()
{
	if (IsShopOpen()) return;
	if (auto* CombatPawn = GetPawn<AFMCharacter>())
	{
		CombatPawn->ServerReload();
	}
}

void AFMPlayerController::StartAiming()
{
	if (IsShopOpen()) return;
	if (auto* CombatPawn = GetPawn<AFMCharacter>())
		CombatPawn->ServerSetAiming(!FMLocalPreferences::Get().bToggleADS || !CombatPawn->IsAiming());
}

void AFMPlayerController::StopAiming()
{
	if (!FMLocalPreferences::Get().bToggleADS)
		if (auto* CombatPawn = GetPawn<AFMCharacter>()) CombatPawn->ServerSetAiming(false);
}

void AFMPlayerController::ToggleTestArmor()
{
	if (IsShopOpen()) return;
	if (auto* CombatPawn = GetPawn<AFMCharacter>()) CombatPawn->ServerToggleTestArmor();
}

template<int32 Index> void AFMPlayerController::PlaceStructure()
{
	const auto* State = GetWorld()->GetGameState<AFMGameState>();
	if (!State || State->GetMatchSnapshot().Phase != EFMMatchPhase::Building || IsShopOpen()) return;
	if (auto* CombatPawn = GetPawn<AFMCharacter>()) CombatPawn->ServerPlaceStructure(Index);
}

void AFMPlayerController::ToggleShop()
{
 if (!IsLocalController() || RoomWidget) return;
 if (ShopWidget) { CloseShop(); return; }
 const auto* State = GetWorld()->GetGameState<AFMGameState>();
 if (!GetPawn<AFMCharacter>() || !State ||
	 (State->GetMatchSnapshot().Phase != EFMMatchPhase::Purchase && State->GetMatchSnapshot().Phase != EFMMatchPhase::CombatTest)) return;
 ShopWidget = CreateWidget<UFMShopWidget>(this, UFMShopWidget::StaticClass());
 if (!ShopWidget) return;
 if (auto* P = GetPawn<AFMCharacter>())
 {
  P->ServerStopFire(); P->ServerSetAiming(false); P->ServerSetSprint(false);
 }
 StopMovement();
 SetIgnoreMoveInput(true); SetIgnoreLookInput(true);
 ShopWidget->AddToViewport(50);
 bShowMouseCursor = true;
 FInputModeUIOnly Mode;
 Mode.SetWidgetToFocus(ShopWidget->TakeWidget());
 Mode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
 SetInputMode(Mode);
 ShopWidget->SetKeyboardFocus();
}
void AFMPlayerController::CloseShop()
{
 if (!ShopWidget) return;
 ShopWidget->RemoveFromParent(); ShopWidget = nullptr;
 SetIgnoreMoveInput(false); SetIgnoreLookInput(false);
 bShowMouseCursor = false;
 SetInputMode(FInputModeGameOnly());
}
