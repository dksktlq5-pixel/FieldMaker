#pragma once

#include "GameFramework/GameStateBase.h"
#include "FMGameState.generated.h"

UENUM(BlueprintType)
enum class EFMMatchPhase : uint8
{
	Practice, WaitingForPlayers, Ready, KnifePreparation, KnifeCombat,
	Purchase, Building, Countdown, Combat, Result, SideSwap, Finished, CombatTest, Deploying
};

UENUM(BlueprintType)
enum class EFMResultReason : uint8
{
	None, Elimination, DoubleElimination, Capture, TimeExpired, TeamLeft
};

USTRUCT(BlueprintType)
struct FFMKillFeedEntry
{
	GENERATED_BODY()
	UPROPERTY(BlueprintReadOnly) FString Killer;
	UPROPERTY(BlueprintReadOnly) FString Victim;
	UPROPERTY(BlueprintReadOnly) FString Weapon;
	UPROPERTY(BlueprintReadOnly) double ServerTime = 0.0;
};

// One replicated snapshot keeps scores, round and phase changes together.
USTRUCT(BlueprintType)
struct FFMMatchSnapshot
{
	GENERATED_BODY()
	UPROPERTY(BlueprintReadOnly) EFMMatchPhase Phase = EFMMatchPhase::Practice;
	UPROPERTY(BlueprintReadOnly) int32 RoundNumber = 0;
	UPROPERTY(BlueprintReadOnly) int32 RoundWinner = -1;
	UPROPERTY(BlueprintReadOnly) EFMResultReason ResultReason = EFMResultReason::None;
	UPROPERTY(BlueprintReadOnly) bool bRoundResultRecorded = false;
	UPROPERTY(BlueprintReadOnly) int32 Team0Score = 0;
	UPROPERTY(BlueprintReadOnly) int32 Team1Score = 0;
	UPROPERTY(BlueprintReadOnly) double PhaseEndServerTime = 0.0;
	UPROPERTY(BlueprintReadOnly) FVector CaptureCenter = FVector::ZeroVector;
	UPROPERTY(BlueprintReadOnly) int32 CapturePercent = 0;
	UPROPERTY(BlueprintReadOnly) int32 CapturingTeam = INDEX_NONE;
	UPROPERTY(BlueprintReadOnly) bool bCaptureActive = false;
	UPROPERTY(BlueprintReadOnly) bool bCaptureContested = false;
	UPROPERTY(BlueprintReadOnly) bool bSidesSwapped = false;
	UPROPERTY(BlueprintReadOnly) bool bPlayerRoom = false;
	UPROPERTY(BlueprintReadOnly) bool bSteamRoom = false;
};

UCLASS()
class FIELDMAKER_API AFMGameState : public AGameStateBase
{
	GENERATED_BODY()
public:
	static constexpr int32 TeamCapacity = 5;
	static constexpr int32 RoomCapacity = 2 * TeamCapacity;

	AFMGameState();
	UFUNCTION(BlueprintPure, Category="Match")
	FFMMatchSnapshot GetMatchSnapshot() const { return MatchSnapshot; }
	UFUNCTION(BlueprintPure, Category="Match")
	float GetPhaseTimeRemaining() const;
	bool RecordRoundResult(int32 Winner, EFMResultReason Reason);
	void AdvanceRound(bool bSwapSides);
	void UpdateCapture(const FVector& Center, int32 Team, int32 Percent, bool bContested);
	void AddKillFeed(const FString& Killer, const FString& Victim, const FString& Weapon);
	const TArray<FFMKillFeedEntry>& GetKillFeed() const { return KillFeed; }
	void SetLobbyReady(bool bReady);
	void InitializePlayerRoom(bool bSteamRoom);
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category="Match")
	void SetPhase(EFMMatchPhase Phase, float Duration = 0.f);
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
private:
	UPROPERTY(Replicated)
	FFMMatchSnapshot MatchSnapshot;
	UPROPERTY(Replicated)
	TArray<FFMKillFeedEntry> KillFeed;
};
