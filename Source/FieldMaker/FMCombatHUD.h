#pragma once

#include "GameFramework/HUD.h"
#include "Math/Box2D.h"
#include "FMCombatHUD.generated.h"

UCLASS()
class AFMCombatHUD : public AHUD
{
	GENERATED_BODY()

public:
	AFMCombatHUD();
 virtual void BeginPlay() override;
 virtual void DrawHUD() override;
private:
 UPROPERTY() TObjectPtr<class UTexture2D> ScopeMask;
 UPROPERTY() TArray<TObjectPtr<class UTexture2D>> WeaponIcons;
 UPROPERTY() TMap<FName, TObjectPtr<class UTexture2D>> ItemIcons;
 TArray<FBox> MinimapFootprints;
 FBox MinimapWorldBounds = FBox(ForceInit);
};
