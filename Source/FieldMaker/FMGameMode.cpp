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

AFMGameMode::AFMGameMode()
{
	HUDClass = AFMCombatHUD::StaticClass();
	DefaultPawnClass = AFMCharacter::StaticClass();
	PlayerStateClass = AFMPlayerState::StaticClass();
	PlayerControllerClass = AFMPlayerController::StaticClass();
	GameStateClass = AFMGameState::StaticClass();
}

FString AFMGameMode::InitNewPlayer(APlayerController* Player, const FUniqueNetIdRepl& UniqueId,
	const FString& Options, const FString& Portal)
{
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
	if (!bTeamMatch || !State) return;
	int32 Counts[2] = {};
	for (TActorIterator<APlayerController> It(GetWorld()); It; ++It)
	{
		const auto* Player = It->GetPlayerState<AFMPlayerState>();
		if (*It != Excluding && Player && Player->GetTeam() < 2) ++Counts[Player->GetTeam()];
	}
	State->SetLobbyReady(Counts[0] > 0 && Counts[1] > 0);
	if (bCombatSandbox) State->SetPhase(EFMMatchPhase::CombatTest);
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
