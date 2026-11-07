#pragma once

#include "Blueprint/UserWidget.h"
#include "FMLobbyWidget.generated.h"

UCLASS()
class UFMLobbyWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFMLobbyWidget(const FObjectInitializer& ObjectInitializer);
	FSimpleDelegate OnCreateRoom;
	FSimpleDelegate OnCreateSteamRoom;
	FSimpleDelegate OnInviteFriends;
	FSimpleDelegate OnPractice;
	FSimpleDelegate OnStart;
	FSimpleDelegate OnClose;
	TDelegate<bool(const FString&)> OnJoinRoom;
	FSlateFontInfo MenuFont;
	bool bInRoom = false;
	bool bSteamRoom = false;
	FText HostAddress;
	FText Notice;

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;

private:
	TSharedRef<SWidget> BuildRoom();
	TSharedRef<SWidget> BuildTeam(uint8 Team, const FText& Title, const FLinearColor& Accent);
	FText GetTeamMember(uint8 Team) const;
	FText GetRoomStatus() const;
	FText GetSteamNotice() const;
	bool CanStart() const;
	FReply Join();
	TSharedPtr<class SEditableTextBox> AddressBox;
	UPROPERTY()
	TObjectPtr<class UTexture2D> MapPreview;
	FSlateBrush MapPreviewBrush;
	bool bShowHostAddress = false;
};
