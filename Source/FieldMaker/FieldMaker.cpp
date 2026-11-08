// Copyright Epic Games, Inc. All Rights Reserved.

#include "FieldMaker.h"
#include "Modules/ModuleManager.h"
#include "Engine/Engine.h"
#include "MoviePlayer.h"
#include "Styling/CoreStyle.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Text/STextBlock.h"

class FFieldMakerModule : public FDefaultGameModuleImpl
{
public:
	virtual void StartupModule() override
	{
		FCoreUObjectDelegates::PreLoadMapWithContext.AddRaw(this, &FFieldMakerModule::ShowLoadingScreen);
	}

	virtual void ShutdownModule() override
	{
		FCoreUObjectDelegates::PreLoadMapWithContext.RemoveAll(this);
	}

private:
	void ShowLoadingScreen(const FWorldContext& Context, const FString& Map)
	{
		if (Context.WorldType != EWorldType::Game || !IsMoviePlayerEnabled() || IsRunningDedicatedServer() ||
			!Map.Contains(TEXT("L_FMBridgeBattlefield"))) return;

		FLoadingScreenAttributes Loading;
		Loading.bAutoCompleteWhenLoadingCompletes = true;
		Loading.MinimumLoadingScreenDisplayTime = 0.75f;
		Loading.WidgetLoadingScreen =
			SNew(SBorder).BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush"))
			.BorderBackgroundColor(FLinearColor(0.012f, 0.017f, 0.022f))
			.HAlign(HAlign_Center).VAlign(VAlign_Center)
			[SNew(STextBlock).Font(FCoreStyle::GetDefaultFontStyle("Bold", 30))
				.Text(NSLOCTEXT("FieldMaker", "LoadingBattlefield", "FIELDMAKER  /  LOADING BATTLEFIELD"))];
		GetMoviePlayer()->SetupLoadingScreen(Loading);
	}
};

IMPLEMENT_PRIMARY_GAME_MODULE(FFieldMakerModule, FieldMaker, "FieldMaker");
