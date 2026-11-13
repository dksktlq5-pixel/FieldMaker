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
	UFUNCTION(BlueprintCallable, Category="Build") void ToggleBuildMode();
	bool IsBuildMode() const { return bBuildMode; }
	int32 GetBuildSelection() const { return BuildSelection; }
	int32 GetBuildRotation() const { return BuildQuarterTurns; }
	UFUNCTION(BlueprintCallable, Category="Shop") void CloseShop();
	UFUNCTION(BlueprintPure, Category="Shop") UFMShopWidget* GetShopWidget() const { return ShopWidget; }
	UFUNCTION(BlueprintPure, Category="Shop") bool IsShopOpen() const { return ShopWidget != nullptr; }

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void SetupInputComponent() override;
	virtual void OnPossess(APawn* InPawn) override;
	virtual void AcknowledgePossession(APawn* InPawn) override;
	virtual void PlayerTick(float DeltaTime) override;

private:
	bool CanBuild() const;
	void RotateStructureOrReload();
	bool bBuildMode = false;
	int32 BuildSelection = 0;
	int32 BuildQuarterTurns = 0;
	UPROPERTY(Transient)
	TObjectPtr<class UFMLobbyWidget> RoomWidget;
	UFUNCTION()
	void StartRoomMatch();
	UFUNCTION()
	void LeaveRoom();
	UPROPERTY(Transient) TObjectPtr<class UFMShopWidget> ShopWidget;
	template <EFMWeaponSlot Slot> void EquipWeapon();
	void UseWeapon();
	void StopUsingWeapon();
	void ReloadWeapon();
	void StartAiming();
	void StopAiming();
	void ToggleTestArmor();
	void HandleNetworkFailure(UWorld* World, UNetDriver* Driver, ENetworkFailure::Type Failure, const FString& Error);
	bool bLeavingRoom = false;
	UFUNCTION(Server, Reliable)
	void ServerSetAppearance(uint32 AppearanceId);
};
