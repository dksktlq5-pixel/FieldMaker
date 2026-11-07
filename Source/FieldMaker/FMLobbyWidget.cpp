#include "FMLobbyWidget.h"
#include "FMGameState.h"
#include "FMPlayerState.h"
#include "Styling/CoreStyle.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SEditableTextBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/SOverlay.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"

#define LOCTEXT_NAMESPACE "FMLobby"

TSharedRef<SWidget> UFMLobbyWidget::RebuildWidget()
{
	SetIsFocusable(true);
	FSlateFontInfo Heading = MenuFont;
	Heading.Size = 26;
	MenuFont.Size = 16;
	const auto Button = [this](const FText& Text, const FSimpleDelegate& Action)
	{
		return SNew(SButton)
			.HAlign(HAlign_Center)
			.ContentPadding(FMargin(14, 9))
			.OnClicked_Lambda([Action] { Action.ExecuteIfBound(); return FReply::Handled(); })
			[SNew(STextBlock).Font(MenuFont).Text(Text)];
	};

	const auto Rows = SNew(SVerticalBox);
	Rows->AddSlot().AutoHeight().Padding(0, 0, 0, 16)
		[SNew(STextBlock).Font(Heading).ColorAndOpacity(FLinearColor(0.88f, 0.66f, 0.31f))
			.Text(bInRoom ? LOCTEXT("RoomTitle", "1대1 대기방") : LOCTEXT("PlayTitle", "플레이"))];
	if (bInRoom)
	{
		Rows->AddSlot().AutoHeight().Padding(0, 0, 0, 12)
			[SNew(STextBlock).Font(MenuFont).Text_Lambda([this] { return GetRoster(); })];
		if (GetOwningPlayer() && GetOwningPlayer()->HasAuthority())
		{
			Rows->AddSlot().AutoHeight().Padding(0, 0, 0, 8)
				[SNew(STextBlock).Font(MenuFont).Text(HostAddress).AutoWrapText(true)];
			Rows->AddSlot().AutoHeight().Padding(0, 0, 0, 8)
				[SNew(SButton).HAlign(HAlign_Center).ContentPadding(FMargin(14, 9))
					.IsEnabled_Lambda([this] { return CanStart(); })
					.OnClicked_Lambda([this] { OnStart.ExecuteIfBound(); return FReply::Handled(); })
					[SNew(STextBlock).Font(MenuFont).Text(LOCTEXT("Start", "경기 테스트 시작"))]];
		}
		else
		{
			Rows->AddSlot().AutoHeight().Padding(0, 0, 0, 12)
				[SNew(STextBlock).Font(MenuFont).Text(LOCTEXT("WaitHost", "방장이 경기를 시작할 때까지 기다리는 중"))];
		}
	}
	else
	{
		Rows->AddSlot().AutoHeight().Padding(0, 0, 0, 8)
			[Button(LOCTEXT("Create", "방 생성 · 로컬 1대1"), OnCreateRoom)];
		Rows->AddSlot().AutoHeight().Padding(0, 6, 0, 8)
			[SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().FillWidth(1).Padding(0, 0, 8, 0)
				[SAssignNew(AddressBox, SEditableTextBox)
					.Font(MenuFont).HintText(LOCTEXT("Address", "호스트 IP 또는 IP:포트"))
					.OnTextCommitted_Lambda([this](const FText&, ETextCommit::Type Commit)
						{ if (Commit == ETextCommit::OnEnter) Join(); })]
				+ SHorizontalBox::Slot().AutoWidth()
				[SNew(SButton).ContentPadding(FMargin(14, 9)).OnClicked_Lambda([this] { return Join(); })
					[SNew(STextBlock).Font(MenuFont).Text(LOCTEXT("Join", "참가"))]]];
		Rows->AddSlot().AutoHeight().Padding(0, 0, 0, 8)
			[Button(LOCTEXT("Practice", "혼자 연습"), OnPractice)];
	}
	Rows->AddSlot().AutoHeight().Padding(0, 0, 0, 8)
		[SNew(STextBlock).Font(MenuFont).AutoWrapText(true).Text_Lambda([this] { return Notice; })];
	Rows->AddSlot().AutoHeight()
		[Button(bInRoom ? LOCTEXT("Leave", "방 나가기") : LOCTEXT("Back", "뒤로"), OnClose)];

	const FSlateBrush* WhiteBrush = FCoreStyle::Get().GetBrush("WhiteBrush");
	return SNew(SOverlay)
		+ SOverlay::Slot()
		[SNew(SBorder).BorderImage(WhiteBrush).BorderBackgroundColor(FLinearColor(0.015f, 0.018f, 0.02f, 0.74f))]
		+ SOverlay::Slot().HAlign(HAlign_Center).VAlign(VAlign_Center)
		[SNew(SBox).WidthOverride(470)
			[SNew(SBorder).BorderImage(WhiteBrush).Padding(22)
				.BorderBackgroundColor(FLinearColor(0.055f, 0.061f, 0.071f))[Rows]]];
}

FReply UFMLobbyWidget::Join()
{
	if (AddressBox && OnJoinRoom.IsBound()) OnJoinRoom.Execute(AddressBox->GetText().ToString());
	return FReply::Handled();
}

FText UFMLobbyWidget::GetRoster() const
{
	const auto* State = GetWorld() ? GetWorld()->GetGameState<AFMGameState>() : nullptr;
	FString Slots[] = {LOCTEXT("Empty", "대기 중").ToString(), LOCTEXT("Empty", "대기 중").ToString()};
	int32 Count = 0;
	if (State)
	{
		for (const APlayerState* Player : State->PlayerArray)
		{
			const auto* Member = Cast<AFMPlayerState>(Player);
			if (Member && Member->GetTeam() < 2)
			{
				Slots[Member->GetTeam()] = Member->GetPlayerName();
				++Count;
			}
		}
	}
	return FText::Format(LOCTEXT("Roster", "{0}/2명 입장\n\n블루   {1}\n레드   {2}"),
		FText::AsNumber(Count), FText::FromString(Slots[0]), FText::FromString(Slots[1]));
}

bool UFMLobbyWidget::CanStart() const
{
	const auto* State = GetWorld() ? GetWorld()->GetGameState<AFMGameState>() : nullptr;
	return State && State->GetMatchSnapshot().Phase == EFMMatchPhase::Ready;
}

#undef LOCTEXT_NAMESPACE
