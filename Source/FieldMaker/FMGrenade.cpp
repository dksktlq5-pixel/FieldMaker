#include "FMGrenade.h"

#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"
#include "Sound/SoundBase.h"
#include "UObject/ConstructorHelpers.h"

AFMGrenade::AFMGrenade()
{
	bReplicates = true;
	SetReplicateMovement(true);

	auto* Collision = CreateDefaultSubobject<USphereComponent>(TEXT("Collision"));
	SetRootComponent(Collision);
	Collision->InitSphereRadius(4.f);
	Collision->SetCollisionProfileName(TEXT("BlockAllDynamic"));
	Collision->SetCollisionObjectType(ECC_WorldDynamic);
	Collision->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	Collision->SetGenerateOverlapEvents(false);
	Collision->SetCollisionResponseToChannel(ECC_Pawn, ECR_Ignore);

	auto* Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	Mesh->SetupAttachment(Collision);
	Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	static ConstructorHelpers::FObjectFinder<UStaticMesh> GrenadeMesh(TEXT("/Game/FieldMaker/Weapons/Realistic/SM_FM_G67.SM_FM_G67"));
	Mesh->SetStaticMesh(GrenadeMesh.Object);

	auto* Projectile = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("Projectile"));
	Projectile->SetUpdatedComponent(Collision);
	Projectile->InitialSpeed = 1450.f;
	Projectile->MaxSpeed = 2000.f;
	Projectile->bShouldBounce = true;
	Projectile->Bounciness = 0.42f;
	Projectile->bAutoActivate = false;
}

void AFMGrenade::BeginPlay()
{
	Super::BeginPlay();
	if (HasAuthority())
	{
		auto* Projectile = FindComponentByClass<UProjectileMovementComponent>();
		Projectile->Velocity = GetActorForwardVector() * Projectile->InitialSpeed + FVector(0, 0, 180);
		Projectile->Activate();
		FTimerHandle Fuse;
		GetWorldTimerManager().SetTimer(Fuse, this, &AFMGrenade::Detonate, 1.8f);
	}
}

void AFMGrenade::Detonate()
{
	UGameplayStatics::ApplyRadialDamageWithFalloff(this, 85.f, 15.f, GetActorLocation(), 100.f, 450.f,
		1.f, nullptr, {}, this, GetInstigatorController());
 MulticastExplosionSound(GetActorLocation());
 Destroy();
}

void AFMGrenade::MulticastExplosionSound_Implementation(FVector Location)
{
 if(GetNetMode()==NM_DedicatedServer)return;
 if(auto* Sound=LoadObject<USoundBase>(nullptr,TEXT("/Game/FreeWeaponSFX/Wav/GrenadeLauncher/sfx_wpn_gl_explosion.sfx_wpn_gl_explosion")))UGameplayStatics::PlaySoundAtLocation(this,Sound,Location,.6f);
}
