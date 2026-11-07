#include "FMSettingsWidget.h"

#include "FMUserPreferences.h"
#include "GameFramework/GameUserSettings.h"
#include "HAL/PlatformTime.h"
#include "InputMappingContext.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Misc/CoreDelegates.h"
#include "Scalability.h"
#include "Styling/CoreStyle.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SSlider.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SScaleBox.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/SOverlay.h"
#include "Widgets/Text/STextBlock.h"

#define LOCTEXT_NAMESPACE "FMSettings"

namespace
{
const FLinearColor Gold(0.70f, 0.47f, 0.15f);
const FLinearColor Ink(0.021f, 0.024f, 0.026f);
const FLinearColor White(0.93f, 0.92f, 0.88f);
const FLinearColor Muted(0.54f, 0.55f, 0.53f);
constexpr int32 CompetitiveQuality = 4;
constexpr float CompetitiveResolutionScale = 85.f;
constexpr float FrameLimits[] = {0.f, 60.f, 120.f, 144.f, 165.f, 240.f};

struct FDisplayDraft
{
	FIntPoint Resolution;
	EWindowMode::Type Mode;
	bool bVSync;
	int32 Quality;
	float FrameLimit;

	explicit FDisplayDraft(UGameUserSettings& Settings)
		: Resolution(Settings.GetScreenResolution()), Mode(Settings.GetFullscreenMode()),
		bVSync(Settings.IsVSyncEnabled()), Quality(Settings.GetOverallScalabilityLevel()),
		FrameLimit(Settings.GetFrameRateLimit())
	{
		if (Quality < 0 && Settings.GetGlobalIlluminationQuality() == 0 && Settings.GetReflectionQuality() == 0 &&
			Settings.GetShadowQuality() == 1 && Settings.GetTextureQuality() == 2 && Settings.GetViewDistanceQuality() == 2)
		{
			Quality = CompetitiveQuality;
		}
	}
};

class SFMSettingsPanel : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SFMSettingsPanel) {}
		SLATE_ARGUMENT(FButtonStyle, ButtonStyle)
		SLATE_ARGUMENT(FSlateFontInfo, MenuFont)
		SLATE_EVENT(FSimpleDelegate, OnClosed)
	SLATE_END_ARGS()

	void Construct(const FArguments& Args)
	{
		ButtonStyle = Args._ButtonStyle;
		Font = Args._MenuFont;
		Close = Args._OnClosed;
		SavedLocal = FMLocalPreferences::Get();
		Local = SavedLocal;
		UKismetSystemLibrary::GetSupportedFullscreenResolutions(Resolutions);
		Resolutions.AddUnique(Display.Resolution);
		Resolutions.Sort([](FIntPoint A, FIntPoint B) { return A.X == B.X ? A.Y < B.Y : A.X < B.X; });

		ChildSlot
		[
			SNew(SOverlay)
			+ SOverlay::Slot()
			[
				SNew(SBorder).BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush"))
				.BorderBackgroundColor(FLinearColor(0, 0, 0, 0.75f))
			]
			+ SOverlay::Slot().HAlign(HAlign_Center).VAlign(VAlign_Center)
			[
				SNew(SScaleBox).Stretch(EStretch::ScaleToFit).StretchDirection(EStretchDirection::DownOnly)
				[
					SNew(SBox).WidthOverride(850).HeightOverride(540)
					[
						SNew(SBorder).BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush"))
						.BorderBackgroundColor(Gold).Padding(1)
						[
							SNew(SBorder).BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush"))
							.BorderBackgroundColor(Ink).Padding(24)
							[ SAssignNew(Content, SBox) ]
						]
					]
				]
			]
			+ SOverlay::Slot()
			[ SAssignNew(Modal, SBox).Visibility(EVisibility::Collapsed) ]
		];
		BuildPage();
	}

	virtual ~SFMSettingsPanel() override
	{
		RollbackVideo();
	}

	virtual bool SupportsKeyboardFocus() const override { return true; }

private:
	enum class EDialog { None, Reset, Video };
	UGameUserSettings* Settings = UGameUserSettings::GetGameUserSettings();
	FDisplayDraft Display{*Settings};
	FDisplayDraft SavedDisplay{*Settings};
	FMLocalPreferences Local;
	FMLocalPreferences SavedLocal;
	FButtonStyle ButtonStyle;
	FSlateFontInfo Font;
	FSimpleDelegate Close;
	TArray<FIntPoint> Resolutions;
	TSharedPtr<SBox> Content;
	TSharedPtr<SBox> Modal;
	int32 Tab = 0;
	EDialog Dialog = EDialog::None;
	double Deadline = 0.0;

	TSharedRef<STextBlock> Text(TAttribute<FText> Value, int32 Size = 15, FLinearColor Color = White) const
	{
		FSlateFontInfo Sized = Font;
		Sized.Size = Size;
		return SNew(STextBlock).Text(Value).Font(Sized).ColorAndOpacity(FSlateColor(Color));
	}

	TSharedRef<SButton> Button(TAttribute<FText> Label, FSimpleDelegate Action, FLinearColor Color = White)
	{
		return SNew(SButton).ButtonStyle(&ButtonStyle).Cursor(EMouseCursor::Hand)
			.OnClicked_Lambda([Action] { Action.ExecuteIfBound(); return FReply::Handled(); })
			[ Text(Label, 15, Color) ];
	}

	void Row(SVerticalBox& Rows, const FText& Label, TSharedRef<SWidget> Control)
	{
		Rows.AddSlot().AutoHeight().Padding(0, 4)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
			[ SNew(SBox).WidthOverride(180) [ Text(Label) ] ]
			+ SHorizontalBox::Slot().FillWidth(1).VAlign(VAlign_Center)
			[ Control ]
		];
	}

	void BuildPage()
	{
		const FText Tabs[] = {LOCTEXT("Game", "게임"), LOCTEXT("Display", "화면"), LOCTEXT("Audio", "오디오"), LOCTEXT("Controls", "조작")};
		auto TabsBar = SNew(SHorizontalBox);
		for (int32 Index = 0; Index < UE_ARRAY_COUNT(Tabs); ++Index)
		{
			TabsBar->AddSlot().AutoWidth().Padding(0, 0, 8, 0)
			[ Button(Tabs[Index], FSimpleDelegate::CreateLambda([this, Index] { Tab = Index; BuildPage(); }), Index == Tab ? Gold : White) ];
		}

		auto Rows = SNew(SVerticalBox);
		switch (Tab)
		{
		case 0: BuildGame(*Rows); break;
		case 1: BuildDisplay(*Rows); break;
		case 2: BuildAudio(*Rows); break;
		case 3: BuildControls(*Rows); break;
		}

		Content->SetContent(
			SNew(SVerticalBox)
			+ SVerticalBox::Slot().AutoHeight().Padding(0, 0, 0, 12)
			[ Text(LOCTEXT("Title", "FIELDMAKER  /  설정"), 28, Gold) ]
			+ SVerticalBox::Slot().AutoHeight().Padding(0, 0, 0, 16) [ TabsBar ]
			+ SVerticalBox::Slot().FillHeight(1) [ SNew(SScrollBox) + SScrollBox::Slot() [ Rows ] ]
			+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Right).Padding(0, 12, 0, 0)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().AutoWidth().Padding(4) [ Button(LOCTEXT("Apply", "적용"), FSimpleDelegate::CreateSP(this, &SFMSettingsPanel::Apply), Gold) ]
				+ SHorizontalBox::Slot().AutoWidth().Padding(4) [ Button(LOCTEXT("Cancel", "취소"), FSimpleDelegate::CreateSP(this, &SFMSettingsPanel::Discard)) ]
				+ SHorizontalBox::Slot().AutoWidth().Padding(4) [ Button(LOCTEXT("Defaults", "기본값"), FSimpleDelegate::CreateSP(this, &SFMSettingsPanel::Defaults)) ]
				+ SHorizontalBox::Slot().AutoWidth().Padding(4) [ Button(LOCTEXT("Close", "닫기"), Close) ]
			]);
	}

	void BuildGame(SVerticalBox& Rows)
	{
		Row(Rows, LOCTEXT("Language", "언어"), Text(LOCTEXT("LanguageUnavailable", "한국어 · 언어 변경 미지원"), 15, Muted));
		Row(Rows, LOCTEXT("Reset", "설정 초기화"), Button(LOCTEXT("Reset", "설정 초기화"),
			FSimpleDelegate::CreateLambda([this] { ShowDialog(EDialog::Reset); })));
	}

	void BuildDisplay(SVerticalBox& Rows)
	{
		Row(Rows, LOCTEXT("Resolution", "해상도"),
			Button(TAttribute<FText>::CreateLambda([this] {
				return FText::FromString(FString::Printf(TEXT("%d × %d   ›"), Display.Resolution.X, Display.Resolution.Y));
			}), FSimpleDelegate::CreateLambda([this] {
				if (!Resolutions.IsEmpty())
				{
					Display.Resolution = Resolutions[(Resolutions.IndexOfByKey(Display.Resolution) + 1) % Resolutions.Num()];
				}
			})));
		Row(Rows, LOCTEXT("WindowMode", "화면 모드"),
			Button(TAttribute<FText>::CreateLambda([this] {
				const FText Modes[] = {LOCTEXT("Fullscreen", "전체 화면   ›"), LOCTEXT("Borderless", "무테 창 모드   ›"), LOCTEXT("Windowed", "창 모드   ›")};
				return Modes[static_cast<int32>(Display.Mode)];
			}), FSimpleDelegate::CreateLambda([this] { Display.Mode = static_cast<EWindowMode::Type>((Display.Mode + 1) % 3); })));
		Row(Rows, LOCTEXT("VSync", "수직 동기화"),
			Button(TAttribute<FText>::CreateLambda([this] { return ToggleText(Display.bVSync); }),
				FSimpleDelegate::CreateLambda([this] { Display.bVSync = !Display.bVSync; })));
		Row(Rows, LOCTEXT("Preset", "그래픽 품질"),
			Button(TAttribute<FText>::CreateLambda([this] {
				const FText Quality[] = {LOCTEXT("Low", "낮음   ›"), LOCTEXT("Medium", "중간   ›"), LOCTEXT("High", "높음   ›")};
				if (Display.Quality == CompetitiveQuality) return LOCTEXT("Competitive", "경쟁   ›");
				return Display.Quality >= 0 && Display.Quality <= 2 ? Quality[Display.Quality] : LOCTEXT("Custom", "사용자 지정   ›");
			}), FSimpleDelegate::CreateLambda([this] {
				Display.Quality = Display.Quality >= 0 && Display.Quality < 2 ? Display.Quality + 1 :
					Display.Quality == 2 ? CompetitiveQuality : Display.Quality == CompetitiveQuality ? -1 : 0;
			})));
		Row(Rows, LOCTEXT("FrameLimit", "프레임 제한"),
			Button(TAttribute<FText>::CreateLambda([this] {
				return Display.FrameLimit > 0.f
					? FText::Format(LOCTEXT("FPS", "{0} FPS   ›"), FText::AsNumber(FMath::RoundToInt(Display.FrameLimit)))
					: LOCTEXT("Unlimited", "무제한   ›");
			}), FSimpleDelegate::CreateLambda([this] {
				int32 Next = 0;
				for (int32 Index = 0; Index < UE_ARRAY_COUNT(FrameLimits); ++Index)
				{
					if (FMath::IsNearlyEqual(Display.FrameLimit, FrameLimits[Index]))
					{
						Next = (Index + 1) % UE_ARRAY_COUNT(FrameLimits);
						break;
					}
				}
				Display.FrameLimit = FrameLimits[Next];
			})));
		Row(Rows, LOCTEXT("CompetitiveTarget", "144 FPS 목표"),
			Button(LOCTEXT("UseCompetitive", "경쟁 설정 선택"),
				FSimpleDelegate::CreateLambda([this] {
					Display.Quality = CompetitiveQuality;
					Display.FrameLimit = 144.f;
					Display.bVSync = false;
				})));
		Rows.AddSlot().AutoHeight().Padding(0, 10)
		[ Text(LOCTEXT("DisplayHint", "경쟁 설정은 내부 해상도 85%와 낮은 조명·반사 비용을 사용합니다."), 13, Muted) ];
	}

	static FText ToggleText(bool Enabled)
	{
		return Enabled ? LOCTEXT("On", "켜짐") : LOCTEXT("Off", "꺼짐");
	}

	void BuildAudio(SVerticalBox& Rows)
	{
		const FText Labels[] = {LOCTEXT("Master", "전체 음량"), LOCTEXT("Music", "음악 음량"), LOCTEXT("Effects", "효과음 음량"), LOCTEXT("UIVolume", "UI 음량")};
		for (int32 Index = 0; Index < UE_ARRAY_COUNT(Labels); ++Index)
		{
			Row(Rows, Labels[Index],
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().FillWidth(1).VAlign(VAlign_Center)
				[ SNew(SSlider).Value_Lambda([this, Index] { return Local.Volume[Index]; }).SliderBarColor(Gold)
					.OnValueChanged_Lambda([this, Index](float Value) { Local.Volume[Index] = Value; }) ]
				+ SHorizontalBox::Slot().AutoWidth().Padding(12, 0).VAlign(VAlign_Center)
				[ Text(TAttribute<FText>::CreateLambda([this, Index] { return FText::AsPercent(Local.Volume[Index]); })) ]
				+ SHorizontalBox::Slot().AutoWidth()
				[ Button(TAttribute<FText>::CreateLambda([this, Index] {
					return Local.Muted[Index] ? LOCTEXT("Muted", "음소거 해제") : LOCTEXT("Mute", "음소거");
				}), FSimpleDelegate::CreateLambda([this, Index] { Local.Muted[Index] = !Local.Muted[Index]; })) ]);
		}
		Rows.AddSlot().AutoHeight().Padding(0, 10)
		[ Text(LOCTEXT("AudioHint", "값은 저장됩니다. 사운드 출력과 미리 듣기는 아직 연결되지 않았습니다."), 13, Muted) ];
	}

	void BuildControls(SVerticalBox& Rows)
	{
		Row(Rows, LOCTEXT("ADSMode", "조준 방식 · 우클릭"),
			Button(TAttribute<FText>::CreateLambda([this] { return Local.bToggleADS ? LOCTEXT("ADSToggle", "토글") : LOCTEXT("ADSHold", "누르는 동안"); }),
				FSimpleDelegate::CreateLambda([this] { Local.bToggleADS = !Local.bToggleADS; })));
		Row(Rows, LOCTEXT("ADSSensitivity", "조준 감도 배율"),
			SNew(SSlider).MinValue(0.1f).MaxValue(1.5f).SliderBarColor(Gold)
			.Value_Lambda([this] { return Local.ADSSensitivity; })
			.OnValueChanged_Lambda([this](float Value) { Local.ADSSensitivity = Value; }));
		Row(Rows, LOCTEXT("Sensitivity", "마우스 감도"),
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().FillWidth(1).VAlign(VAlign_Center)
			[ SNew(SSlider).MinValue(0.1f).MaxValue(3.0f).SliderBarColor(Gold)
				.Value_Lambda([this] { return Local.Sensitivity; })
				.OnValueChanged_Lambda([this](float Value) { Local.Sensitivity = Value; }) ]
			+ SHorizontalBox::Slot().AutoWidth().Padding(12, 0).VAlign(VAlign_Center)
			[ Text(TAttribute<FText>::CreateLambda([this] { return FText::FromString(FString::Printf(TEXT("%.2f"), Local.Sensitivity)); })) ]);
		Row(Rows, LOCTEXT("Invert", "수직 감도 반전"),
			Button(TAttribute<FText>::CreateLambda([this] { return ToggleText(Local.bInvertY); }),
				FSimpleDelegate::CreateLambda([this] { Local.bInvertY = !Local.bInvertY; })));
		Rows.AddSlot().AutoHeight().Padding(0, 10, 0, 4) [ Text(LOCTEXT("KeyList", "현재 키 설정 · 읽기 전용"), 15, Gold) ];
		const auto* Context = LoadObject<UInputMappingContext>(nullptr, TEXT("/Game/Input/IMC_FMPlayer.IMC_FMPlayer"));
		if (Context)
		{
			const FName Actions[] = {TEXT("IA_FMMove"), TEXT("IA_FMLook"), TEXT("IA_FMJump"), TEXT("IA_FMSprint"), TEXT("IA_FMCrouch")};
			const FText Labels[] = {LOCTEXT("Move", "이동"), LOCTEXT("Look", "시점"), LOCTEXT("Jump", "점프"), LOCTEXT("Sprint", "달리기"), LOCTEXT("Crouch", "앉기")};
			for (int32 Index = 0; Index < UE_ARRAY_COUNT(Actions); ++Index)
			{
				TArray<FString> Keys;
				for (const auto& Mapping : Context->GetMappings())
				{
					if (Mapping.Action && Mapping.Action->GetFName() == Actions[Index])
					{
						Keys.AddUnique(Mapping.Key.GetDisplayName().ToString());
					}
				}
				Row(Rows, Labels[Index], Text(FText::FromString(FString::Join(Keys, TEXT("  /  "))), 14, Muted));
			}
		}
	}

	void Discard()
	{
		Display = SavedDisplay;
		Local = SavedLocal;
	}

	void Defaults()
	{
		Local = FMLocalPreferences{};
		Display.Resolution = Settings->GetDesktopResolution();
		Display.Mode = EWindowMode::WindowedFullscreen;
		Display.bVSync = false;
		Display.Quality = 2;
		Display.FrameLimit = 0.f;
		Resolutions.AddUnique(Display.Resolution);
	}

	void Apply()
	{
		if (Display.Resolution != SavedDisplay.Resolution || Display.Mode != SavedDisplay.Mode)
		{
			Settings->SetScreenResolution(Display.Resolution);
			Settings->SetFullscreenMode(Display.Mode);
			Settings->ApplyResolutionSettings(false);
			UE_LOG(LogTemp, Display, TEXT("FIELDMAKER settings: video preview"));
			ShowDialog(EDialog::Video);
			return;
		}
		Commit();
	}

	void Commit()
	{
		Settings->SetVSyncEnabled(Display.bVSync);
		Settings->SetFrameRateLimit(Display.FrameLimit);
		if (Display.Quality == CompetitiveQuality)
		{
			Settings->SetOverallScalabilityLevel(1);
			Settings->SetViewDistanceQuality(2);
			Settings->SetAntiAliasingQuality(2);
			Settings->SetTextureQuality(2);
			Settings->SetGlobalIlluminationQuality(0);
			Settings->SetReflectionQuality(0);
			Settings->SetPostProcessingQuality(0);
			Settings->SetResolutionScaleValueEx(CompetitiveResolutionScale);
		}
		else if (Display.Quality >= 0)
		{
			Settings->SetOverallScalabilityLevel(Display.Quality);
		}
		Settings->ApplyNonResolutionSettings();
		Settings->SaveSettings();
		Local.Save();
		SavedLocal = Local;
		SavedDisplay = Display;
		UE_LOG(LogTemp, Display, TEXT("FIELDMAKER settings: applied"));
	}

	void RollbackVideo()
	{
		if (Dialog == EDialog::Video)
		{
			Settings->SetScreenResolution(SavedDisplay.Resolution);
			Settings->SetFullscreenMode(SavedDisplay.Mode);
			Settings->ApplyResolutionSettings(false);
			Display.Resolution = SavedDisplay.Resolution;
			Display.Mode = SavedDisplay.Mode;
			Dialog = EDialog::None;
			UE_LOG(LogTemp, Display, TEXT("FIELDMAKER settings: video reverted"));
		}
	}

	void DismissDialog()
	{
		RollbackVideo();
		Dialog = EDialog::None;
		Modal->SetVisibility(EVisibility::Collapsed);
		Content->SetEnabled(true);
	}

	void ConfirmDialog()
	{
		const EDialog Current = Dialog;
		Dialog = EDialog::None;
		DismissDialog();
		if (Current == EDialog::Video)
		{
			Settings->ConfirmVideoMode();
			Commit();
		}
		else if (Current == EDialog::Reset)
		{
			Defaults();
			Apply();
		}
	}

	void ShowDialog(EDialog Kind)
	{
		Dialog = Kind;
		const bool bVideo = Kind == EDialog::Video;
		Deadline = FPlatformTime::Seconds() + 15.0;
		Content->SetEnabled(false);
		Modal->SetVisibility(EVisibility::Visible);
		Modal->SetContent(
			SNew(SOverlay)
			+ SOverlay::Slot()
			[ SNew(SBorder).BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush")).BorderBackgroundColor(FLinearColor(0, 0, 0, 0.78f)) ]
			+ SOverlay::Slot().HAlign(HAlign_Center).VAlign(VAlign_Center)
			[
				SNew(SBorder).BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush")).BorderBackgroundColor(Ink).Padding(24)
				[
					SNew(SVerticalBox)
					+ SVerticalBox::Slot().AutoHeight().Padding(0, 0, 0, 10)
					[ Text(bVideo ? LOCTEXT("VideoTitle", "화면 변경 확인") : LOCTEXT("Reset", "설정 초기화"), 23, Gold) ]
					+ SVerticalBox::Slot().AutoHeight().Padding(0, 0, 0, 14)
					[ Text(TAttribute<FText>::CreateLambda([this, bVideo] {
						return bVideo
							? FText::Format(LOCTEXT("Countdown", "{0}초 안에 확인하지 않으면 이전 화면으로 돌아갑니다."),
								FMath::Max(0, FMath::CeilToInt(Deadline - FPlatformTime::Seconds())))
							: LOCTEXT("ResetPrompt", "모든 로컬 옵션을 기본값으로 초기화할까요?");
					})) ]
					+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Right)
					[
						SNew(SHorizontalBox)
						+ SHorizontalBox::Slot().AutoWidth().Padding(4)
						[ Button(LOCTEXT("Cancel", "취소"), FSimpleDelegate::CreateSP(this, &SFMSettingsPanel::DismissDialog)) ]
						+ SHorizontalBox::Slot().AutoWidth().Padding(4)
						[ Button(LOCTEXT("Confirm", "확인"), FSimpleDelegate::CreateSP(this, &SFMSettingsPanel::ConfirmDialog), Gold) ]
					]
				]
			]);
		if (bVideo)
		{
			RegisterActiveTimer(0.1f, FWidgetActiveTimerDelegate::CreateSP(this, &SFMSettingsPanel::TickConfirmation));
		}
	}

	EActiveTimerReturnType TickConfirmation(double CurrentTime, float DeltaTime)
	{
		if (Dialog != EDialog::Video)
		{
			return EActiveTimerReturnType::Stop;
		}
		if (FPlatformTime::Seconds() >= Deadline)
		{
			DismissDialog();
			return EActiveTimerReturnType::Stop;
		}
		return EActiveTimerReturnType::Continue;
	}
};
}

TSharedRef<SWidget> UFMSettingsWidget::RebuildWidget()
{
	return SNew(SFMSettingsPanel).ButtonStyle(ButtonStyle).MenuFont(MenuFont).OnClosed(OnClosed);
}

#undef LOCTEXT_NAMESPACE
