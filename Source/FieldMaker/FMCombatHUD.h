#pragma once

#include "GameFramework/HUD.h"
#include "FMCombatHUD.generated.h"

UCLASS()
class AFMCombatHUD : public AHUD
{
	GENERATED_BODY()

public:
	AFMCombatHUD();
 virtual void DrawHUD() override;
private:
 UPROPERTY() TObjectPtr<class UTexture2D> ScopeMask;
};
