#pragma once
#include "GameFramework/Character.h"
#include "FMCharacter.generated.h"

struct FInputActionValue;

UCLASS()
class FIELDMAKER_API AFMCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	AFMCharacter();
	virtual void OnStartCrouch(float HalfHeightAdjust, float ScaledHalfHeightAdjust) override;
	virtual void OnEndCrouch(float HalfHeightAdjust, float ScaledHalfHeightAdjust) override;

protected:
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;

private:
	UPROPERTY(VisibleAnywhere, Category = "Camera")
	TObjectPtr<class UCameraComponent> FirstPersonCamera;

	void Move(const FInputActionValue& Value);
	void Look(const FInputActionValue& Value);
	void ToggleCrouch();
	void StartSprint();
	void StopSprint();
	void SetSprint(bool bSprint);

	UFUNCTION(Server, Reliable)
	void ServerSetSprint(bool bSprint);
};
