#include "FMPlayerState.h"
#include "FMCharacter.h"
#include "Net/UnrealNetwork.h"

void AFMPlayerState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AFMPlayerState, AppearanceId);
	DOREPLIFETIME(AFMPlayerState, Team);
	DOREPLIFETIME_CONDITION(AFMPlayerState, Credits, COND_OwnerOnly);
	DOREPLIFETIME_CONDITION(AFMPlayerState, StructureStock, COND_OwnerOnly);
}

void AFMPlayerState::SetAppearanceId(uint32 Id)
{
	if (HasAuthority() && FMAppearance::IsValid(Id))
	{
		AppearanceId = Id;
		OnRep_AppearanceId();
        ForceNetUpdate();
	}
}

void AFMPlayerState::OnRep_AppearanceId()
{
	if (auto* Character = Cast<AFMCharacter>(GetPawn()))
	{
		Character->ApplyAppearance(AppearanceId);
	}
}

bool AFMPlayerState::SpendCredits(int32 Price)
{
	if (!HasAuthority() || Price < 0 || Credits < Price) return false;
	Credits -= Price;
	ForceNetUpdate();
	return true;
}

void AFMPlayerState::AddCredits(int32 Amount)
{
	if (!HasAuthority()) return;
	Credits = FMath::Clamp(Credits + Amount, 0, 12000);
	ForceNetUpdate();
}

void AFMPlayerState::ResetEconomy(int32 StartingCredits)
{
	if (!HasAuthority()) return;
	Credits = StartingCredits;
	StructureStock = {0, 0, 0, 0};
	ForceNetUpdate();
}

void AFMPlayerState::ChangeStructureStock(int32 Index, int32 Delta)
{
	if (!HasAuthority() || !StructureStock.IsValidIndex(Index)) return;
	StructureStock[Index] = FMath::Max(0, StructureStock[Index] + Delta);
	ForceNetUpdate();
}
