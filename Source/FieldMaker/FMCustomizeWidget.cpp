#include "FMCustomizeWidget.h"

#include "FMCharacter.h"
#include "FMAppearanceSaveGame.h"
#include "Components/PointLightComponent.h"
#include "Components/SceneCaptureComponent2D.h"
#include "Engine/PointLight.h"
#include "Engine/SceneCapture2D.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Engine/World.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Styling/CoreStyle.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SScaleBox.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/SOverlay.h"
#include "Widgets/Text/STextBlock.h"

#define LOCTEXT_NAMESPACE "FMCustomize"

namespace
{
const FLinearColor Gold(0.70f, 0.47f, 0.15f);
const FLinearColor Ink(0.021f, 0.024f, 0.026f);
const FLinearColor White(0.93f, 0.92f, 0.88f);

struct FChoice
{
	FText Category;
	FText Item;
	uint8 Mask;
};

const TArray<FChoice>& GetChoices()
{
	static const TArray<FChoice> Items = {
		{LOCTEXT("Cap", "모자"), LOCTEXT("CapItem", "베이지 전술 모자"), FMAppearance::Cap},
		{LOCTEXT("Head", "머리 · 얼굴"), LOCTEXT("HeadItem", "Quantum 기본 얼굴"), 0},
		{LOCTEXT("Shirt", "상의"), LOCTEXT("ShirtItem", "파란 롤업 셔츠"), 0},
		{LOCTEXT("Vest", "방탄복"), LOCTEXT("VestItem", "베이지 방탄복"), FMAppearance::Vest},
		{LOCTEXT("Drops", "전술 장비"), LOCTEXT("DropsItem", "전술 파우치"), FMAppearance::Drops},
		{LOCTEXT("Holster", "홀스터"), LOCTEXT("HolsterItem", "베이지 하드 홀스터"), FMAppearance::Holster},
		{LOCTEXT("Jeans", "바지"), LOCTEXT("JeansItem", "베이지 청바지"), 0},
		{LOCTEXT("Patch", "패치"), LOCTEXT("PatchItem", "후면 패치"), FMAppearance::Patch}
	};
	return Items;
}
}

void UFMCustomizeWidget::InitializePreview(float ExposureEV, bool bHideEnvironment)
{
	DraftAppearance = UFMAppearanceSaveGame::Load();
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	const FVector Origin(0, 0, 20000);
	FActorSpawnParameters Spawn;
	Spawn.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	Spawn.ObjectFlags |= RF_Transient;
	Preview = World->SpawnActor<AFMCharacter>(Origin + FVector(0, 0, 88), FRotator::ZeroRotator, Spawn);
	Capture = World->SpawnActor<ASceneCapture2D>(Origin + FVector(315, 20, 103), FRotator(-2, 184, 0), Spawn);
	if (!Preview || !Capture)
	{
		Status = LOCTEXT("Unavailable", "캐릭터 미리보기를 열 수 없습니다.");
		return;
	}

	Preview->SetReplicates(false);
	Preview->SetActorEnableCollision(false);
	Preview->GetCharacterMovement()->DisableMovement();
	Preview->ApplyAppearance(DraftAppearance);
	const auto MakeLight = [&](FVector Offset, FLinearColor Color, float Intensity)
	{
		auto* Light = World->SpawnActor<APointLight>(Origin + Offset, FRotator::ZeroRotator, Spawn);
		if (Light)
		{
			auto* Point = Light->FindComponentByClass<UPointLightComponent>();
			Point->SetMobility(EComponentMobility::Movable);
			Point->SetAttenuationRadius(550);
			Point->SetIntensity(Intensity);
			Point->SetLightColor(Color);
			Point->SetCastShadows(false);
		}
		return Light;
	};
	KeyLight = MakeLight(FVector(130, 75, 195), FLinearColor(1, 0.90f, 0.76f), 16000);
	FillLight = MakeLight(FVector(55, -105, 128), FLinearColor(0.66f, 0.79f, 1), 9500);

	PreviewTarget = NewObject<UTextureRenderTarget2D>(this);
	PreviewTarget->ClearColor = FLinearColor::Black;
	PreviewTarget->InitAutoFormat(480, 560);
	auto* Scene = Capture->GetCaptureComponent2D();
	Scene->TextureTarget = PreviewTarget;
	Scene->CaptureSource = SCS_FinalColorLDR;
	Scene->FOVAngle = 38;
	Scene->PostProcessSettings.bOverride_AutoExposureMinBrightness = true;
	Scene->PostProcessSettings.bOverride_AutoExposureMaxBrightness = true;
	Scene->PostProcessSettings.AutoExposureMinBrightness = ExposureEV;
	Scene->PostProcessSettings.AutoExposureMaxBrightness = ExposureEV;
 if(bHideEnvironment){Scene->ShowFlags.SetAtmosphere(false);Scene->ShowFlags.SetFog(false);}
	Scene->PrimitiveRenderMode = ESceneCapturePrimitiveRenderMode::PRM_UseShowOnlyList;
	Scene->ShowOnlyActorComponents(Preview);
	PreviewBrush.SetResourceObject(PreviewTarget);
	PreviewBrush.SetImageSize(FVector2D(340, 396));
}

TSharedRef<SWidget> UFMCustomizeWidget::RebuildWidget()
{
	const auto Text = [this](TAttribute<FText> Label, int32 Size, FLinearColor Color = White)
	{
		FSlateFontInfo Font = MenuFont;
		Font.Size = Size;
		return SNew(STextBlock).Text(Label).Font(Font).ColorAndOpacity(FSlateColor(Color));
	};
	const auto Button = [this, &Text](TAttribute<FText> Label, TFunction<void()> Action)
	{
		return SNew(SButton).ButtonStyle(&ButtonStyle).Cursor(EMouseCursor::Hand)
			.OnClicked_Lambda([Action = MoveTemp(Action)] { Action(); return FReply::Handled(); })
			[ Text(Label, 15) ];
	};
	auto Categories = SNew(SVerticalBox);
	const auto& Choices = GetChoices();
	for (int32 Index = 0; Index < Choices.Num(); ++Index)
	{
		Categories->AddSlot().AutoHeight().Padding(0, 2)
		[ Button(Choices[Index].Category, [this, Index] { Category = Index; }) ];
	}
	auto ApplyPreview = [this]
	{
		if (Preview)
		{
			Preview->ApplyAppearance(DraftAppearance);
		}
	};
	return SNew(SOverlay)
		+ SOverlay::Slot()
		[ SNew(SBorder).BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush")).BorderBackgroundColor(FLinearColor(0, 0, 0, 0.75f)) ]
		+ SOverlay::Slot().HAlign(HAlign_Center).VAlign(VAlign_Center)
		[
			SNew(SScaleBox).Stretch(EStretch::ScaleToFit).StretchDirection(EStretchDirection::DownOnly)
			[
				SNew(SBox).WidthOverride(900).HeightOverride(580)
				[
					SNew(SBorder).BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush")).BorderBackgroundColor(Gold).Padding(1)
					[
						SNew(SBorder).BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush")).BorderBackgroundColor(Ink).Padding(24)
						[
							SNew(SVerticalBox)
							+ SVerticalBox::Slot().AutoHeight().Padding(0, 0, 0, 14) [ Text(LOCTEXT("Title", "FIELDMAKER  /  커스터마이즈"), 28, Gold) ]
							+ SVerticalBox::Slot().FillHeight(1)
							[
								SNew(SHorizontalBox)
								+ SHorizontalBox::Slot().AutoWidth()
								[ SNew(SBox).WidthOverride(138) [ Categories ] ]
								+ SHorizontalBox::Slot().FillWidth(1).Padding(14, 0).VAlign(VAlign_Center)
								[
									SNew(SVerticalBox)
									+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center)
									[ SNew(SImage).Image(&PreviewBrush) ]
									+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(0, 8, 0, 0)
									[ Button(LOCTEXT("Rotate", "회전"), [this] {
										if (Preview)
										{
											Preview->AddActorWorldRotation(FRotator(0.f, 45.f, 0.f));
										}
									}) ]
								]
								+ SHorizontalBox::Slot().AutoWidth().Padding(4, 0)
								[
									SNew(SBox).WidthOverride(238)
									[
										SNew(SVerticalBox)
										+ SVerticalBox::Slot().AutoHeight().Padding(0, 0, 0, 10)
										[ Text(TAttribute<FText>::CreateLambda([this] { return GetChoices()[Category].Category; }), 22, Gold) ]
										+ SVerticalBox::Slot().AutoHeight().Padding(0, 0, 0, 10)
										[ Text(TAttribute<FText>::CreateLambda([this] { return GetChoices()[Category].Item; }), 16) ]
										+ SVerticalBox::Slot().AutoHeight()
										[
											Button(TAttribute<FText>::CreateLambda([this] {
												const uint8 Mask = GetChoices()[Category].Mask;
												return Mask ? (DraftAppearance & Mask ? LOCTEXT("Remove", "벗기") : LOCTEXT("Equip", "착용")) : LOCTEXT("Fixed", "기본 파츠");
											}), [this, ApplyPreview] {
												const uint8 Mask = GetChoices()[Category].Mask;
												if (!Mask)
												{
													return;
												}
												Status = FText::GetEmpty();
												DraftAppearance ^= Mask;
												if (!(DraftAppearance & FMAppearance::Vest))
												{
													DraftAppearance = Mask == FMAppearance::Patch && (DraftAppearance & Mask)
														? DraftAppearance | FMAppearance::Vest : DraftAppearance & ~FMAppearance::Patch;
												}
												ApplyPreview();
											})
										]
										+ SVerticalBox::Slot().AutoHeight().Padding(0, 14, 0, 0)
										[ Text(TAttribute<FText>::CreateLambda([this] { return Status; }), 14, Gold) ]
									]
								]
							]
							+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Right).Padding(0, 12, 0, 0)
							[
								SNew(SHorizontalBox)
								+ SHorizontalBox::Slot().AutoWidth().Padding(4)
								[ Button(LOCTEXT("Apply", "적용"), [this] {
									if (UFMAppearanceSaveGame::Save(DraftAppearance))
									{
										Status = LOCTEXT("Saved", "외형을 저장했습니다.");
									}
									else
									{
										Status = LOCTEXT("SaveFailed", "외형을 저장할 수 없습니다.");
									}
								}) ]
								+ SHorizontalBox::Slot().AutoWidth().Padding(4)
								[ Button(LOCTEXT("Reset", "기본 외형"), [this, ApplyPreview] { DraftAppearance = FMAppearance::Default; Status = FText::GetEmpty(); ApplyPreview(); }) ]
								+ SHorizontalBox::Slot().AutoWidth().Padding(4)
								[ Button(LOCTEXT("Cancel", "취소"), [this] { OnClosed.ExecuteIfBound(); }) ]
							]
						]
					]
				]
			]
		];
}

void UFMCustomizeWidget::CleanupPreview()
{
	for (AActor* Actor : {static_cast<AActor*>(Capture.Get()), static_cast<AActor*>(Preview.Get()),
		static_cast<AActor*>(KeyLight.Get()), static_cast<AActor*>(FillLight.Get())})
	{
		if (IsValid(Actor))
		{
			Actor->Destroy();
		}
	}
	Capture=nullptr; Preview=nullptr; KeyLight=nullptr; FillLight=nullptr;
}
void UFMCustomizeWidget::NativeDestruct()
{
 CleanupPreview();
 Super::NativeDestruct();
}

#undef LOCTEXT_NAMESPACE
