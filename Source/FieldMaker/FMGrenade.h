#pragma once

#include "GameFramework/Actor.h"
#include "FMGrenade.generated.h"

UCLASS()
class AFMGrenade : public AActor
{
	GENERATED_BODY()

public:
	AFMGrenade();

protected:
	virtual void BeginPlay() override;

private:
	void Detonate();
 UFUNCTION(NetMulticast, Reliable) void MulticastExplosionSound(FVector Location);
};
