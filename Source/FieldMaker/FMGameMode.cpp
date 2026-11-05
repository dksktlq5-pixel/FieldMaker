#include "FMGameMode.h"
#include "FMCharacter.h"
#include "FMPlayerController.h"

AFMGameMode::AFMGameMode()
{
	DefaultPawnClass = AFMCharacter::StaticClass();
	PlayerControllerClass = AFMPlayerController::StaticClass();
}
