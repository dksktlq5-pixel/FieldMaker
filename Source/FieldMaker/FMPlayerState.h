#pragma once

#include "FMAppearanceSaveGame.h"
#include "GameFramework/PlayerState.h"
#include "FMPlayerState.generated.h"

UCLASS()
class AFMPlayerState : public APlayerState
{
	GENERATED_BODY()

public:
	static constexpr uint8 UnassignedTeam = 2;
	UFUNCTION(BlueprintPure, Category="Team")
	uint8 GetTeam() const { return Team; }
	void SetTeam(uint8 InTeam) { if (HasAuthority()) { Team = InTeam; } }
	uint8 GetAppearanceId() const { return AppearanceId; }
	void SetAppearanceId(uint8 Id);
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

private:
	UPROPERTY(Replicated)
	uint8 Team = UnassignedTeam;

	UPROPERTY(ReplicatedUsing = OnRep_AppearanceId)
	uint8 AppearanceId = FMAppearance::Default;

	UFUNCTION()
	void OnRep_AppearanceId();
};
