#include "FMCatalogWidget.h"
#include "FMCombatData.h"
#include "FMStructure.h"
#include "FMWeaponVisualData.h"
#include "Components/PointLightComponent.h"
#include "Components/SceneCaptureComponent2D.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/PointLight.h"
#include "Engine/SceneCapture2D.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Widgets/Images/SImage.h"
#include "Styling/CoreStyle.h"
#include "TimerManager.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SScaleBox.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"

namespace
{
const FLinearColor Gold(0.95f, 0.72f, 0.35f);
const FLinearColor Paper(0.91f, 0.90f, 0.86f);

TSharedRef<STextBlock> CatalogText(const FString& Text, int32 Size, FLinearColor Color = Paper)
{
	return SNew(STextBlock).Text(FText::FromString(Text)).ColorAndOpacity(Color)
		.Font(FCoreStyle::GetDefaultFontStyle("Regular", Size)).AutoWrapText(true);
}

void AddHeading(SVerticalBox& List, const TCHAR* Name)
{
	List.AddSlot().AutoHeight().Padding(0, 16, 0, 6)[CatalogText(Name, 20, Gold)];
}

}

TSharedRef<SWidget> UFMCatalogWidget::RebuildWidget()
{
	SetIsFocusable(true);
	auto List = SNew(SVerticalBox);
	Entries.Reset();
	auto AddEntry = [this, &List](FString Name, FString Details, TSoftObjectPtr<UStaticMesh> Mesh)
	{
		const int32 Index = Entries.Add({MoveTemp(Name), MoveTemp(Details), Mesh});
		List->AddSlot().AutoHeight().Padding(0, 0, 0, 4)
		[
			SNew(SButton).ContentPadding(FMargin(12, 8))
			.ButtonColorAndOpacity_Lambda([this, Index]
			{
				return Selected == Index ? FLinearColor(0.54f, 0.38f, 0.18f) : FLinearColor(0.21f, 0.25f, 0.29f);
			})
			.OnClicked_Lambda([this, Index] { Select(Index); return FReply::Handled(); })
			[CatalogText(Entries[Index].Name, 16)]
		];
	};
	if (bStructures)
	{
		List->AddSlot().AutoHeight()[CatalogText(TEXT("구매 시간에 B로 구매  ·  구축 시간에 C  ·  1~4 선택  ·  R 회전  ·  좌클릭 배치"), 15)];
		const TCHAR* Uses[] = {TEXT("낮은 엄폐물"), TEXT("높은 엄폐물"), TEXT("높은 구조물에 오르는 발판"), TEXT("넓은 엄폐물과 적층용 받침")};
		for (int32 Index = 0; Index < AFMStructure::ItemCount; ++Index)
		{
			const auto* Item = AFMStructure::GetItem(Index);
			if (!Item) continue;
			AddEntry(FString::Printf(TEXT("%d  %s"), Index + 1, Item->Name), FString::Printf(TEXT("%s\n$%d   ·   내구도 %.0f   ·   %.2f × %.2f × %.2f m"), Uses[Index],
				Item->Price, Item->Health, Item->HalfSize.X / 50, Item->HalfSize.Y / 50, Item->HalfSize.Z / 50), GetDefault<AFMStructure>()->GetItemMesh(Index));
		}
	}
	else
	{
		const auto* Weapons = LoadObject<UDataTable>(nullptr, FMCatalog::Weapons);
		const auto* Visuals = LoadObject<UDataTable>(nullptr, FMCatalog::Visuals);
		if (!Weapons) List->AddSlot().AutoHeight()[CatalogText(TEXT("무기 데이터를 불러오지 못했습니다."), 16)];
		else
		{
			const TCHAR* Groups[] = {TEXT("라이플"), TEXT("권총"), TEXT("기관단총"), TEXT("저격소총"),
				TEXT("산탄총"), TEXT("기관총"), TEXT("근접무기"), TEXT("투척무기")};
			TArray<FName> Ids = Weapons->GetRowNames();
			Ids.Sort(FNameLexicalLess());
			for (int32 Group = 0; Group < UE_ARRAY_COUNT(Groups); ++Group)
			{
				bool bHeadingAdded = false;
				for (const FName Id : Ids)
				{
					const auto* Row = Weapons->FindRow<FFMWeaponRow>(Id, TEXT("Armory"), false);
					if (!Row || (!Row->bInCatalog && Id != TEXT("Knife")) || static_cast<int32>(Row->Category) != Group) continue;
					if (!bHeadingAdded)
					{
						AddHeading(*List, Groups[Group]);
						bHeadingAdded = true;
					}
					const auto* Visual = Visuals ? Visuals->FindRow<FFMWeaponVisualRow>(Id, TEXT("Armory"), false) : nullptr;
					const FString Price = Row->Category == EFMWeaponCategory::Melee ? TEXT("기본 지급")
						: FString::Printf(TEXT("$%d"), Row->Price);
					FString Details = Price;
					if (Row->Category == EFMWeaponCategory::Grenade) Details += TEXT("   ·   파편 수류탄");
					else
					{
						Details += FString::Printf(TEXT("   ·   기본 피해 %.0f"), Row->Damage);
						if (Row->PelletCount > 1) Details += FString::Printf(TEXT(" × %d발"), Row->PelletCount);
						if (Row->Category != EFMWeaponCategory::Melee)
							Details += FString::Printf(TEXT("   ·   탄약 %d / %d   ·   장전 %.2f초"), Row->MagazineSize, Row->ReserveAmmo, Row->ReloadTime);
					}
					if (Visual && !Visual->ModelLabel.IsEmpty()) Details += TEXT("\n") + Visual->ModelLabel.ToString();
					AddEntry(Row->DisplayName.IsEmpty() ? Id.ToString() : Row->DisplayName.ToString(), Details, Visual ? Visual->Mesh : nullptr);
				}
			}
		}
	}

	Selected = Entries.IsEmpty() ? INDEX_NONE : 0;
	PreviewBrush.SetImageSize(FVector2D(560, 315));

	return SNew(SBorder).BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush"))
		.BorderBackgroundColor(FLinearColor(0.015f, 0.019f, 0.025f, 0.96f)).Padding(24)
		[
			SNew(SScaleBox).Stretch(EStretch::ScaleToFit)
			[
				SNew(SBox).WidthOverride(950).HeightOverride(650)
				[
					SNew(SVerticalBox)
					+ SVerticalBox::Slot().AutoHeight().Padding(0, 0, 0, 12)
					[
						SNew(SHorizontalBox)
						+ SHorizontalBox::Slot().FillWidth(1)[CatalogText(bStructures ? TEXT("구조물 도감") : TEXT("무기고"), 31, Gold)]
						+ SHorizontalBox::Slot().AutoWidth()
						[
							SNew(SButton).ContentPadding(FMargin(14, 6)).OnClicked_Lambda([this]
							{
								OnClosed.ExecuteIfBound();
								return FReply::Handled();
							})[CatalogText(TEXT("닫기  /  ESC"), 15)]
						]
					]
					+ SVerticalBox::Slot().FillHeight(1)
					[
						SNew(SHorizontalBox)
						+ SHorizontalBox::Slot().AutoWidth()
						[SNew(SBox).WidthOverride(280)[SNew(SScrollBox) + SScrollBox::Slot()[List]]]
						+ SHorizontalBox::Slot().FillWidth(1).Padding(24, 0, 0, 0)
						[
							SNew(SVerticalBox)
							+ SVerticalBox::Slot().AutoHeight()
							[
								SNew(SBorder).BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush"))
								.BorderBackgroundColor(FLinearColor(0.06f, 0.08f, 0.10f)).Padding(12)
								[
									SNew(SBox).HeightOverride(315)
									[
										SNew(SOverlay)
										+ SOverlay::Slot()[SNew(SImage).Image(&PreviewBrush)
											.Visibility_Lambda([this] { return PreviewBrush.GetResourceObject() ? EVisibility::Visible : EVisibility::Collapsed; })]
										+ SOverlay::Slot().HAlign(HAlign_Center).VAlign(VAlign_Center)
										[SNew(STextBlock).Text(FText::FromString(TEXT("모델 미리보기 없음")))
											.Visibility_Lambda([this] { return PreviewBrush.GetResourceObject() ? EVisibility::Collapsed : EVisibility::Visible; })]
									]
								]
							]
							+ SVerticalBox::Slot().AutoHeight().Padding(0, 20, 0, 8)
							[SNew(STextBlock).Font(FCoreStyle::GetDefaultFontStyle("Bold", 27)).ColorAndOpacity(Gold)
								.Text_Lambda([this] { return Entries.IsValidIndex(Selected) ? FText::FromString(Entries[Selected].Name) : FText::GetEmpty(); })]
							+ SVerticalBox::Slot().AutoHeight()
							[SNew(STextBlock).AutoWrapText(true).Font(FCoreStyle::GetDefaultFontStyle("Regular", 16)).ColorAndOpacity(Paper)
								.Text_Lambda([this] { return Entries.IsValidIndex(Selected) ? FText::FromString(Entries[Selected].Details) : FText::GetEmpty(); })]
						]
					]
				]
			]
		];
}

void UFMCatalogWidget::Select(int32 Index)
{
	if (!Entries.IsValidIndex(Index) || !GetWorld()) return;
	Selected = Index;
	PreviewBrush.SetResourceObject(nullptr);
	UStaticMesh* Mesh = Entries[Index].Mesh.LoadSynchronous();
	if (!Mesh) return;

	const FVector Origin(0, 0, 20000);
	if (!Capture)
	{
		FActorSpawnParameters Params;
		Params.ObjectFlags |= RF_Transient;
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		Model = GetWorld()->SpawnActor<AStaticMeshActor>(Origin, FRotator::ZeroRotator, Params);
		Light = GetWorld()->SpawnActor<APointLight>(Origin + FVector(150, -220, 220), FRotator::ZeroRotator, Params);
		Capture = GetWorld()->SpawnActor<ASceneCapture2D>(Origin, FRotator::ZeroRotator, Params);
		if (!Model || !Capture) return;
		Model->SetActorEnableCollision(false);
		Model->GetStaticMeshComponent()->SetMobility(EComponentMobility::Movable);
		if (Light)
		{
			auto* Component = Light->FindComponentByClass<UPointLightComponent>();
			Component->SetMobility(EComponentMobility::Movable);
			Component->SetAttenuationRadius(900);
			Component->SetIntensity(42000);
			Component->SetCastShadows(false);
		}
		Target = NewObject<UTextureRenderTarget2D>(this);
		Target->InitAutoFormat(640, 360);
		auto* Scene = Capture->GetCaptureComponent2D();
		Scene->TextureTarget = Target;
		Scene->CaptureSource = SCS_FinalColorLDR;
		Scene->bCaptureEveryFrame = false;
		Scene->bCaptureOnMovement = false;
		Scene->PrimitiveRenderMode = ESceneCapturePrimitiveRenderMode::PRM_UseShowOnlyList;
		Scene->ShowOnlyActorComponents(Model);
		Scene->ShowFlags.SetAtmosphere(false);
		Scene->ShowFlags.SetFog(false);
		Scene->FOVAngle = 38;
		Scene->PostProcessSettings.bOverride_AutoExposureMinBrightness = true;
		Scene->PostProcessSettings.bOverride_AutoExposureMaxBrightness = true;
		Scene->PostProcessSettings.AutoExposureMinBrightness = 8.f;
		Scene->PostProcessSettings.AutoExposureMaxBrightness = 8.f;
	}
	if (!Model || !Target) return;
	const auto Bounds = Mesh->GetBounds();
	const double Scale = 105.0 / FMath::Max(1.0, Bounds.BoxExtent.GetMax());
	Model->GetStaticMeshComponent()->SetStaticMesh(Mesh);
	Model->SetActorScale3D(FVector(Scale));
	Model->SetActorLocation(Origin - Bounds.Origin * Scale);
	const FVector Camera = Origin + FVector(115, -365, 145);
	Capture->SetActorLocationAndRotation(Camera, (Origin - Camera).Rotation());
	Capture->GetCaptureComponent2D()->bCaptureEveryFrame = true;
	GetWorld()->GetTimerManager().SetTimer(PreviewCaptureTimer, FTimerDelegate::CreateWeakLambda(this, [this]
	{
		if (Capture) Capture->GetCaptureComponent2D()->bCaptureEveryFrame = false;
	}), 1.0f, false);
	PreviewBrush.SetResourceObject(Target);
}

void UFMCatalogWidget::NativeConstruct()
{
	Super::NativeConstruct();
	Select(Selected);
}

void UFMCatalogWidget::NativeDestruct()
{
	PreviewBrush.SetResourceObject(nullptr);
	if (GetWorld()) GetWorld()->GetTimerManager().ClearTimer(PreviewCaptureTimer);
	if (Capture) Capture->Destroy();
	if (Light) Light->Destroy();
	if (Model) Model->Destroy();
	Capture = nullptr;
	Light = nullptr;
	Model = nullptr;
	Target = nullptr;
	Super::NativeDestruct();
}

FReply UFMCatalogWidget::NativeOnKeyDown(const FGeometry& Geometry, const FKeyEvent& Event)
{
	if (Event.GetKey() == EKeys::Escape)
	{
		OnClosed.ExecuteIfBound();
		return FReply::Handled();
	}
	return Super::NativeOnKeyDown(Geometry, Event);
}
