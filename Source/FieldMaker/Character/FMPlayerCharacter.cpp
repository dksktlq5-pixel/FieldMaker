#include "FieldMaker/Character/FMPlayerCharacter.h"

#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"

#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputAction.h"
#include "InputActionValue.h"
#include "InputMappingContext.h"

#include "Engine/LocalPlayer.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"


AFMPlayerCharacter::AFMPlayerCharacter()
{
	// 현재 캐릭터에는 프레임마다 실행할 로직이 없으므로 Tick 비활성화
	PrimaryActorTick.bCanEverTick = false;

	// 멀티플레이 복제 활성화
	bReplicates = true;

	// 기본 캐릭터 충돌 크기
	GetCapsuleComponent()->InitCapsuleSize(42.0f, 96.0f);

	/* =========================================================
	 * FPS Rotation
	 * ========================================================= */

	// 좌우 시점 회전에 캐릭터가 따라가도록 설정
	bUseControllerRotationYaw = true;

	// 위아래 회전은 카메라에만 적용
	bUseControllerRotationPitch = false;
	bUseControllerRotationRoll = false;

	// 이동 방향으로 캐릭터가 자동 회전하지 않도록 설정
	GetCharacterMovement()->bOrientRotationToMovement = false;

	/* =========================================================
	 * Character Movement
	 * ========================================================= */

	// 앉기 사용 허용
	GetCharacterMovement()
		->GetNavAgentPropertiesRef()
		.bCanCrouch = true;

	// 기본 이동 속도
	GetCharacterMovement()->MaxWalkSpeed = DefaultMoveSpeed;

	// 앉은 상태 이동 속도
	GetCharacterMovement()->MaxWalkSpeedCrouched =
		CrouchedMoveSpeed;

	// 가속과 감속
	GetCharacterMovement()->MaxAcceleration = 2048.0f;
	GetCharacterMovement()->BrakingDecelerationWalking = 2048.0f;

	/* =========================================================
	 * Third Person Mesh
	 * ========================================================= */

	// ACharacter 기본 Mesh는 다른 플레이어가 보는 전신 메시
	GetMesh()->SetOwnerNoSee(true);
	GetMesh()->SetOnlyOwnerSee(false);

	GetMesh()->SetCastShadow(true);
	GetMesh()->bCastHiddenShadow = true;

	// 현재는 이동 충돌이나 Trace를 방해하지 않도록 비활성화
	GetMesh()->SetCollisionEnabled(
		ECollisionEnabled::NoCollision
	);

	/* =========================================================
	 * First Person Camera
	 * ========================================================= */

	FirstPersonCamera =
		CreateDefaultSubobject<UCameraComponent>(
			TEXT("FirstPersonCamera")
		);

	FirstPersonCamera->SetupAttachment(
		GetCapsuleComponent()
	);

	// 캡슐 중심을 기준으로 카메라 높이 설정
	FirstPersonCamera->SetRelativeLocation(
		FVector(0.0f, 0.0f, 64.0f)
	);

	// Controller의 Pitch와 Yaw를 카메라에 반영
	FirstPersonCamera->bUsePawnControlRotation = true;

	/* =========================================================
	 * First Person Mesh
	 * ========================================================= */

	FirstPersonMesh =
		CreateDefaultSubobject<USkeletalMeshComponent>(
			TEXT("FirstPersonMesh")
		);

	FirstPersonMesh->SetupAttachment(
		FirstPersonCamera
	);

	// 본인에게만 보이는 1인칭 메시
	FirstPersonMesh->SetOnlyOwnerSee(true);
	FirstPersonMesh->SetOwnerNoSee(false);

	// 충돌과 오버랩 비활성화
	FirstPersonMesh->SetCollisionEnabled(
		ECollisionEnabled::NoCollision
	);

	FirstPersonMesh->SetGenerateOverlapEvents(false);

	// 1인칭 메시 그림자 비활성화
	FirstPersonMesh->SetCastShadow(false);

	// 위치와 회전은 블루프린트에서 조절
	FirstPersonMesh->SetRelativeLocation(
		FVector::ZeroVector
	);

	FirstPersonMesh->SetRelativeRotation(
		FRotator::ZeroRotator
	);
}


void AFMPlayerCharacter::BeginPlay()
{
	Super::BeginPlay();
}


void AFMPlayerCharacter::SetupPlayerInputComponent(
	UInputComponent* PlayerInputComponent
)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	/* =========================================================
	 * Mapping Context 등록
	 * ========================================================= */

	APlayerController* PlayerController =
		Cast<APlayerController>(GetController());

	if (PlayerController)
	{
		ULocalPlayer* LocalPlayer =
			PlayerController->GetLocalPlayer();

		if (LocalPlayer)
		{
			UEnhancedInputLocalPlayerSubsystem* InputSubsystem =
				LocalPlayer->GetSubsystem<
					UEnhancedInputLocalPlayerSubsystem
				>();

			if (InputSubsystem && PlayerMappingContext)
			{
				InputSubsystem->AddMappingContext(
					PlayerMappingContext,
					0
				);
			}
		}
	}

	/* =========================================================
	 * Enhanced Input Component
	 * ========================================================= */

	UEnhancedInputComponent* EnhancedInputComponent =
		Cast<UEnhancedInputComponent>(
			PlayerInputComponent
		);

	if (!EnhancedInputComponent)
	{
		UE_LOG(
			LogTemp,
			Error,
			TEXT(
				"FMPlayerCharacter: "
				"EnhancedInputComponent를 찾지 못했습니다."
			)
		);

		return;
	}

	/* =========================================================
	 * Move
	 * ========================================================= */

	if (MoveAction)
	{
		EnhancedInputComponent->BindAction(
			MoveAction,
			ETriggerEvent::Triggered,
			this,
			&AFMPlayerCharacter::Move
		);
	}

	/* =========================================================
	 * Look
	 * ========================================================= */

	if (LookAction)
	{
		EnhancedInputComponent->BindAction(
			LookAction,
			ETriggerEvent::Triggered,
			this,
			&AFMPlayerCharacter::Look
		);
	}

	/* =========================================================
	 * Jump
	 * ========================================================= */

	if (JumpAction)
	{
		EnhancedInputComponent->BindAction(
			JumpAction,
			ETriggerEvent::Started,
			this,
			&AFMPlayerCharacter::StartJump
		);

		EnhancedInputComponent->BindAction(
			JumpAction,
			ETriggerEvent::Completed,
			this,
			&AFMPlayerCharacter::StopJump
		);

		EnhancedInputComponent->BindAction(
			JumpAction,
			ETriggerEvent::Canceled,
			this,
			&AFMPlayerCharacter::StopJump
		);
	}

	/* =========================================================
	 * Crouch
	 * ========================================================= */

	if (CrouchAction)
	{
		EnhancedInputComponent->BindAction(
			CrouchAction,
			ETriggerEvent::Started,
			this,
			&AFMPlayerCharacter::ToggleCrouch
		);
	}

	/* =========================================================
	 * Silent Walk
	 * ========================================================= */

	if (SilentWalkAction)
	{
		EnhancedInputComponent->BindAction(
			SilentWalkAction,
			ETriggerEvent::Started,
			this,
			&AFMPlayerCharacter::StartSilentWalk
		);

		EnhancedInputComponent->BindAction(
			SilentWalkAction,
			ETriggerEvent::Completed,
			this,
			&AFMPlayerCharacter::StopSilentWalk
		);

		EnhancedInputComponent->BindAction(
			SilentWalkAction,
			ETriggerEvent::Canceled,
			this,
			&AFMPlayerCharacter::StopSilentWalk
		);
	}
}


void AFMPlayerCharacter::Move(const FInputActionValue& Value)
{
	const FVector2D MovementInput = Value.Get<FVector2D>();

	if (!Controller)
	{
		return;
	}

	const FRotator ControlRotation = Controller->GetControlRotation();
	const FRotator YawRotation(0.f, ControlRotation.Yaw, 0.f);

	const FVector ForwardDirection =
		FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);

	const FVector RightDirection =
		FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

	// X = 앞뒤
	AddMovementInput(ForwardDirection, MovementInput.X);

	// Y = 좌우
	AddMovementInput(RightDirection, MovementInput.Y);
}


void AFMPlayerCharacter::Look(
	const FInputActionValue& Value
)
{
	const FVector2D LookInput =
		Value.Get<FVector2D>();

	AddControllerYawInput(LookInput.X);
	AddControllerPitchInput(LookInput.Y);
}


void AFMPlayerCharacter::StartJump()
{
	Jump();
}


void AFMPlayerCharacter::StopJump()
{
	StopJumping();
}


void AFMPlayerCharacter::ToggleCrouch()
{
	// 공중에서는 앉기 전환 금지
	if (GetCharacterMovement()->IsFalling())
	{
		return;
	}

	if (bIsCrouched)
	{
		UnCrouch();
	}
	else
	{
		Crouch();
	}
}


void AFMPlayerCharacter::StartSilentWalk()
{
	GetCharacterMovement()->MaxWalkSpeed =
		SilentWalkSpeed;
}


void AFMPlayerCharacter::StopSilentWalk()
{
	GetCharacterMovement()->MaxWalkSpeed =
		DefaultMoveSpeed;
}