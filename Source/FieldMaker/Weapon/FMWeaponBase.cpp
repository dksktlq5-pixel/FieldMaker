#include "FMWeaponBase.h"

#include "FMWeaponDefinition.h"

#include "Components/PrimitiveComponent.h"
#include "Components/SceneComponent.h"

#include "Engine/DataTable.h"
#include "Engine/DamageEvents.h"
#include "Engine/World.h"

#include "GameFramework/Controller.h"
#include "GameFramework/DamageType.h"
#include "GameFramework/Pawn.h"

#include "Kismet/GameplayStatics.h"
#include "Net/UnrealNetwork.h"
#include "TimerManager.h"
#include "DrawDebugHelpers.h"

AFMWeaponBase::AFMWeaponBase()
{
	PrimaryActorTick.bCanEverTick = false;

	bReplicates = true;
	SetReplicateMovement(false);

	SceneRoot =
		CreateDefaultSubobject<USceneComponent>(
			TEXT("SceneRoot")
		);

	SetRootComponent(SceneRoot);
}


void AFMWeaponBase::BeginPlay()
{
	Super::BeginPlay();

	/*
	 * 서버와 클라이언트 모두 동일한 DataTable을 읽는다.
	 *
	 * Damage, FireInterval, SocketName 같은 정적 설정은
	 * 별도로 복제하지 않고 같은 DataTable에서 초기화한다.
	 */
	const bool bInitialized =
		InitializeFromWeaponData();

	if (!bInitialized)
	{
		UE_LOG(
			LogTemp,
			Warning,
			TEXT(
				"%s: WeaponData 초기화에 실패했습니다. "
				"기본값으로 계속 실행합니다."
			),
			*GetName()
		);
	}

	/*
	 * 현재 탄약과 Runtime State는 서버에서만 초기화하고,
	 * 클라이언트는 Replication으로 전달받는다.
	 */
	if (HasAuthority())
	{
		CurrentAmmo = MagazineSize;
		ReserveAmmo = InitialReserveAmmo;
		WeaponState = EFMWeaponState::Idle;
	}
}

bool AFMWeaponBase::InitializeFromWeaponData()
{
	/* =========================================================
	 * DataTable 검사
	 * ========================================================= */

	if (!WeaponDataTable)
	{
		UE_LOG(
			LogTemp,
			Error,
			TEXT(
				"%s: WeaponDataTable이 지정되지 않았습니다."
			),
			*GetName()
		);

		return false;
	}

	if (WeaponRowName.IsNone())
	{
		UE_LOG(
			LogTemp,
			Error,
			TEXT(
				"%s: WeaponRowName이 지정되지 않았습니다."
			),
			*GetName()
		);

		return false;
	}

	/* =========================================================
	 * Row 검색
	 * ========================================================= */

	const FFMWeaponDefinition* WeaponData =
		WeaponDataTable->FindRow<FFMWeaponDefinition>(
			WeaponRowName,
			TEXT("AFMWeaponBase::InitializeFromWeaponData")
		);

	if (!WeaponData)
	{
		UE_LOG(
			LogTemp,
			Error,
			TEXT(
				"%s: DT에서 Weapon Row를 찾지 못했습니다. "
				"RowName = %s"
			),
			*GetName(),
			*WeaponRowName.ToString()
		);

		return false;
	}

	/* =========================================================
	 * Identity / Slot
	 * ========================================================= */

	WeaponID =
		WeaponData->WeaponID.IsNone()
			? WeaponRowName
			: WeaponData->WeaponID;

	WeaponSlot =
		WeaponData->WeaponSlot;

	/* =========================================================
	 * Fire
	 * ========================================================= */

	Damage =
		FMath::Max(
			WeaponData->Damage,
			0.0f
		);

	FireRange =
		FMath::Max(
			WeaponData->FireRange,
			0.0f
		);

	FireInterval =
		FMath::Max(
			WeaponData->FireInterval,
			0.01f
		);

	FireMode =
		WeaponData->FireMode;

	/* =========================================================
	 * Ammo
	 * ========================================================= */

	MagazineSize =
		FMath::Max(
			WeaponData->MagazineSize,
			1
		);

	InitialReserveAmmo =
		FMath::Max(
			WeaponData->InitialReserveAmmo,
			0
		);

	/* =========================================================
	 * Reload
	 * ========================================================= */

	ReloadDuration =
		FMath::Max(
			WeaponData->ReloadDuration,
			0.0f
		);

	/* =========================================================
	 * Attachment
	 * ========================================================= */

	if (!WeaponData->FirstPersonSocket.IsNone())
	{
		FirstPersonSocketName =
			WeaponData->FirstPersonSocket;
	}

	if (!WeaponData->ThirdPersonSocket.IsNone())
	{
		ThirdPersonSocketName =
			WeaponData->ThirdPersonSocket;
	}

	UE_LOG(
		LogTemp,
		Log,
		TEXT(
			"%s: WeaponData 초기화 완료 | "
			"Row=%s | WeaponID=%s | "
			"Magazine=%d | Reserve=%d | "
			"Damage=%.1f | Interval=%.3f"
		),
		*GetName(),
		*WeaponRowName.ToString(),
		*WeaponID.ToString(),
		MagazineSize,
		InitialReserveAmmo,
		Damage,
		FireInterval
	);

	return true;
}

/* =============================================================
 * Fire
 * ============================================================= */

void AFMWeaponBase::StartFire()
{
	if (!HasAuthority())
	{
		ServerStartFire();
		return;
	}

	StartFireInternal();
}


void AFMWeaponBase::StopFire()
{
	if (!HasAuthority())
	{
		ServerStopFire();
		return;
	}

	StopFireInternal();
}


void AFMWeaponBase::ServerStartFire_Implementation()
{
	StartFireInternal();
}


void AFMWeaponBase::ServerStopFire_Implementation()
{
	StopFireInternal();
}


void AFMWeaponBase::StartFireInternal()
{
	if (WeaponState == EFMWeaponState::Reloading)
	{
		return;
	}

	bWantsToFire = true;

	if (FireMode == EFMFireMode::SemiAuto)
	{
		FireOnce();
		bWantsToFire = false;
		return;
	}

	/*
	 * Full Auto는 버튼을 누른 순간 한 발 발사하고,
	 * FireInterval마다 다음 발사를 예약한다.
	 */
	FireOnce();
}


void AFMWeaponBase::StopFireInternal()
{
	bWantsToFire = false;

	GetWorldTimerManager().ClearTimer(
		FireTimerHandle
	);

	if (WeaponState == EFMWeaponState::Firing)
	{
		WeaponState = EFMWeaponState::Idle;
	}
}


bool AFMWeaponBase::CanFire() const
{
	if (!HasAuthority())
	{
		return false;
	}

	if (WeaponState == EFMWeaponState::Reloading)
	{
		return false;
	}

	if (CurrentAmmo <= 0)
	{
		return false;
	}

	if (!GetWorld())
	{
		return false;
	}

	const float CurrentTime =
		GetWorld()->GetTimeSeconds();

	return
		CurrentTime - LastFireTime >=
		FireInterval;
}


void AFMWeaponBase::FireOnce()
{
	if (!CanFire())
	{
		/*
		 * Full Auto 입력 중인데 발사 간격만 아직 안 지난 경우,
		 * 다음 발사 시간을 다시 예약한다.
		 */
		if (
			bWantsToFire &&
			FireMode == EFMFireMode::FullAuto &&
			CurrentAmmo > 0 &&
			WeaponState != EFMWeaponState::Reloading
		)
		{
			ScheduleNextShot();
		}

		return;
	}

	WeaponState = EFMWeaponState::Firing;

	CurrentAmmo--;

	UE_LOG(
		LogTemp,
		Log,
		TEXT(
			"%s: Fire | Ammo=%d / %d"
		),
		*GetName(),
		CurrentAmmo,
		ReserveAmmo
	);

	LastFireTime =
		GetWorld()->GetTimeSeconds();

	PerformHitScan();

	if (
		FireMode == EFMFireMode::FullAuto &&
		bWantsToFire &&
		CurrentAmmo > 0
	)
	{
		ScheduleNextShot();
	}
	else
	{
		WeaponState = EFMWeaponState::Idle;
		bWantsToFire = false;
	}
}


void AFMWeaponBase::ScheduleNextShot()
{
	if (!GetWorld())
	{
		return;
	}

	const float CurrentTime =
		GetWorld()->GetTimeSeconds();

	const float TimeSinceLastShot =
		CurrentTime - LastFireTime;

	const float RemainingTime =
		FMath::Max(
			FireInterval - TimeSinceLastShot,
			0.001f
		);

	GetWorldTimerManager().SetTimer(
		FireTimerHandle,
		this,
		&AFMWeaponBase::FireOnce,
		RemainingTime,
		false
	);
}


void AFMWeaponBase::PerformHitScan()
{
	UWorld* World = GetWorld();

	if (!World)
	{
		return;
	}

	APawn* OwnerPawn =
		Cast<APawn>(GetOwner());

	if (!OwnerPawn)
	{
		return;
	}

	AController* OwnerController =
		OwnerPawn->GetController();

	FVector TraceStart;
	FRotator ViewRotation;

	if (OwnerController)
	{
		OwnerController->GetPlayerViewPoint(
			TraceStart,
			ViewRotation
		);
	}
	else
	{
		OwnerPawn->GetActorEyesViewPoint(
			TraceStart,
			ViewRotation
		);
	}

	const FVector TraceDirection =
		ViewRotation.Vector();

	const FVector TraceEnd =
		TraceStart +
		TraceDirection * FireRange;

	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(this);
	QueryParams.AddIgnoredActor(OwnerPawn);
	QueryParams.bReturnPhysicalMaterial = true;
	QueryParams.bTraceComplex = true;

	FHitResult HitResult;

	const bool bHit =
		World->LineTraceSingleByChannel(
			HitResult,
			TraceStart,
			TraceEnd,
			FireTraceChannel,
			QueryParams
		);

	/*
	 * 실제 충돌 위치까지만 선을 표시한다.
	 * 적중: 초록색
	 * 미적중: 빨간색
	 */
	const FVector DebugEnd =
		bHit
			? HitResult.ImpactPoint
			: TraceEnd;

	DrawDebugLine(
		World,
		TraceStart,
		DebugEnd,
		bHit ? FColor::Green : FColor::Red,
		false,
		2.0f,
		0,
		1.5f
	);

	if (!bHit)
	{
		return;
	}

	DrawDebugSphere(
		World,
		HitResult.ImpactPoint,
		8.0f,
		12,
		FColor::Yellow,
		false,
		2.0f,
		0,
		1.0f
	);

	AActor* HitActor =
		HitResult.GetActor();

	if (!HitActor)
	{
		return;
	}

	UE_LOG(
		LogTemp,
		Log,
		TEXT(
			"%s: Hit Actor=%s | Location=%s"
		),
		*GetName(),
		*HitActor->GetName(),
		*HitResult.ImpactPoint.ToString()
	);

	UGameplayStatics::ApplyPointDamage(
		HitActor,
		Damage,
		TraceDirection,
		HitResult,
		OwnerController,
		this,
		DamageTypeClass
	);
}


/* =============================================================
 * Reload
 * ============================================================= */

void AFMWeaponBase::StartReload()
{
	if (!HasAuthority())
	{
		ServerStartReload();
		return;
	}

	StartReloadInternal();
}


void AFMWeaponBase::ServerStartReload_Implementation()
{
	StartReloadInternal();
}


void AFMWeaponBase::StartReloadInternal()
{
	if (!CanReload())
	{
		return;
	}

	StopFireInternal();

	WeaponState = EFMWeaponState::Reloading;

	GetWorldTimerManager().SetTimer(
		ReloadTimerHandle,
		this,
		&AFMWeaponBase::FinishReload,
		ReloadDuration,
		false
	);
}


void AFMWeaponBase::FinishReload()
{
	if (!HasAuthority())
	{
		return;
	}

	const int32 MissingAmmo =
		MagazineSize - CurrentAmmo;

	const int32 AmmoToLoad =
		FMath::Min(
			MissingAmmo,
			ReserveAmmo
		);

	CurrentAmmo += AmmoToLoad;
	ReserveAmmo -= AmmoToLoad;

	WeaponState = EFMWeaponState::Idle;

	UE_LOG(
		LogTemp,
		Log,
		TEXT(
			"%s: Reload Complete | Ammo=%d / %d"
		),
		*GetName(),
		CurrentAmmo,
		ReserveAmmo
	);
}


/* =============================================================
 * Visibility
 * ============================================================= */

void AFMWeaponBase::SetWeaponVisible(
	bool bVisible
)
{
	TArray<UPrimitiveComponent*> PrimitiveComponents;

	GetComponents<UPrimitiveComponent>(
		PrimitiveComponents
	);

	for (
		UPrimitiveComponent* Component :
		PrimitiveComponents
	)
	{
		if (!IsValid(Component))
		{
			continue;
		}

		Component->SetVisibility(
			bVisible,
			true
		);

		Component->SetHiddenInGame(
			!bVisible,
			true
		);
	}
}


/* =============================================================
 * Replication
 * ============================================================= */

void AFMWeaponBase::OnRep_CurrentAmmo()
{
	/*
	 * 이후 HUD 갱신 이벤트를 여기서 호출한다.
	 */
}


void AFMWeaponBase::OnRep_ReserveAmmo()
{
	/*
	 * 이후 HUD 갱신 이벤트를 여기서 호출한다.
	 */
}


void AFMWeaponBase::OnRep_WeaponState()
{
	/*
	 * 이후 AnimInstance 또는 Weapon ABP 갱신에 사용한다.
	 */
}


void AFMWeaponBase::GetLifetimeReplicatedProps(
	TArray<FLifetimeProperty>& OutLifetimeProps
) const
{
	Super::GetLifetimeReplicatedProps(
		OutLifetimeProps
	);

	DOREPLIFETIME(
		AFMWeaponBase,
		CurrentAmmo
	);

	DOREPLIFETIME(
		AFMWeaponBase,
		ReserveAmmo
	);

	DOREPLIFETIME(
		AFMWeaponBase,
		WeaponState
	);
}