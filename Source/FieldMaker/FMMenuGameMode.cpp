#include "FMMenuGameMode.h"
#include "FMMenuPlayerController.h"

AFMMenuGameMode::AFMMenuGameMode()
{
	PlayerControllerClass = AFMMenuPlayerController::StaticClass();
	DefaultPawnClass = nullptr;
}
