#pragma once
#include "GameFramework/PlayerController.h"
#include "FMCharacter.h"
#include "FMPlayerController.generated.h"

class UFMShopWidget;

UCLASS()
class FIELDMAKER_API AFMPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category="Shop") void ToggleShop();
	UFUNCTION(BlueprintCallable, Category="Shop") void CloseShop();
	UFUNCTION(BlueprintPure, Category="Shop") UFMShopWidget* GetShopWidget() const { return ShopWidget; }
	UFUNCTION(BlueprintPure, Category="Shop") bool IsShopOpen() const { return ShopWidget != nullptr; }

protected:
	virtual void BeginPlay() override;
	virtual void SetupInputComponent() override;
	virtual void OnPossess(APawn* InPawn) override;
	virtual void AcknowledgePossession(APawn* InPawn) override;

private:
	UPROPERTY(Transient) TObjectPtr<class UFMShopWidget> ShopWidget;
	template <EFMWeaponSlot Slot> void EquipWeapon();
	void UseWeapon();
	void StopUsingWeapon();
	void ReloadWeapon();
	void StartAiming();
	void StopAiming();
	void ToggleTestArmor();
	UFUNCTION(Server, Reliable)
	void ServerSetAppearance(uint8 AppearanceId);
};
