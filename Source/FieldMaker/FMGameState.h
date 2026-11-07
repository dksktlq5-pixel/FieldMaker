#pragma once

#include "GameFramework/GameStateBase.h"
#include "FMGameState.generated.h"

UENUM(BlueprintType)
enum class EFMMatchPhase : uint8
{
	Practice, WaitingForPlayers, Ready, KnifePreparation, KnifeCombat,
	Purchase, Building, Countdown, Combat, Result, SideSwap, Finished, CombatTest
};

// One replicated snapshot keeps scores, round and phase changes together.
USTRUCT(BlueprintType)
struct FFMMatchSnapshot
{
	GENERATED_BODY()
	UPROPERTY(BlueprintReadOnly) EFMMatchPhase Phase = EFMMatchPhase::Practice;
	UPROPERTY(BlueprintReadOnly) int32 RoundNumber = 0;
	UPROPERTY(BlueprintReadOnly) int32 Team0Score = 0;
	UPROPERTY(BlueprintReadOnly) int32 Team1Score = 0;
	UPROPERTY(BlueprintReadOnly) double PhaseEndServerTime = 0.0;
	UPROPERTY(BlueprintReadOnly) bool bSidesSwapped = false;
	UPROPERTY(BlueprintReadOnly) bool bPlayerRoom = false;
};

UCLASS()
class FIELDMAKER_API AFMGameState : public AGameStateBase
{
	GENERATED_BODY()
public:
	AFMGameState();
	UFUNCTION(BlueprintPure, Category="Match")
	FFMMatchSnapshot GetMatchSnapshot() const { return MatchSnapshot; }
	UFUNCTION(BlueprintPure, Category="Match")
	float GetPhaseTimeRemaining() const;
	void SetLobbyReady(bool bReady);
	void InitializePlayerRoom();
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category="Match")
	void SetPhase(EFMMatchPhase Phase, float Duration = 0.f);
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
private:
	UPROPERTY(Replicated)
	FFMMatchSnapshot MatchSnapshot;
};
