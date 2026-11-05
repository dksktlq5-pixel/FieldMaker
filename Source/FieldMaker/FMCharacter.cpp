#include "FMCharacter.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "EnhancedInputComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "InputAction.h"
#include "InputActionValue.h"

namespace
{
constexpr float WalkSpeed = 450.0f;
constexpr float SprintSpeed = 650.0f;
constexpr float CameraHeight = 64.0f;

const UInputAction* LoadAction(const TCHAR* Name)
{
	const FString Path = FString::Printf(TEXT("/Game/Input/%s.%s"), Name, Name);
	return LoadObject<UInputAction>(nullptr, *Path);
}
}

AFMCharacter::AFMCharacter()
{
	bUseControllerRotationYaw = true;
	FirstPersonCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FirstPersonCamera"));
	FirstPersonCamera->SetupAttachment(GetCapsuleComponent());
	FirstPersonCamera->SetRelativeLocation(FVector(0, 0, CameraHeight));
	FirstPersonCamera->bUsePawnControlRotation = true;

	auto* Movement = GetCharacterMovement();
	Movement->GetNavAgentPropertiesRef().bCanCrouch = true;
	Movement->MaxWalkSpeed = WalkSpeed;
}

void AFMCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	auto* Input = Cast<UEnhancedInputComponent>(PlayerInputComponent);
	if (!Input)
	{
		return;
	}

	if (const auto* Action = LoadAction(TEXT("IA_FMMove")))
	{
		Input->BindAction(Action, ETriggerEvent::Triggered, this, &AFMCharacter::Move);
	}
	if (const auto* Action = LoadAction(TEXT("IA_FMLook")))
	{
		Input->BindAction(Action, ETriggerEvent::Triggered, this, &AFMCharacter::Look);
	}
	if (const auto* Action = LoadAction(TEXT("IA_FMJump")))
	{
		Input->BindAction(Action, ETriggerEvent::Started, this, &AFMCharacter::Jump);
		Input->BindAction(Action, ETriggerEvent::Completed, this, &AFMCharacter::StopJumping);
		Input->BindAction(Action, ETriggerEvent::Canceled, this, &AFMCharacter::StopJumping);
	}
	if (const auto* Action = LoadAction(TEXT("IA_FMSprint")))
	{
		Input->BindAction(Action, ETriggerEvent::Started, this, &AFMCharacter::StartSprint);
		Input->BindAction(Action, ETriggerEvent::Completed, this, &AFMCharacter::StopSprint);
		Input->BindAction(Action, ETriggerEvent::Canceled, this, &AFMCharacter::StopSprint);
	}
	if (const auto* Action = LoadAction(TEXT("IA_FMCrouch")))
	{
		Input->BindAction(Action, ETriggerEvent::Started, this, &AFMCharacter::ToggleCrouch);
	}
}

void AFMCharacter::Move(const FInputActionValue& Value)
{
	const FVector2D Axis = Value.Get<FVector2D>();
	AddMovementInput(GetActorForwardVector(), Axis.Y);
	AddMovementInput(GetActorRightVector(), Axis.X);
}

void AFMCharacter::Look(const FInputActionValue& Value)
{
	const FVector2D Axis = Value.Get<FVector2D>();
	AddControllerYawInput(Axis.X);
	AddControllerPitchInput(Axis.Y);
}

void AFMCharacter::ToggleCrouch()
{
	if (GetCharacterMovement()->bWantsToCrouch)
	{
		UnCrouch();
	}
	else
	{
		Crouch();
	}
}

void AFMCharacter::OnStartCrouch(float HalfHeightAdjust, float ScaledHalfHeightAdjust)
{
	Super::OnStartCrouch(HalfHeightAdjust, ScaledHalfHeightAdjust);
	FirstPersonCamera->SetRelativeLocation(FVector(0, 0, CameraHeight - HalfHeightAdjust));
}

void AFMCharacter::OnEndCrouch(float HalfHeightAdjust, float ScaledHalfHeightAdjust)
{
	Super::OnEndCrouch(HalfHeightAdjust, ScaledHalfHeightAdjust);
	FirstPersonCamera->SetRelativeLocation(FVector(0, 0, CameraHeight));
}

void AFMCharacter::StartSprint()
{
	SetSprint(true);
}

void AFMCharacter::StopSprint()
{
	SetSprint(false);
}

void AFMCharacter::SetSprint(bool bSprint)
{
	GetCharacterMovement()->MaxWalkSpeed = bSprint ? SprintSpeed : WalkSpeed;
	if (!HasAuthority())
	{
		ServerSetSprint(bSprint);
	}
}

void AFMCharacter::ServerSetSprint_Implementation(bool bSprint)
{
	SetSprint(bSprint);
}
