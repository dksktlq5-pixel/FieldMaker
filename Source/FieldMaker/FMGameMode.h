#pragma once
#include "GameFramework/GameModeBase.h"
#include "FMGameMode.generated.h"

UCLASS()
class FIELDMAKER_API AFMGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	AFMGameMode();
	virtual FString InitNewPlayer(APlayerController* NewPlayerController, const FUniqueNetIdRepl& UniqueId,
		const FString& Options, const FString& Portal = TEXT("")) override;
	virtual void PostLogin(APlayerController* NewPlayer) override;
	virtual void Logout(AController* Exiting) override;
	virtual void StartPlay() override;
	virtual AActor* ChoosePlayerStart_Implementation(AController* Player) override;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Match")
	bool bTeamMatch = false;
	// Opt-in for development maps only; the actual match still uses phase gates.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Match")
	bool bCombatSandbox = false;
private:
	void AssignTeam(APlayerController* Player);
	void RefreshLobby(AController* Excluding = nullptr);
};
