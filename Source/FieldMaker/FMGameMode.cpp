#include "FMGameMode.h"
#include "FMCombatHUD.h"
#include "GameFramework/GameStateBase.h"
#include "FMPlayerState.h"
#include "FMCharacter.h"
#include "FMPlayerController.h"
#include "FMGameState.h"
#include "GameFramework/PlayerStart.h"
#include "EngineUtils.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"

AFMGameMode::AFMGameMode()
{
	HUDClass = AFMCombatHUD::StaticClass();
	DefaultPawnClass = AFMCharacter::StaticClass();
	PlayerStateClass = AFMPlayerState::StaticClass();
	PlayerControllerClass = AFMPlayerController::StaticClass();
	GameStateClass = AFMGameState::StaticClass();
}

void AFMGameMode::InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage)
{
	Super::InitGame(MapName, Options, ErrorMessage);
	bPlayerRoom = UGameplayStatics::HasOption(Options, TEXT("FMRoom"));
	if (bPlayerRoom) bStartPlayersAsSpectators = true;
}

void AFMGameMode::InitGameState()
{
	Super::InitGameState();
	if (auto* State = GetGameState<AFMGameState>(); bPlayerRoom && State)
	{
		State->InitializePlayerRoom();
	}
}

FString AFMGameMode::GetRoomAdmissionError(APlayerController* PendingPlayer)
{
	if (!bPlayerRoom) return {};
	if (bRoomStarted) return TEXT("FIELDMAKER_ROOM_STARTED");
	const int32 PendingCount = PendingPlayer && PendingPlayer->PlayerState && !MustSpectate(PendingPlayer) ? 1 : 0;
	return GetNumPlayers() - PendingCount >= 2 ? TEXT("FIELDMAKER_ROOM_FULL") : FString();
}

void AFMGameMode::PreLogin(const FString& Options, const FString& Address, const FUniqueNetIdRepl& UniqueId,
	FString& ErrorMessage)
{
	Super::PreLogin(Options, Address, UniqueId, ErrorMessage);
	if (ErrorMessage.IsEmpty()) ErrorMessage = GetRoomAdmissionError();
}

FString AFMGameMode::InitNewPlayer(APlayerController* Player, const FUniqueNetIdRepl& UniqueId,
	const FString& Options, const FString& Portal)
{
	// A second connection can finish its handshake after the room filled.
	const FString AdmissionError = GetRoomAdmissionError(Player);
	if (!AdmissionError.IsEmpty()) return AdmissionError;
	// Super::InitNewPlayer selects the start spot, so team assignment must precede it.
	AssignTeam(Player);
	return Super::InitNewPlayer(Player, UniqueId, Options, Portal);
}

void AFMGameMode::AssignTeam(APlayerController* Player)
{
	auto* NewState = Player ? Player->GetPlayerState<AFMPlayerState>() : nullptr;
	if (!NewState || !GameState)
	{
		return;
	}

	int32 Counts[2] = {};
	for (const APlayerState* Member : GameState->PlayerArray)
	{
		const auto* State = Cast<AFMPlayerState>(Member);
		if (State && State != NewState && State->GetTeam() < AFMPlayerState::UnassignedTeam)
		{
			++Counts[State->GetTeam()];
		}
	}
	NewState->SetTeam(Counts[0] <= Counts[1] ? 0 : 1);
}

void AFMGameMode::StartPlay()
{
	Super::StartPlay();
	RefreshLobby();
}

void AFMGameMode::PostLogin(APlayerController* Player)
{
	Super::PostLogin(Player);
	RefreshLobby();
}

void AFMGameMode::Logout(AController* Exiting)
{
	Super::Logout(Exiting);
	RefreshLobby(Exiting);
}

void AFMGameMode::RefreshLobby(AController* Excluding)
{
	auto* State = GetGameState<AFMGameState>();
	if ((!bTeamMatch && !bPlayerRoom) || !State || bRoomStarted) return;
	int32 Counts[2] = {};
	for (TActorIterator<APlayerController> It(GetWorld()); It; ++It)
	{
		const auto* Player = It->GetPlayerState<AFMPlayerState>();
		if (*It != Excluding && Player && Player->GetTeam() < 2) ++Counts[Player->GetTeam()];
	}
	State->SetLobbyReady(Counts[0] > 0 && Counts[1] > 0);
	if (bCombatSandbox && !bPlayerRoom) State->SetPhase(EFMMatchPhase::CombatTest);
}

bool AFMGameMode::StartPlayerRoom(APlayerController* Requester)
{
	auto* State = GetGameState<AFMGameState>();
	if (!bPlayerRoom || bRoomStarted || !State || !Requester || !Requester->IsLocalController() ||
		State->GetMatchSnapshot().Phase != EFMMatchPhase::Ready)
	{
		return false;
	}

	bRoomStarted = true;
	bStartPlayersAsSpectators = false;
	State->SetPhase(EFMMatchPhase::CombatTest);
	for (TActorIterator<APlayerController> It(GetWorld()); It; ++It)
	{
		RestartPlayer(*It);
	}
	return true;
}

bool AFMGameMode::PlayerCanRestart_Implementation(APlayerController* Player)
{
	return (!bPlayerRoom || bRoomStarted) && Super::PlayerCanRestart_Implementation(Player);
}

AActor* AFMGameMode::ChoosePlayerStart_Implementation(AController* Player)
{
	const auto* State = Player ? Player->GetPlayerState<AFMPlayerState>() : nullptr;
	if (bTeamMatch && State && State->GetTeam() < 2)
	{
		const auto* Match = GetGameState<AFMGameState>();
		const bool bSwap = Match && Match->GetMatchSnapshot().bSidesSwapped;
		const int32 SpawnSide = State->GetTeam() ^ int32(bSwap);
		const FName Tag(*FString::Printf(TEXT("FM_Team%d"), SpawnSide));
		for (TActorIterator<APlayerStart> It(GetWorld()); It; ++It)
		{
			if (It->PlayerStartTag == Tag) return *It;
		}
		UE_LOG(LogTemp, Warning, TEXT("FIELDMAKER: Missing team start %s"), *Tag.ToString());
	}
	return Super::ChoosePlayerStart_Implementation(Player);
}
