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
	UFUNCTION(BlueprintPure,Category="Appearance") int32 GetAppearanceCode() const {return int32(AppearanceId);}
 uint32 GetAppearanceId() const { return AppearanceId; }
	void SetAppearanceId(uint32 Id);
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UFUNCTION(BlueprintPure, Category="Economy") int32 GetCredits() const { return Credits; }
	UFUNCTION(BlueprintPure, Category="Economy") int32 GetStructureCount(int32 Index) const { return StructureStock.IsValidIndex(Index) ? StructureStock[Index] : 0; }
	bool SpendCredits(int32 Price);
	void AddCredits(int32 Amount);
	void ResetEconomy(int32 StartingCredits);
	void ChangeStructureStock(int32 Index, int32 Delta);
private:
	UPROPERTY(Replicated) int32 Credits = 0;
	UPROPERTY(Replicated) TArray<int32> StructureStock = {0, 0, 0, 0};
	UPROPERTY(Replicated)
	uint8 Team = UnassignedTeam;

	UPROPERTY(ReplicatedUsing = OnRep_AppearanceId)
	uint32 AppearanceId = FMAppearance::Default;

	UFUNCTION()
	void OnRep_AppearanceId();
};
