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
	MatchSnapshot.RoundWinner = -1;
	MatchSnapshot.ResultReason = EFMResultReason::None;
	MatchSnapshot.bRoundResultRecorded = false;
	ForceNetUpdate();
}

void AFMGameState::InitializePlayerRoom(bool bSteamRoom)
{
	if (!HasAuthority()) return;
	MatchSnapshot.bPlayerRoom = true;
	MatchSnapshot.bSteamRoom = bSteamRoom;
	SetLobbyReady(false);
}

bool AFMGameState::RecordRoundResult(int32 Winner, EFMResultReason Reason)
{
	if (!HasAuthority() || MatchSnapshot.bRoundResultRecorded) return false;
	MatchSnapshot.bRoundResultRecorded = true;
	MatchSnapshot.RoundWinner = Winner;
	MatchSnapshot.ResultReason = Reason;
	if (Winner == 0) ++MatchSnapshot.Team0Score;
	if (Winner == 1) ++MatchSnapshot.Team1Score;
	ForceNetUpdate();
	return true;
}

void AFMGameState::AdvanceRound(bool bSwapSides)
{
	if (!HasAuthority()) return;
	++MatchSnapshot.RoundNumber;
	MatchSnapshot.bSidesSwapped |= bSwapSides;
	MatchSnapshot.RoundWinner = -1;
	MatchSnapshot.ResultReason = EFMResultReason::None;
	MatchSnapshot.bRoundResultRecorded = false;
	ForceNetUpdate();
}

void AFMGameState::UpdateCapture(const FVector& Center, int32 Team, int32 Percent, bool bContested)
{
	if (!HasAuthority()) return;
	MatchSnapshot.CaptureCenter = Center;
	MatchSnapshot.CapturingTeam = Team;
	MatchSnapshot.CapturePercent = Percent;
	MatchSnapshot.bCaptureActive = true;
	MatchSnapshot.bCaptureContested = bContested;
}

void AFMGameState::AddKillFeed(const FString& Killer, const FString& Victim, const FString& Weapon)
{
	if (!HasAuthority()) return;
	FFMKillFeedEntry Entry;
	Entry.Killer = Killer.Left(20);
	Entry.Victim = Victim.Left(20);
	Entry.Weapon = Weapon.Left(20);
	Entry.ServerTime = GetServerWorldTimeSeconds();
	KillFeed.Insert(MoveTemp(Entry), 0);
	KillFeed.SetNum(FMath::Min(KillFeed.Num(), 6));
	ForceNetUpdate();
}

void AFMGameState::SetPhase(EFMMatchPhase Phase, float Duration)
{
	if (!HasAuthority() || !FMath::IsFinite(Duration)) return;
	MatchSnapshot.Phase = Phase;
	if (Phase != EFMMatchPhase::Combat)
	{
		MatchSnapshot.bCaptureActive = false;
		MatchSnapshot.bCaptureContested = false;
		MatchSnapshot.CapturingTeam = INDEX_NONE;
		MatchSnapshot.CapturePercent = 0;
	}
	MatchSnapshot.PhaseEndServerTime = Duration > 0.f ? GetServerWorldTimeSeconds() + Duration : 0.0;
	ForceNetUpdate();
}

void AFMGameState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AFMGameState, MatchSnapshot);
	DOREPLIFETIME(AFMGameState, KillFeed);
}
