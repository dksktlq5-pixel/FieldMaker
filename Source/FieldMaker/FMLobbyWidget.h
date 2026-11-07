#pragma once

#include "Blueprint/UserWidget.h"
#include "FMLobbyWidget.generated.h"

UCLASS()
class UFMLobbyWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	FSimpleDelegate OnCreateRoom;
	FSimpleDelegate OnPractice;
	FSimpleDelegate OnStart;
	FSimpleDelegate OnClose;
	TDelegate<bool(const FString&)> OnJoinRoom;
	FSlateFontInfo MenuFont;
	bool bInRoom = false;
	FText HostAddress;
	FText Notice;

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;

private:
	FText GetRoster() const;
	bool CanStart() const;
	FReply Join();
	TSharedPtr<class SEditableTextBox> AddressBox;
};
