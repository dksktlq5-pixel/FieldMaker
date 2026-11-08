#pragma once
#include "GameFramework/GameModeBase.h"
#include "FMGameState.h"
#include "TimerManager.h"
#include "FMGameMode.generated.h"

UCLASS()
class FIELDMAKER_API AFMGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	AFMGameMode();
	virtual void InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage) override;
	virtual void InitGameState() override;
	virtual void PreLogin(const FString& Options, const FString& Address, const FUniqueNetIdRepl& UniqueId,
		FString& ErrorMessage) override;
	virtual FString InitNewPlayer(APlayerController* NewPlayerController, const FUniqueNetIdRepl& UniqueId,
		const FString& Options, const FString& Portal = TEXT("")) override;
	virtual void PostLogin(APlayerController* NewPlayer) override;
	virtual void Logout(AController* Exiting) override;
	virtual void StartPlay() override;
	virtual AActor* ChoosePlayerStart_Implementation(AController* Player) override;
	virtual bool PlayerCanRestart_Implementation(APlayerController* Player) override;
	bool StartPlayerRoom(APlayerController* Requester);

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Match")
	bool bTeamMatch = false;
	// Opt-in for development maps only; the actual match still uses phase gates.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Match")
	bool bCombatSandbox = false;
	bool IsInPreparationZone(const AController* Player, const FVector& Location, bool bSpawnOnly) const;
private:
	void SetRoundPhase(EFMMatchPhase Phase, float Duration);
	void UpdateRound();
	void EndRound(int32 Winner);
	void ReturnPlayersToSpawn(bool bResetRound, bool bResetEquipment);
	void ClearStructures();
	const AActor* GetTeamStart(uint8 Side) const;
	FTimerHandle RoundTimer;
	double CaptureProgress = 0;
	double LastRoundUpdate = 0;
	int32 CaptureTeam = INDEX_NONE;
	bool bPlayerRoom = false;
	bool bRoomStarted = false;
	FString GetRoomAdmissionError(APlayerController* PendingPlayer = nullptr);
	void AssignTeam(APlayerController* Player);
	void RefreshLobby(AController* Excluding = nullptr);
};
