#include "FMLobbyWidget.h"
#include "FMGameState.h"
#include "FMPlayerState.h"
#include "FMSteamRoomSubsystem.h"
#include "Engine/GameInstance.h"
#include "Engine/GameViewportClient.h"
#include "Engine/Texture2D.h"
#include "UObject/ConstructorHelpers.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Layout/SBackgroundBlur.h"
#include "Widgets/Layout/SScaleBox.h"
#include "Styling/CoreStyle.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SEditableTextBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/SOverlay.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"

#define LOCTEXT_NAMESPACE "FMLobby"

namespace
{
const FLinearColor RoomText(0.88f, 0.93f, 0.97f);
const FLinearColor RoomMuted(0.61f, 0.70f, 0.78f);
const FLinearColor RoomGold(0.96f, 0.77f, 0.45f);

TSharedRef<STextBlock> RoomLabel(const FSlateFontInfo& Font, int32 Size, const FText& Text,
	const FLinearColor& Color = RoomText)
{
	FSlateFontInfo SizedFont = Font;
	SizedFont.Size = Size;
	return SNew(STextBlock).Font(SizedFont).Text(Text).ColorAndOpacity(Color);
}
}

UFMLobbyWidget::UFMLobbyWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	static ConstructorHelpers::FObjectFinder<UTexture2D> Preview(
		TEXT("/Game/FieldMaker/UI/Frontend/T_FMBridgeLobbyPreview.T_FMBridgeLobbyPreview"));
	MapPreview = Preview.Object;
	MapPreviewBrush.SetResourceObject(MapPreview);
	MapPreviewBrush.DrawAs = ESlateBrushDrawType::Image;
	MapPreviewBrush.ImageSize = FVector2D(380, 178);
}

void UFMLobbyWidget::NativeConstruct()
{
	Super::NativeConstruct();
	if (bInRoom && GetWorld() && !RoomViewport.IsValid())
	{
		if (auto* Viewport = GetWorld()->GetGameViewport())
		{
			RoomViewport = Viewport;
			bPreviousWorldRenderingDisabled = Viewport->bDisableWorldRendering;
			Viewport->bDisableWorldRendering = true;
		}
	}
}

void UFMLobbyWidget::NativeDestruct()
{
	if (auto* Viewport = RoomViewport.Get())
	{
		Viewport->bDisableWorldRendering = bPreviousWorldRenderingDisabled;
	}
	RoomViewport.Reset();
	Super::NativeDestruct();
}

TSharedRef<SWidget> UFMLobbyWidget::RebuildWidget()
{
	SetIsFocusable(true);
	if (bInRoom) return BuildRoom();
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
			.Text(LOCTEXT("PlayTitle", "플레이"))];
	Rows->AddSlot().AutoHeight().Padding(0, 0, 0, 8)
		[Button(LOCTEXT("Create", "방 생성 · 로컬 5대5"), OnCreateRoom)];
	Rows->AddSlot().AutoHeight().Padding(0, 0, 0, 8)
		[Button(LOCTEXT("CreateSteam", "방 생성 · Steam 친구"), OnCreateSteamRoom)];
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
	Rows->AddSlot().AutoHeight().Padding(0, 0, 0, 8)
		[SNew(STextBlock).Font(MenuFont).AutoWrapText(true).Text_Lambda([this] { return Notice; })];
	Rows->AddSlot().AutoHeight()
		[SNew(STextBlock).Font(MenuFont).AutoWrapText(true).Text_Lambda([this] { return GetSteamNotice(); })];
	Rows->AddSlot().AutoHeight()
		[Button(LOCTEXT("Back", "뒤로"), OnClose)];

	const FSlateBrush* WhiteBrush = FCoreStyle::Get().GetBrush("WhiteBrush");
	return SNew(SOverlay)
		+ SOverlay::Slot()
		[SNew(SBorder).BorderImage(WhiteBrush).BorderBackgroundColor(FLinearColor(0.015f, 0.018f, 0.02f, 0.74f))]
		+ SOverlay::Slot().HAlign(HAlign_Center).VAlign(VAlign_Center)
		[SNew(SBox).WidthOverride(470)
			[SNew(SBorder).BorderImage(WhiteBrush).Padding(22)
				.BorderBackgroundColor(FLinearColor(0.055f, 0.061f, 0.071f))[Rows]]];
}

TSharedRef<SWidget> UFMLobbyWidget::BuildRoom()
{
	const auto* White = FCoreStyle::Get().GetBrush("WhiteBrush");
	const bool bHost = GetOwningPlayer() && GetOwningPlayer()->HasAuthority();
	const auto Header = SNew(SHorizontalBox)
		+ SHorizontalBox::Slot().FillWidth(1)
		[RoomLabel(MenuFont, 16, LOCTEXT("Brand", "FIELDMAKER  /  플레이"), RoomGold)]
		+ SHorizontalBox::Slot().AutoWidth()
		[RoomLabel(MenuFont, 14, bSteamRoom ? LOCTEXT("SteamRoomType", "Steam 친구 대기방") :
			LOCTEXT("RoomType", "로컬 사용자 설정 경기"), RoomMuted)];

	const auto MapCard = SNew(SBox).WidthOverride(380).HeightOverride(178)
		[SNew(SOverlay)
			+ SOverlay::Slot()
			[SNew(SImage).Image(&MapPreviewBrush)]
			+ SOverlay::Slot().VAlign(VAlign_Bottom)
			[SNew(SBorder).BorderImage(White).BorderBackgroundColor(FLinearColor(0.025f, 0.044f, 0.06f, 0.90f))
				.Padding(FMargin(16, 10))
				[SNew(SHorizontalBox)
					+ SHorizontalBox::Slot().FillWidth(1)
					[RoomLabel(MenuFont, 20, LOCTEXT("MapName", "브리지 전장"))]
					+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
					[RoomLabel(MenuFont, 13, LOCTEXT("MapBadge", "전장"), RoomGold)]]]];

	const auto Teams = SNew(SHorizontalBox)
		+ SHorizontalBox::Slot().FillWidth(1).Padding(0, 0, 6, 0)
		[BuildTeam(1, LOCTEXT("RedTeam", "레드 팀"), FLinearColor(0.91f, 0.29f, 0.36f))]
		+ SHorizontalBox::Slot().FillWidth(1).Padding(6, 0, 0, 0)
		[BuildTeam(0, LOCTEXT("BlueTeam", "블루 팀"), FLinearColor(0.26f, 0.73f, 0.81f))];

	const auto Footer = SNew(SHorizontalBox)
		+ SHorizontalBox::Slot().FillWidth(1).VAlign(VAlign_Center)
		[SNew(SButton).HAlign(HAlign_Center).ContentPadding(FMargin(18, 10))
			.OnClicked_Lambda([this] { OnClose.ExecuteIfBound(); return FReply::Handled(); })
			[RoomLabel(MenuFont, 16, LOCTEXT("Leave", "방 나가기"))]];

	if (bHost)
	{
		Footer->AddSlot().FillWidth(1.45f).Padding(14, 0)
			[SNew(SButton).HAlign(HAlign_Center).ContentPadding(FMargin(24, 14))
				.ButtonColorAndOpacity(FLinearColor(0.22f, 0.70f, 0.68f))
				.IsEnabled_Lambda([this] { return CanStart(); })
				.OnClicked_Lambda([this] { OnStart.ExecuteIfBound(); return FReply::Handled(); })
				[RoomLabel(MenuFont, 20, LOCTEXT("Start", "경기 테스트 시작"))]];
		Footer->AddSlot().FillWidth(1).VAlign(VAlign_Center)
			[SNew(SButton).HAlign(HAlign_Center).ContentPadding(FMargin(18, 10))
				.OnClicked_Lambda([this]
				{
					if (bSteamRoom) OnInviteFriends.ExecuteIfBound();
					else bShowHostAddress = !bShowHostAddress;
					return FReply::Handled();
				})
				[SNew(STextBlock).Font(MenuFont).ColorAndOpacity(RoomText)
					.Text_Lambda([this]
					{
						if (bSteamRoom) return LOCTEXT("SteamInvite", "Steam 친구 초대");
						return bShowHostAddress ? LOCTEXT("HideAddress", "참가 주소 숨기기") : LOCTEXT("ShowAddress", "참가 주소 보기");
					})]];
	}
	else
	{
		Footer->AddSlot().FillWidth(2.45f).Padding(14, 0, 0, 0).VAlign(VAlign_Center)
			[RoomLabel(MenuFont, 16, LOCTEXT("WaitHost", "방장이 경기를 시작할 때까지 기다리는 중"), RoomMuted)];
	}

	const auto Content = SNew(SVerticalBox)
		+ SVerticalBox::Slot().AutoHeight()
		[SNew(SBorder).BorderImage(White).BorderBackgroundColor(FLinearColor(0.035f, 0.055f, 0.08f, 0.95f))
			.Padding(FMargin(18, 12))[Header]]
		+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(0, 20, 0, 4)
		[RoomLabel(MenuFont, 32, LOCTEXT("RoomTitle", "5대5 대기방"))]
		+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(0, 0, 0, 12)
		[SNew(STextBlock).Font(MenuFont).ColorAndOpacity(RoomMuted)
			.Text_Lambda([this] { return GetRoomStatus(); })]
		+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center)
		[MapCard]
		+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(0, 9, 0, 16)
		[RoomLabel(MenuFont, 14, LOCTEXT("Settings", "모드   5대5 교전 테스트     /     정원   10명"), RoomMuted)]
		+ SVerticalBox::Slot().AutoHeight()
		[Teams]
		+ SVerticalBox::Slot().AutoHeight().Padding(0, 16, 0, 0)
		[Footer]
		+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Right)
		[SNew(STextBlock).Font(MenuFont).ColorAndOpacity(RoomMuted)
			.Text_Lambda([this] { return GetSteamNotice(); })
			.Visibility(bSteamRoom ? EVisibility::Visible : EVisibility::Collapsed)]
		+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Right).Padding(0, 8, 0, 0)
		[SNew(STextBlock).Font(MenuFont).ColorAndOpacity(RoomMuted).Justification(ETextJustify::Right)
			.Text(HostAddress)
			.Visibility_Lambda([this] { return bShowHostAddress ? EVisibility::Visible : EVisibility::Collapsed; })];

	return SNew(SOverlay)
		+ SOverlay::Slot()[SNew(SImage).Image(&MapPreviewBrush)]
		+ SOverlay::Slot()[SNew(SBackgroundBlur).BlurStrength(8.f)]
		+ SOverlay::Slot()
		[SNew(SBorder).BorderImage(White).BorderBackgroundColor(FLinearColor(0.032f, 0.063f, 0.105f, 0.72f))]
		+ SOverlay::Slot().HAlign(HAlign_Center).VAlign(VAlign_Center).Padding(26)
		[SNew(SScaleBox).Stretch(EStretch::ScaleToFit).StretchDirection(EStretchDirection::DownOnly)
			[SNew(SBox).WidthOverride(1000)[Content]]];
}

TSharedRef<SWidget> UFMLobbyWidget::BuildTeam(uint8 Team, const FText& Title, const FLinearColor& Accent)
{
	const auto* White = FCoreStyle::Get().GetBrush("WhiteBrush");
	const auto Slots = SNew(SVerticalBox)
		+ SVerticalBox::Slot().AutoHeight()
		[SNew(SBorder).BorderImage(White).BorderBackgroundColor(Accent.CopyWithNewOpacity(0.78f))
			.Padding(FMargin(16, 8))
			[RoomLabel(MenuFont, 18, Title)]];
	for (int32 SeatIndex = 0; SeatIndex < AFMGameState::TeamCapacity; ++SeatIndex)
	{
		Slots->AddSlot().AutoHeight().Padding(0, 2, 0, 0)
			[SNew(SBox).HeightOverride(42)
				[SNew(SBorder).BorderImage(White)
					.BorderBackgroundColor(Accent * 0.14f + FLinearColor(0.016f, 0.022f, 0.032f, 0.70f))
					.Padding(FMargin(12, 5))
					[SNew(SHorizontalBox)
						+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0, 0, 12, 0)
						[SNew(SBorder).BorderImage(White).BorderBackgroundColor(Accent.CopyWithNewOpacity(0.23f))
							.Padding(FMargin(10, 4))[RoomLabel(MenuFont, 15, FText::AsNumber(SeatIndex + 1), Accent)]]
						+ SHorizontalBox::Slot().FillWidth(1).VAlign(VAlign_Center)
						[SNew(STextBlock).Font(MenuFont).ColorAndOpacity(RoomText)
							.OverflowPolicy(ETextOverflowPolicy::Ellipsis)
							.Text_Lambda([this, Team, SeatIndex] { return GetTeamMember(Team, SeatIndex); })]]]];
	}
	return Slots;
}

FText UFMLobbyWidget::GetTeamMember(uint8 Team, int32 SeatIndex) const
{
	const auto* State = GetWorld() ? GetWorld()->GetGameState<AFMGameState>() : nullptr;
	if (State)
	{
		for (const APlayerState* Player : State->PlayerArray)
		{
			if (const auto* Member = Cast<AFMPlayerState>(Player); Member && Member->GetTeam() == Team)
			{
				if (SeatIndex-- != 0) continue;
				const FText Name = FText::FromString(Member->GetPlayerName());
				return Member == GetOwningPlayerState() ? FText::Format(LOCTEXT("LocalName", "{0}  ·  나"), Name) : Name;
			}
		}
	}
	return LOCTEXT("Empty", "플레이어 대기 중");
}

FText UFMLobbyWidget::GetRoomStatus() const
{
	const auto* State = GetWorld() ? GetWorld()->GetGameState<AFMGameState>() : nullptr;
	const int32 Players = State ? State->PlayerArray.Num() : 0;
	return FText::Format(LOCTEXT("RoomStatus", "{0} / {1}  ·  {2}"),
		FText::AsNumber(Players), FText::AsNumber(AFMGameState::RoomCapacity),
		CanStart() ? LOCTEXT("ReadyStatus", "시작 가능") : LOCTEXT("WaitingStatus", "상대 입장 대기 중"));
}

FText UFMLobbyWidget::GetSteamNotice() const
{
	const auto* Steam = GetGameInstance() ? GetGameInstance()->GetSubsystem<UFMSteamRoomSubsystem>() : nullptr;
	return Steam ? Steam->GetNotice() : FText::GetEmpty();
}

FReply UFMLobbyWidget::Join()
{
	if (AddressBox && OnJoinRoom.IsBound()) OnJoinRoom.Execute(AddressBox->GetText().ToString());
	return FReply::Handled();
}

bool UFMLobbyWidget::CanStart() const
{
	const auto* State = GetWorld() ? GetWorld()->GetGameState<AFMGameState>() : nullptr;
	return State && State->GetMatchSnapshot().Phase == EFMMatchPhase::Ready;
}

#undef LOCTEXT_NAMESPACE
