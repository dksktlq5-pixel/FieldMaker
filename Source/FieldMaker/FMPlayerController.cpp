#include "FMPlayerController.h"
#include "EnhancedInputSubsystems.h"
#include "Engine/LocalPlayer.h"
#include "InputMappingContext.h"

void AFMPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	ULocalPlayer* LocalPlayer = GetLocalPlayer();
	if (!LocalPlayer)
	{
		return;
	}

	auto* Input = LocalPlayer->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>();
	const auto* Context = LoadObject<UInputMappingContext>(nullptr, TEXT("/Game/Input/IMC_FMPlayer.IMC_FMPlayer"));
	if (Input && Context && !Input->HasMappingContext(Context))
	{
		Input->AddMappingContext(Context, 0);
	}
}
