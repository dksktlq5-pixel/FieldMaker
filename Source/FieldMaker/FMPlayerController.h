#pragma once
#include "GameFramework/PlayerController.h"
#include "FMPlayerController.generated.h"

UCLASS()
class FIELDMAKER_API AFMPlayerController : public APlayerController
{
	GENERATED_BODY()

protected:
	virtual void SetupInputComponent() override;
};
