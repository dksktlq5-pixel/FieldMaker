#pragma once

#include "GameFramework/Actor.h"
#include "FMStructure.generated.h"

class UBoxComponent;
class UStaticMesh;
class UStaticMeshComponent;

struct FFMStructureItem
{
	const TCHAR* Name;
	int32 Price;
	float Health;
	FVector HalfSize;
};

UCLASS()
class FIELDMAKER_API AFMStructure : public AActor
{
	GENERATED_BODY()
public:
	AFMStructure();
	static int32 FindItem(FName Id);
	static const FFMStructureItem* GetItem(int32 Index);
	void Initialize(int32 Index, uint8 Team);
	virtual float TakeDamage(float Damage, const FDamageEvent& Event, AController* InstigatorController, AActor* Causer) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
protected:
	virtual void BeginPlay() override;
private:
	UFUNCTION() void ApplyItem();
	UPROPERTY(VisibleAnywhere) TObjectPtr<UBoxComponent> Collision;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> Mesh;
	UPROPERTY() TObjectPtr<UStaticMesh> SandbagMesh;
	UPROPERTY() TObjectPtr<UStaticMesh> WallMesh;
	UPROPERTY(ReplicatedUsing=ApplyItem) int32 ItemIndex = 0;
	UPROPERTY(Replicated) float Health = 0;
	uint8 OwnerTeam = 2;
};
