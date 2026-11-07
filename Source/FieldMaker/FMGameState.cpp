#include "FMGameState.h"
#include "Net/UnrealNetwork.h"

AFMGameState::AFMGameState()
{
	// Keep the authoritative clock sufficiently fresh for phase countdown displays.
	ServerWorldTimeSecondsUpdateFrequency = 1.f;
}

float AFMGameState::GetPhaseTimeRemaining() const
{
	return MatchSnapshot.PhaseEndServerTime > 0.0
		? float(FMath::Max(0.0, MatchSnapshot.PhaseEndServerTime - GetServerWorldTimeSeconds())) : 0.f;
}

void AFMGameState::SetLobbyReady(bool bReady)
{
	if (!HasAuthority()) return;
	MatchSnapshot.Phase = bReady ? EFMMatchPhase::Ready : EFMMatchPhase::WaitingForPlayers;
	MatchSnapshot.RoundNumber = 1;
	MatchSnapshot.PhaseEndServerTime = 0.0;
	ForceNetUpdate();
}

void AFMGameState::InitializePlayerRoom()
{
	if (!HasAuthority()) return;
	MatchSnapshot.bPlayerRoom = true;
	SetLobbyReady(false);
}

void AFMGameState::SetPhase(EFMMatchPhase Phase, float Duration)
{
	if (!HasAuthority() || !FMath::IsFinite(Duration)) return;
	MatchSnapshot.Phase = Phase;
	MatchSnapshot.PhaseEndServerTime = Duration > 0.f ? GetServerWorldTimeSeconds() + Duration : 0.0;
	ForceNetUpdate();
}

void AFMGameState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AFMGameState, MatchSnapshot);
}
