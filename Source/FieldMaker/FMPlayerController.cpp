#include "FMPlayerController.h"
#include "FMPlayerState.h"
#include "EnhancedInputSubsystems.h"
#include "Engine/LocalPlayer.h"
#include "InputMappingContext.h"
#include "Components/InputComponent.h"
#include "FMUserPreferences.h"
#include "FMShopWidget.h"

void AFMPlayerController::BeginPlay()
{
	Super::BeginPlay();
	if (IsLocalController())
	{
		SetInputMode(FInputModeGameOnly());
		bShowMouseCursor = false;
	}
}

void AFMPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	ULocalPlayer* LocalPlayer = GetLocalPlayer();
	if (!LocalPlayer)
	{
		return;
	}

	InputComponent->BindKey(EKeys::One, IE_Pressed, this, &AFMPlayerController::EquipWeapon<EFMWeaponSlot::Primary>);
	InputComponent->BindKey(EKeys::Two, IE_Pressed, this, &AFMPlayerController::EquipWeapon<EFMWeaponSlot::Secondary>);
	InputComponent->BindKey(EKeys::Three, IE_Pressed, this, &AFMPlayerController::EquipWeapon<EFMWeaponSlot::Melee>);
	InputComponent->BindKey(EKeys::Four, IE_Pressed, this, &AFMPlayerController::EquipWeapon<EFMWeaponSlot::Throwable>);
	InputComponent->BindKey(EKeys::LeftMouseButton, IE_Pressed, this, &AFMPlayerController::UseWeapon);
	InputComponent->BindKey(EKeys::LeftMouseButton, IE_Released, this, &AFMPlayerController::StopUsingWeapon);
	InputComponent->BindKey(EKeys::R, IE_Pressed, this, &AFMPlayerController::ReloadWeapon);
	InputComponent->BindKey(EKeys::RightMouseButton, IE_Pressed, this, &AFMPlayerController::StartAiming);
	InputComponent->BindKey(EKeys::RightMouseButton, IE_Released, this, &AFMPlayerController::StopAiming);
	InputComponent->BindKey(EKeys::B, IE_Pressed, this, &AFMPlayerController::ToggleShop);
	InputComponent->BindKey(EKeys::V, IE_Pressed, this, &AFMPlayerController::ToggleTestArmor);

	auto* Input = LocalPlayer->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>();
	const auto* Context = LoadObject<UInputMappingContext>(nullptr, TEXT("/Game/Input/IMC_FMPlayer.IMC_FMPlayer"));
	if (Input && Context && !Input->HasMappingContext(Context))
	{
		Input->AddMappingContext(Context, 0);
	}
}

void AFMPlayerController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);
	if (IsLocalController())
	{
		ServerSetAppearance(UFMAppearanceSaveGame::Load());
	}
}

void AFMPlayerController::AcknowledgePossession(APawn* InPawn)
{
	Super::AcknowledgePossession(InPawn);
	if (IsLocalController() && !HasAuthority())
	{
		ServerSetAppearance(UFMAppearanceSaveGame::Load());
	}
}

void AFMPlayerController::ServerSetAppearance_Implementation(uint8 AppearanceId)
{
	if (auto* State = GetPlayerState<AFMPlayerState>())
	{
		State->SetAppearanceId(AppearanceId);
	}
}

template <EFMWeaponSlot Slot>
void AFMPlayerController::EquipWeapon()
{
	if (IsShopOpen()) return;
	if (auto* CombatPawn = GetPawn<AFMCharacter>())
	{
		CombatPawn->ServerEquipWeapon(Slot);
	}
}

void AFMPlayerController::UseWeapon()
{
	if (IsShopOpen()) return;
	if (auto* CombatPawn = GetPawn<AFMCharacter>())
	{
		CombatPawn->ServerStartFire();
	}
}

void AFMPlayerController::StopUsingWeapon()
{
	if (auto* CombatPawn = GetPawn<AFMCharacter>())
	{
		CombatPawn->ServerStopFire();
	}
}

void AFMPlayerController::ReloadWeapon()
{
	if (IsShopOpen()) return;
	if (auto* CombatPawn = GetPawn<AFMCharacter>())
	{
		CombatPawn->ServerReload();
	}
}

void AFMPlayerController::StartAiming()
{
	if (IsShopOpen()) return;
	if (auto* CombatPawn = GetPawn<AFMCharacter>())
		CombatPawn->ServerSetAiming(!FMLocalPreferences::Get().bToggleADS || !CombatPawn->IsAiming());
}

void AFMPlayerController::StopAiming()
{
	if (!FMLocalPreferences::Get().bToggleADS)
		if (auto* CombatPawn = GetPawn<AFMCharacter>()) CombatPawn->ServerSetAiming(false);
}

void AFMPlayerController::ToggleTestArmor()
{
	if (IsShopOpen()) return;
	if (auto* CombatPawn = GetPawn<AFMCharacter>()) CombatPawn->ServerToggleTestArmor();
}

void AFMPlayerController::ToggleShop()
{
 if (!IsLocalController()) return;
 if (ShopWidget) { CloseShop(); return; }
 ShopWidget = CreateWidget<UFMShopWidget>(this, UFMShopWidget::StaticClass());
 if (!ShopWidget) return;
 if (auto* P = GetPawn<AFMCharacter>())
 {
  P->ServerStopFire(); P->ServerSetAiming(false); P->ServerSetSprint(false);
 }
 StopMovement();
 SetIgnoreMoveInput(true); SetIgnoreLookInput(true);
 ShopWidget->AddToViewport(50);
 bShowMouseCursor = true;
 FInputModeUIOnly Mode;
 Mode.SetWidgetToFocus(ShopWidget->TakeWidget());
 Mode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
 SetInputMode(Mode);
 ShopWidget->SetKeyboardFocus();
}
void AFMPlayerController::CloseShop()
{
 if (!ShopWidget) return;
 ShopWidget->RemoveFromParent(); ShopWidget = nullptr;
 SetIgnoreMoveInput(false); SetIgnoreLookInput(false);
 bShowMouseCursor = false;
 SetInputMode(FInputModeGameOnly());
}
