#include "FMStructure.h"
#include "FMGameState.h"
#include "FMPlayerState.h"
#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "GameFramework/Controller.h"
#include "Net/UnrealNetwork.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
const FFMStructureItem Items[] = {
	{TEXT("모래주머니"), 500, 200.f, FVector(85, 25, 45)},
	{TEXT("방탄벽"), 1000, 450.f, FVector(90, 20, 85)},
	{TEXT("발판 상자"), 400, 175.f, FVector(62.5, 62.5, 32.5)},
	{TEXT("컨테이너"), 1500, 650.f, FVector(200, 100, 70)}
};
}

AFMStructure::AFMStructure()
{
	bReplicates = true;
	PrimaryActorTick.bCanEverTick = false;
	Collision = CreateDefaultSubobject<UBoxComponent>(TEXT("StructureCollision"));
	SetRootComponent(Collision);
	Collision->SetCollisionProfileName(TEXT("BlockAllDynamic"));
	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("StructureMesh"));
	Mesh->SetupAttachment(Collision);
	Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cube(TEXT("/Engine/BasicShapes/Cube.Cube"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Sandbags(
		TEXT("/Game/StylizedBridge/StaticMesh/Road/Prefabs/SM_BarrierSandBag_A1.SM_BarrierSandBag_A1"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Wall(
		TEXT("/Game/StylizedBridge/StaticMesh/Road/Prefabs/SM_BarrierRoad_A1.SM_BarrierRoad_A1"));
	SandbagMesh = Sandbags.Succeeded() ? Sandbags.Object : Cube.Object;
	WallMesh = Wall.Succeeded() ? Wall.Object : Cube.Object;
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Crate(
		TEXT("/Game/StylizedBridge/StaticMesh/Props/SM_WoodBox_A1.SM_WoodBox_A1"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Container(
		TEXT("/Game/StylizedBridge/StaticMesh/Train/SM_TrainContainer_A1.SM_TrainContainer_A1"));
	CrateMesh = Crate.Succeeded() ? Crate.Object : Cube.Object;
	CrateLid = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("CrateLid"));
	CrateLid->SetupAttachment(Collision);
	CrateLid->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	CrateLid->SetStaticMesh(Cube.Object);
	if (Crate.Succeeded()) CrateLid->SetMaterial(0, Crate.Object->GetMaterial(0));
	CrateLid->SetVisibility(false);
	ContainerMesh = Container.Succeeded() ? Container.Object : Cube.Object;
}

int32 AFMStructure::FindItem(FName Id)
{
	if (Id == TEXT("Sandbags")) return 0;
	if (Id == TEXT("BallisticWall")) return 1;
	if (Id == TEXT("StepCrate")) return 2;
	if (Id == TEXT("BuildContainer")) return 3;
	return INDEX_NONE;
}

const FFMStructureItem* AFMStructure::GetItem(int32 Index)
{
	return Index >= 0 && Index < UE_ARRAY_COUNT(Items) ? &Items[Index] : nullptr;
}

void AFMStructure::BeginPlay()
{
	Super::BeginPlay();
	ApplyItem();
}

void AFMStructure::Initialize(int32 Index, uint8 Team, AFMStructure* Support)
{
	ItemIndex = Index;
	OwnerTeam = Team;
	Health = Items[Index].Health;
	if (Support) Support->OnDestroyed.AddDynamic(this, &AFMStructure::OnSupportDestroyed);
	ApplyItem();
}

void AFMStructure::OnSupportDestroyed(AActor*)
{
	Destroy();
}

void AFMStructure::ApplyItem()
{
	const auto* Item = GetItem(ItemIndex);
	if (!Item) return;
	Collision->SetBoxExtent(Item->HalfSize);
	CrateLid->SetVisibility(ItemIndex == 2);
	if (ItemIndex == 2)
	{
		CrateLid->SetRelativeLocation(FVector(0, 0, Item->HalfSize.Z - 2.5f));
		CrateLid->SetRelativeScale3D(FVector(Item->HalfSize.X / 50.f, Item->HalfSize.Y / 50.f, 0.05f));
	}
	UStaticMesh* const Visuals[] = {SandbagMesh.Get(), WallMesh.Get(), CrateMesh.Get(), ContainerMesh.Get()};
	auto* Visual = Visuals[ItemIndex];
	if (!Visual) return;
	Mesh->SetStaticMesh(Visual);
	const auto Bounds = Visual->GetBounds();
	const bool bTurn = Bounds.BoxExtent.Y > Bounds.BoxExtent.X;
	const FRotator Rotation(0.f, bTurn ? 90.f : 0.f, 0.f);
	const FVector DesiredExtent = bTurn ? FVector(Item->HalfSize.Y, Item->HalfSize.X, Item->HalfSize.Z) : Item->HalfSize;
	const FVector Scale = DesiredExtent / Bounds.BoxExtent.ComponentMax(FVector::OneVector);
	Mesh->SetRelativeTransform(FTransform(Rotation, -Rotation.RotateVector(Bounds.Origin * Scale), Scale));
}

float AFMStructure::TakeDamage(float Damage, const FDamageEvent& Event, AController* InstigatorController, AActor* Causer)
{
	const auto* State = GetWorld()->GetGameState<AFMGameState>();
	const auto* Attacker = InstigatorController ? InstigatorController->GetPlayerState<AFMPlayerState>() : nullptr;
	if (!HasAuthority() || IsActorBeingDestroyed() || !FMath::IsFinite(Damage) || Damage <= 0 || !State ||
		State->GetMatchSnapshot().Phase != EFMMatchPhase::Combat || State->GetPhaseTimeRemaining() <= 0 ||
		!Attacker || Attacker->GetTeam() >= 2 || Attacker->GetTeam() == OwnerTeam) return 0;
	const float Applied = FMath::Min(Damage, Health);
	Health -= Applied;
	if (Health <= 0) Destroy();
	return Applied;
}

void AFMStructure::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AFMStructure, ItemIndex);
	DOREPLIFETIME(AFMStructure, Health);
	DOREPLIFETIME(AFMStructure, OwnerTeam);
}
