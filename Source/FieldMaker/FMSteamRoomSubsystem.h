#pragma once

#include "Interfaces/OnlineSessionInterface.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "FMSteamRoomSubsystem.generated.h"

UCLASS()
class UFMSteamRoomSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;
	virtual UWorld* GetWorld() const override;
	void BindInvites();
	void CreateRoom();
	void InviteFriends();
	void LeaveRoom();
	bool IsBusy() const;
	FText GetNotice() const { return Notice; }

private:
	IOnlineSessionPtr GetSessions() const;
	void Unbind();
	void OnInviteAccepted(bool bSuccess, int32 ControllerId, FUniqueNetIdPtr UserId,
		const FOnlineSessionSearchResult& Result);
	void OnCreated(FName Name, bool bSuccess);
	void OnJoined(FName Name, EOnJoinSessionCompleteResult::Type Result);
	void OnMapLoaded(UWorld* World);
	void OnNetworkFailure(UWorld* World, UNetDriver* Driver, ENetworkFailure::Type Type, const FString& Error);
	void OnTravelFailure(UWorld* World, ETravelFailure::Type Type, const FString& Error);
	void TravelFailed(UWorld* World);

	IOnlineSessionPtr BoundSessions;
	FDelegateHandle InviteHandle;
	FDelegateHandle CreateHandle;
	FDelegateHandle JoinHandle;
	bool bTravelPending = false;
	FText Notice;
};
