#include "Misc/AutomationTest.h"
#if WITH_DEV_AUTOMATION_TESTS
#include "FMPlayerController.h"
#include "FMShopWidget.h"
#include "Blueprint/WidgetBlueprintLibrary.h"
#include "Engine/World.h"
#include "Framework/Application/SlateApplication.h"
#include "HAL/IConsoleManager.h"
#include "Input/Events.h"

namespace
{
// Run inside the existing PIE world. Editor Automation starts by tearing PIE down,
// so this verification deliberately does not register an EditorContext test.
void VerifyShopKeyboard(UWorld* World)
{
 auto* PC=World?Cast<AFMPlayerController>(World->GetFirstPlayerController()):nullptr;
 if(!PC || !PC->IsLocalController())
 {
  UE_LOG(LogTemp,Warning,TEXT("FMShopKeyboardVerification skipped: requires local PIE player"));return;
 }
 PC->CloseShop();
 const bool MoveWasIgnored=PC->IsMoveInputIgnored(),LookWasIgnored=PC->IsLookInputIgnored();
 int32 Passed=0,Failed=0;
 auto Check=[&](bool Value,const TCHAR* Message){if(Value)++Passed;else{++Failed;UE_LOG(LogTemp,Error,TEXT("FMShopKeyboardVerification: %s"),Message);}};
 for(const FKey Key:{EKeys::B,EKeys::Escape})
 {
  PC->ToggleShop();
  Check(PC->IsShopOpen() && PC->IsMoveInputIgnored() && PC->IsLookInputIgnored(),TEXT("opening locks movement and look"));
  UFMShopWidget* Shop=PC->GetShopWidget();
  if(!Shop){Check(false,TEXT("owned shop widget exists"));break;}
  FSlateApplication::Get().SetKeyboardFocus(Shop->TakeWidget(),EFocusCause::SetDirectly);
  FKeyEvent Event(Key,FModifierKeysState(),0,false,0,0);
  Check(FSlateApplication::Get().ProcessKeyDownEvent(Event),TEXT("close key consumed by widget"));
  Check(!PC->IsShopOpen(),TEXT("B / Escape closes shop"));
  Check(PC->IsMoveInputIgnored()==MoveWasIgnored,TEXT("move lock restored"));
  Check(PC->IsLookInputIgnored()==LookWasIgnored,TEXT("look lock restored"));
 }
 PC->CloseShop();
 UE_LOG(LogTemp,Display,TEXT("FMShopKeyboardVerification passed=%d failed=%d"),Passed,Failed);
}
FAutoConsoleCommandWithWorld ShopKeyboardCommand(TEXT("fm.Shop.VerifyKeyboard"),TEXT("Verify shop B / Escape handling inside an existing PIE session"),FConsoleCommandWithWorldDelegate::CreateStatic(&VerifyShopKeyboard));
}
#endif
