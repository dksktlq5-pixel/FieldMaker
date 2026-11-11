#include "FMCatalogWidget.h"
#include "FMCombatData.h"
#include "FMStructure.h"
#include "FMWeaponVisualData.h"
#include "Styling/CoreStyle.h"
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

void AddEntry(SVerticalBox& List, const FString& Name, const FString& Details)
{
	List.AddSlot().AutoHeight().Padding(0, 0, 0, 5)
	[
		SNew(SBorder).BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush"))
		.BorderBackgroundColor(FLinearColor(0.08f, 0.09f, 0.10f)).Padding(12)
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot().AutoHeight()[CatalogText(Name, 17)]
			+ SVerticalBox::Slot().AutoHeight().Padding(0, 5, 0, 0)[CatalogText(Details, 14)]
		]
	];
}
}

TSharedRef<SWidget> UFMCatalogWidget::RebuildWidget()
{
	SetIsFocusable(true);
	auto List = SNew(SVerticalBox);
	if (bStructures)
	{
		List->AddSlot().AutoHeight()[CatalogText(TEXT("구매 시간에 B로 구매  ·  구축 시간에 C  ·  1~4 선택  ·  R 회전  ·  좌클릭 배치"), 15)];
		const TCHAR* Uses[] = {TEXT("낮은 엄폐물"), TEXT("높은 엄폐물"), TEXT("높은 구조물에 오르는 발판"), TEXT("넓은 엄폐물과 적층용 받침")};
		for (int32 Index = 0; Index < AFMStructure::ItemCount; ++Index)
		{
			const auto* Item = AFMStructure::GetItem(Index);
			if (!Item) continue;
			AddHeading(*List, *FString::Printf(TEXT("%d  %s"), Index + 1, Item->Name));
			AddEntry(*List, Uses[Index], FString::Printf(TEXT("$%d   ·   내구도 %.0f   ·   %.2f × %.2f × %.2f m"),
				Item->Price, Item->Health, Item->HalfSize.X / 50, Item->HalfSize.Y / 50, Item->HalfSize.Z / 50));
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
					AddEntry(*List, Row->DisplayName.IsEmpty() ? Id.ToString() : Row->DisplayName.ToString(), Details);
				}
			}
		}
	}

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
					+ SVerticalBox::Slot().FillHeight(1)[SNew(SScrollBox) + SScrollBox::Slot()[List]]
				]
			]
		];
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
