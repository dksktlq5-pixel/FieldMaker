#pragma once

#include "GameFramework/PlayerController.h"
#include "FMMenuPlayerController.generated.h"

class UUserWidget;

UCLASS()
class FIELDMAKER_API AFMMenuPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	AFMMenuPlayerController();

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	UPROPERTY()
	TSubclassOf<UUserWidget> MenuClass;

	UPROPERTY()
	TSubclassOf<UUserWidget> QuitClass;

	UPROPERTY(Transient)
	TObjectPtr<UUserWidget> Menu;

	UPROPERTY(Transient)
	TObjectPtr<UUserWidget> QuitDialog;

	void FocusMenu(class UWidget* Widget);
	void LogMenuSelection(const TCHAR* Selection);
	void CloseSettings();
	void OpenCatalog(bool bStructures);
	void CloseCatalog();
	UPROPERTY(Transient) TObjectPtr<class UFMCatalogWidget> CatalogPanel;
	void CloseCustomize();
	void ClosePlayPanel();
	void HandleNetworkFailure(UWorld* World, class UNetDriver* Driver, ENetworkFailure::Type Failure, const FString& Error);

	UPROPERTY(Transient)
	TObjectPtr<class UFMLobbyWidget> PlayPanel;
	bool bJoiningRoom = false;

	UFUNCTION()
	void CreateRoom();
	UFUNCTION()
	void CreateSteamRoom();
	UFUNCTION()
	bool JoinRoom(const FString& Address);
	bool IsSteamTravelBusy() const;

	UPROPERTY(Transient)
	TObjectPtr<class UFMCustomizeWidget> CustomizePanel;

	UPROPERTY(Transient)
	TObjectPtr<class UFMSettingsWidget> SettingsPanel;

	UFUNCTION()
	void Play();
	UFUNCTION()
	void Armory();
	UFUNCTION()
	void Structure();
	UFUNCTION()
	void Training();
	UFUNCTION()
	void Customize();
	UFUNCTION()
	void Shop();
	UFUNCTION()
	void Settings();
	UFUNCTION()
	void ShowQuitDialog();
	UFUNCTION()
	void CancelQuit();
	UFUNCTION()
	void ConfirmQuit();
};
