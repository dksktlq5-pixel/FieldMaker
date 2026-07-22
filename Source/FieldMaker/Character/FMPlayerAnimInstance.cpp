#include "FieldMaker/Character/FMPlayerAnimInstance.h"

#include "FieldMaker/Character/FMPlayerCharacter.h"

#include "GameFramework/CharacterMovementComponent.h"
#include "KismetAnimationLibrary.h"


void UFMPlayerAnimInstance::NativeInitializeAnimation()
{
	Super::NativeInitializeAnimation();

	PlayerCharacter =
		Cast<AFMPlayerCharacter>(TryGetPawnOwner());

	if (PlayerCharacter)
	{
		CharacterMovement =
			PlayerCharacter->GetCharacterMovement();
	}
}


void UFMPlayerAnimInstance::NativeUpdateAnimation(
	float DeltaSeconds
)
{
	Super::NativeUpdateAnimation(DeltaSeconds);

	/*
	 * 애니메이션 블루프린트 미리보기 또는 초기 생성 시점에는
	 * Pawn Owner가 아직 없을 수 있으므로 다시 검색한다.
	 */
	if (!PlayerCharacter)
	{
		PlayerCharacter =
			Cast<AFMPlayerCharacter>(TryGetPawnOwner());
	}

	if (!PlayerCharacter)
	{
		return;
	}

	if (!CharacterMovement)
	{
		CharacterMovement =
			PlayerCharacter->GetCharacterMovement();
	}

	if (!CharacterMovement)
	{
		return;
	}

	/* =========================================================
	 * Velocity
	 * ========================================================= */

	Velocity = PlayerCharacter->GetVelocity();

	/*
	 * Z축 속도는 점프와 낙하 값이므로,
	 * 지상 이동 속도를 계산할 때는 제거한다.
	 */
	const FVector HorizontalVelocity(
		Velocity.X,
		Velocity.Y,
		0.0f
	);

	GroundSpeed = HorizontalVelocity.Size();

	/* =========================================================
	 * Direction
	 * ========================================================= */

	Direction =
		UKismetAnimationLibrary::CalculateDirection(
			HorizontalVelocity,
			PlayerCharacter->GetActorRotation()
		);

	/* =========================================================
	 * Movement State
	 * ========================================================= */

	const bool bHasAcceleration =
		CharacterMovement
			->GetCurrentAcceleration()
			.SizeSquared2D() > KINDA_SMALL_NUMBER;

	bShouldMove =
		GroundSpeed > 3.0f &&
		bHasAcceleration;

	bIsFalling =
		CharacterMovement->IsFalling();

	bIsCrouched =
		PlayerCharacter->bIsCrouched;
}