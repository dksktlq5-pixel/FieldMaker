#include "FMSteamRoomSubsystem.h"
#include "FMGameState.h"
#include "FMMenuPlayerController.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Interfaces/OnlineExternalUIInterface.h"
#include "Interfaces/OnlineIdentityInterface.h"
#include "Kismet/GameplayStatics.h"
#include "OnlineSessionSettings.h"
#include "OnlineSubsystem.h"
#include "OnlineSubsystemUtils.h"

#define LOCTEXT_NAMESPACE "FMSteamRoom"

namespace
{
const FName FieldMakerRoomKey(TEXT("FIELDMAKER_ROOM"));
const FName SteamService(TEXT("STEAM"));
}

void UFMSteamRoomSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	FCoreUObjectDelegates::PostLoadMapWithWorld.AddUObject(this, &UFMSteamRoomSubsystem::OnMapLoaded);
	GEngine->OnNetworkFailure().AddUObject(this, &UFMSteamRoomSubsystem::OnNetworkFailure);
	GEngine->OnTravelFailure().AddUObject(this, &UFMSteamRoomSubsystem::OnTravelFailure);
}

void UFMSteamRoomSubsystem::Deinitialize()
{
	LeaveRoom();
	Unbind();
	FCoreUObjectDelegates::PostLoadMapWithWorld.RemoveAll(this);
	if (GEngine)
	{
		GEngine->OnNetworkFailure().RemoveAll(this);
		GEngine->OnTravelFailure().RemoveAll(this);
	}
	Super::Deinitialize();
}

UWorld* UFMSteamRoomSubsystem::GetWorld() const
{
	const auto* Instance = GetGameInstance();
	return Instance ? Instance->GetWorld() : nullptr;
}

IOnlineSessionPtr UFMSteamRoomSubsystem::GetSessions() const
{
	const auto* World = GetWorld();
	auto* Steam = World && World->WorldType == EWorldType::Game ? Online::GetSubsystem(World, SteamService) : nullptr;
	const auto Identity = Steam ? Steam->GetIdentityInterface() : nullptr;
	return Identity && Identity->GetLoginStatus(0) == ELoginStatus::LoggedIn ? Steam->GetSessionInterface() : nullptr;
}

void UFMSteamRoomSubsystem::BindInvites()
{
	const auto Sessions = GetSessions();
	if (Sessions == BoundSessions) return;
	Unbind();
	BoundSessions = Sessions;
	if (BoundSessions)
	{
		InviteHandle = BoundSessions->AddOnSessionUserInviteAcceptedDelegate_Handle(
			FOnSessionUserInviteAcceptedDelegate::CreateUObject(this, &UFMSteamRoomSubsystem::OnInviteAccepted));
	}
}

void UFMSteamRoomSubsystem::Unbind()
{
	if (!BoundSessions) return;
	BoundSessions->ClearOnSessionUserInviteAcceptedDelegate_Handle(InviteHandle);
	BoundSessions->ClearOnCreateSessionCompleteDelegate_Handle(CreateHandle);
	BoundSessions->ClearOnJoinSessionCompleteDelegate_Handle(JoinHandle);
	InviteHandle.Reset();
	CreateHandle.Reset();
	JoinHandle.Reset();
	BoundSessions.Reset();
}

bool UFMSteamRoomSubsystem::IsBusy() const
{
	return CreateHandle.IsValid() || JoinHandle.IsValid() || bTravelPending ||
		(BoundSessions && BoundSessions->GetSessionState(NAME_GameSession) == EOnlineSessionState::Destroying);
}

void UFMSteamRoomSubsystem::CreateRoom()
{
	BindInvites();
	if (!BoundSessions)
	{
		Notice = LOCTEXT("SteamUnavailable", "Steam에 로그인하고 독립 실행 게임에서 다시 시도하세요.");
		return;
	}
	if (IsBusy() || BoundSessions->GetNamedSession(NAME_GameSession)) return;

	FOnlineSessionSettings Settings;
	Settings.NumPublicConnections = AFMGameState::RoomCapacity;
	Settings.bUsesPresence = true;
	Settings.bUseLobbiesIfAvailable = true;
	Settings.bAllowInvites = true;
	Settings.bAllowJoinInProgress = true;
	Settings.Set(FieldMakerRoomKey, true, EOnlineDataAdvertisementType::ViaOnlineService);
	Notice = LOCTEXT("Creating", "Steam 대기방 생성 중…");
	CreateHandle = BoundSessions->AddOnCreateSessionCompleteDelegate_Handle(
		FOnCreateSessionCompleteDelegate::CreateUObject(this, &UFMSteamRoomSubsystem::OnCreated));
	if (!BoundSessions->CreateSession(0, NAME_GameSession, Settings) && CreateHandle.IsValid())
	{
		OnCreated(NAME_GameSession, false);
	}
}

void UFMSteamRoomSubsystem::OnCreated(FName Name, bool bSuccess)
{
	if (Name != NAME_GameSession || !BoundSessions) return;
	BoundSessions->ClearOnCreateSessionCompleteDelegate_Handle(CreateHandle);
	CreateHandle.Reset();
	if (!bSuccess)
	{
		Notice = LOCTEXT("CreateFailed", "Steam 대기방을 만들지 못했습니다.");
		LeaveRoom();
		return;
	}

	bTravelPending = true;
	Notice = FText::GetEmpty();
	UGameplayStatics::OpenLevel(this, TEXT("/Game/Map/L_FMBridgeBattlefield"), true, TEXT("listen?FMRoom?FMSteamRoom"));
}

void UFMSteamRoomSubsystem::InviteFriends()
{
	auto* Steam = Online::GetSubsystem(GetWorld(), SteamService);
	const auto ExternalUI = Steam ? Steam->GetExternalUIInterface() : nullptr;
	if (!ExternalUI || !ExternalUI->ShowInviteUI(0, NAME_GameSession))
	{
		Notice = LOCTEXT("InviteFailed", "Steam 친구 초대 창을 열 수 없습니다.");
	}
}

void UFMSteamRoomSubsystem::OnInviteAccepted(bool bSuccess, int32 ControllerId, FUniqueNetIdPtr UserId,
	const FOnlineSessionSearchResult& Result)
{
	if (!Cast<AFMMenuPlayerController>(GetGameInstance()->GetFirstLocalPlayerController()) ||
		!BoundSessions || IsBusy() || BoundSessions->GetNamedSession(NAME_GameSession))
	{
		return;
	}
	bool bFieldMakerRoom = false;
	if (!bSuccess || !Result.IsValid() || !Result.Session.SessionSettings.Get(FieldMakerRoomKey, bFieldMakerRoom) || !bFieldMakerRoom)
	{
		Notice = LOCTEXT("InvalidInvite", "유효한 FIELDMAKER 대기방 초대를 찾지 못했습니다.");
		return;
	}

	Notice = LOCTEXT("Joining", "Steam 대기방에 연결 중…");
	JoinHandle = BoundSessions->AddOnJoinSessionCompleteDelegate_Handle(
		FOnJoinSessionCompleteDelegate::CreateUObject(this, &UFMSteamRoomSubsystem::OnJoined));
	if (!BoundSessions->JoinSession(ControllerId, NAME_GameSession, Result) && JoinHandle.IsValid())
	{
		OnJoined(NAME_GameSession, EOnJoinSessionCompleteResult::UnknownError);
	}
}

void UFMSteamRoomSubsystem::OnJoined(FName Name, EOnJoinSessionCompleteResult::Type Result)
{
	if (Name != NAME_GameSession || !BoundSessions) return;
	BoundSessions->ClearOnJoinSessionCompleteDelegate_Handle(JoinHandle);
	JoinHandle.Reset();
	FString Address;
	auto* Player = GetGameInstance()->GetFirstLocalPlayerController();
	if (Result != EOnJoinSessionCompleteResult::Success || !Player ||
		!BoundSessions->GetResolvedConnectString(NAME_GameSession, Address) || Address.IsEmpty())
	{
		Notice = LOCTEXT("JoinFailed", "Steam 대기방에 참가하지 못했습니다.");
		LeaveRoom();
		return;
	}
	bTravelPending = true;
	Notice = FText::GetEmpty();
	Player->ClientTravel(Address, TRAVEL_Absolute);
}

void UFMSteamRoomSubsystem::LeaveRoom()
{
	bTravelPending = false;
	if (BoundSessions && BoundSessions->GetNamedSession(NAME_GameSession) &&
		BoundSessions->GetSessionState(NAME_GameSession) != EOnlineSessionState::Destroying)
	{
		BoundSessions->DestroySession(NAME_GameSession);
	}
}

void UFMSteamRoomSubsystem::OnMapLoaded(UWorld* World)
{
	if (!World || World->GetGameInstance() != GetGameInstance()) return;
	bTravelPending = false;
	if (World->GetMapName().EndsWith(TEXT("L_FMFrontend")) && !IsBusy()) LeaveRoom();
	BindInvites();
}

void UFMSteamRoomSubsystem::OnNetworkFailure(UWorld* World, UNetDriver* Driver, ENetworkFailure::Type Type, const FString& Error)
{
	TravelFailed(World);
}

void UFMSteamRoomSubsystem::OnTravelFailure(UWorld* World, ETravelFailure::Type Type, const FString& Error)
{
	TravelFailed(World);
}

void UFMSteamRoomSubsystem::TravelFailed(UWorld* World)
{
	if (World != GetWorld() || !BoundSessions || !BoundSessions->GetNamedSession(NAME_GameSession)) return;
	Notice = LOCTEXT("ConnectionLost", "Steam 방 연결이 종료됐습니다.");
	LeaveRoom();
}

#undef LOCTEXT_NAMESPACE
