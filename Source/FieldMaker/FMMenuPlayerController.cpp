#include "FMMenuPlayerController.h"
#include "FMCustomizeWidget.h"
#include "FMSettingsWidget.h"

#include "Blueprint/UserWidget.h"
#include "Blueprint/WidgetTree.h"
#include "Brushes/SlateRoundedBoxBrush.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Kismet/GameplayStatics.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
const FLinearColor Gold(0.70f, 0.47f, 0.15f);
const FLinearColor LightText(0.91f, 0.89f, 0.83f);

void StyleMenuButton(UButton& Button)
{
	FButtonStyle Style = Button.GetStyle();
	Style.SetNormal(FSlateRoundedBoxBrush(FLinearColor(0.015f, 0.017f, 0.018f, 0.48f), 2.0f));
	Style.SetHovered(FSlateRoundedBoxBrush(FLinearColor(0.12f, 0.082f, 0.028f, 0.94f), 2.0f, Gold, 1.0f));
	Style.SetPressed(FSlateRoundedBoxBrush(FLinearColor(0.22f, 0.135f, 0.035f), 2.0f, Gold, 2.0f));
	Style.SetNormalPadding(FMargin(12.0f, 3.0f));
	Style.SetPressedPadding(FMargin(12.0f, 4.0f, 12.0f, 2.0f));
	Button.SetStyle(Style);
	Button.SetCursor(EMouseCursor::Hand);
}

UTextBlock* AddText(UWidgetTree& Tree, const FText& Text, FSlateFontInfo Font, int32 Size)
{
	auto* Block = Tree.ConstructWidget<UTextBlock>();
	Font.Size = Size;
	Block->SetFont(Font);
	Block->SetText(Text);
	Block->SetColorAndOpacity(FSlateColor(LightText));
	return Block;
}
}

AFMMenuPlayerController::AFMMenuPlayerController()
{
	static ConstructorHelpers::FClassFinder<UUserWidget> Main(
		TEXT("/Game/FieldMaker/UI/Frontend/WBP_MainMenu"));
	static ConstructorHelpers::FClassFinder<UUserWidget> Quit(
		TEXT("/Game/FieldMaker/UI/Frontend/WBP_QuitConfirm"));
	MenuClass = Main.Class;
	QuitClass = Quit.Class;
}

void AFMMenuPlayerController::BeginPlay()
{
	Super::BeginPlay();
	if (!IsLocalController() || !MenuClass)
	{
		return;
	}

	Menu = CreateWidget<UUserWidget>(this, MenuClass);
	if (!Menu)
	{
		return;
	}

	const auto Bind = [this](FName Name, void (AFMMenuPlayerController::*Handler)(), const TCHAR* FunctionName)
	{
		if (auto* Button = Cast<UButton>(Menu->GetWidgetFromName(Name)))
		{
			StyleMenuButton(*Button);
			Button->OnClicked.__Internal_AddDynamic(this, Handler, FName(FunctionName));
		}
	};

	Bind(TEXT("Btn_Play"), &AFMMenuPlayerController::Play, TEXT("Play"));
	Bind(TEXT("Btn_Armory"), &AFMMenuPlayerController::Armory, TEXT("Armory"));
	Bind(TEXT("Btn_Structure"), &AFMMenuPlayerController::Structure, TEXT("Structure"));
	Bind(TEXT("Btn_Training"), &AFMMenuPlayerController::Training, TEXT("Training"));
	Bind(TEXT("Btn_Customize"), &AFMMenuPlayerController::Customize, TEXT("Customize"));
	Bind(TEXT("Btn_Shop"), &AFMMenuPlayerController::Shop, TEXT("Shop"));
	Bind(TEXT("Btn_Settings"), &AFMMenuPlayerController::Settings, TEXT("Settings"));
	Bind(TEXT("Btn_Quit"), &AFMMenuPlayerController::ShowQuitDialog, TEXT("ShowQuitDialog"));

	Menu->AddToViewport();
	bShowMouseCursor = true;
	FocusMenu(Menu->GetWidgetFromName(TEXT("Btn_Play")));
}

void AFMMenuPlayerController::FocusMenu(UWidget* Widget)
{
	FInputModeUIOnly InputMode;
	InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	if (Widget)
	{
		InputMode.SetWidgetToFocus(Widget->TakeWidget());
	}
	SetInputMode(InputMode);
}

void AFMMenuPlayerController::LogMenuSelection(const TCHAR* Selection)
{
	UE_LOG(LogTemp, Display, TEXT("FIELDMAKER menu preview: %s"), Selection);
}

void AFMMenuPlayerController::Play() { UGameplayStatics::OpenLevel(this, TEXT("/Game/Map/L_FMMovementTest")); }
void AFMMenuPlayerController::Armory() { LogMenuSelection(TEXT("Armory")); }
void AFMMenuPlayerController::Structure() { LogMenuSelection(TEXT("Structure")); }
void AFMMenuPlayerController::Training() { Play(); }
void AFMMenuPlayerController::Customize()
{
	if (CustomizePanel)
	{
		return;
	}

	auto* Button = Cast<UButton>(Menu->GetWidgetFromName(TEXT("Btn_Customize")));
	auto* Label = Cast<UTextBlock>(Menu->GetWidgetFromName(TEXT("Text_Customize")));
	if (!Button || !Label)
	{
		return;
	}

	CustomizePanel = CreateWidget<UFMCustomizeWidget>(this, UFMCustomizeWidget::StaticClass());
	if (CustomizePanel)
	{
		CustomizePanel->ButtonStyle = Button->GetStyle();
		CustomizePanel->MenuFont = Label->GetFont();
		CustomizePanel->OnClosed.BindUObject(this, &AFMMenuPlayerController::CloseCustomize);
		CustomizePanel->InitializePreview();
		Menu->SetIsEnabled(false);
		CustomizePanel->AddToViewport(10);
		FocusMenu(CustomizePanel);
	}
}

void AFMMenuPlayerController::CloseCustomize()
{
	CustomizePanel->RemoveFromParent();
	CustomizePanel = nullptr;
	Menu->SetIsEnabled(true);
	FocusMenu(Menu->GetWidgetFromName(TEXT("Btn_Customize")));
}
void AFMMenuPlayerController::Shop() { LogMenuSelection(TEXT("Shop")); }
void AFMMenuPlayerController::Settings()
{
	if (SettingsPanel)
	{
		return;
	}

	auto* Button = Cast<UButton>(Menu->GetWidgetFromName(TEXT("Btn_Settings")));
	auto* Label = Cast<UTextBlock>(Menu->GetWidgetFromName(TEXT("Text_Settings")));
	if (!Button || !Label)
	{
		return;
	}

	SettingsPanel = CreateWidget<UFMSettingsWidget>(this, UFMSettingsWidget::StaticClass());
	if (SettingsPanel)
	{
		SettingsPanel->ButtonStyle = Button->GetStyle();
		SettingsPanel->MenuFont = Label->GetFont();
		SettingsPanel->OnClosed.BindUObject(this, &AFMMenuPlayerController::CloseSettings);
		Menu->SetIsEnabled(false);
		SettingsPanel->AddToViewport(10);
		FocusMenu(SettingsPanel);
	}
}

void AFMMenuPlayerController::CloseSettings()
{
	SettingsPanel->RemoveFromParent();
	SettingsPanel = nullptr;
	Menu->SetIsEnabled(true);
	FocusMenu(Menu->GetWidgetFromName(TEXT("Btn_Settings")));
}

void AFMMenuPlayerController::ShowQuitDialog()
{
	if (QuitDialog || !QuitClass)
	{
		return;
	}

	QuitDialog = CreateWidget<UUserWidget>(this, QuitClass);
	if (!QuitDialog)
	{
		return;
	}

	UWidgetTree& Tree = *QuitDialog->WidgetTree;
	auto* Canvas = Tree.ConstructWidget<UCanvasPanel>();
	Tree.RootWidget = Canvas;

	auto* Scrim = Tree.ConstructWidget<UBorder>();
	Scrim->SetBrushColor(FLinearColor(0, 0, 0, 0.72f));
	auto* ScrimSlot = Canvas->AddChildToCanvas(Scrim);
	ScrimSlot->SetAnchors(FAnchors(0, 0, 1, 1));
	ScrimSlot->SetOffsets(FMargin(0));

	auto* Panel = Tree.ConstructWidget<UBorder>();
	Panel->SetBrush(FSlateRoundedBoxBrush(FLinearColor(0.022f, 0.024f, 0.025f), 3.0f, Gold, 1.0f));
	Panel->SetPadding(FMargin(28.0f));
	auto* PanelSlot = Canvas->AddChildToCanvas(Panel);
	PanelSlot->SetAnchors(FAnchors(0.5f));
	PanelSlot->SetAlignment(FVector2D(0.5f));
	PanelSlot->SetSize(FVector2D(400.0f, 180.0f));
	PanelSlot->SetPosition(FVector2D::ZeroVector);

	auto* Stack = Tree.ConstructWidget<UVerticalBox>();
	Panel->SetContent(Stack);
	const auto* MenuTitle = Cast<UTextBlock>(Menu->GetWidgetFromName(TEXT("Text_Quit")));
	const auto* MenuDescription = Cast<UTextBlock>(Menu->GetWidgetFromName(TEXT("Text_QuitDesc")));
	const FSlateFontInfo TitleFont = MenuTitle ? MenuTitle->GetFont() : FSlateFontInfo();
	const FSlateFontInfo BodyFont = MenuDescription ? MenuDescription->GetFont() : TitleFont;
	auto* Title = AddText(Tree, NSLOCTEXT("FMMenu", "QuitTitle", "게임 종료"), TitleFont, 25);
	Title->SetColorAndOpacity(FSlateColor(Gold));
	Stack->AddChildToVerticalBox(Title)->SetPadding(FMargin(0, 0, 0, 8));
	Stack->AddChildToVerticalBox(AddText(Tree, NSLOCTEXT("FMMenu", "QuitPrompt", "FIELDMAKER를 종료하시겠습니까?"), BodyFont, 16));
	auto* Actions = Tree.ConstructWidget<UHorizontalBox>();
	auto* ActionsSlot = Stack->AddChildToVerticalBox(Actions);
	ActionsSlot->SetHorizontalAlignment(HAlign_Right);
	ActionsSlot->SetPadding(FMargin(0, 14, 0, 0));

	auto* Cancel = Tree.ConstructWidget<UButton>(UButton::StaticClass(), TEXT("Btn_CancelQuit"));
	StyleMenuButton(*Cancel);
	Cancel->SetContent(AddText(Tree, NSLOCTEXT("FMMenu", "Cancel", "취소"), TitleFont, 16));
	Actions->AddChildToHorizontalBox(Cancel)->SetPadding(FMargin(0, 0, 8, 0));
	Cancel->OnClicked.AddDynamic(this, &AFMMenuPlayerController::CancelQuit);

	auto* Confirm = Tree.ConstructWidget<UButton>(UButton::StaticClass(), TEXT("Btn_ConfirmQuit"));
	StyleMenuButton(*Confirm);
	Confirm->SetContent(AddText(Tree, NSLOCTEXT("FMMenu", "Confirm", "종료"), TitleFont, 16));
	Actions->AddChildToHorizontalBox(Confirm);
	Confirm->OnClicked.AddDynamic(this, &AFMMenuPlayerController::ConfirmQuit);

	Menu->SetIsEnabled(false);
	QuitDialog->AddToViewport(10);
	FocusMenu(Cancel);
}

void AFMMenuPlayerController::CancelQuit()
{
	QuitDialog->RemoveFromParent();
	QuitDialog = nullptr;
	Menu->SetIsEnabled(true);
	FocusMenu(Menu->GetWidgetFromName(TEXT("Btn_Quit")));
}

void AFMMenuPlayerController::ConfirmQuit()
{
	UKismetSystemLibrary::QuitGame(this, this, EQuitPreference::Quit, false);
}
