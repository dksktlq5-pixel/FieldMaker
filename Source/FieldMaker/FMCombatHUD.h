#pragma once

#include "GameFramework/HUD.h"
#include "FMCombatHUD.generated.h"

UCLASS()
class AFMCombatHUD : public AHUD
{
	GENERATED_BODY()

public:
	virtual void DrawHUD() override;
};
