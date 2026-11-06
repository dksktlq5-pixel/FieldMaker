#include "FMPlayerState.h"
#include "FMCharacter.h"
#include "Net/UnrealNetwork.h"

void AFMPlayerState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AFMPlayerState, AppearanceId);
	DOREPLIFETIME(AFMPlayerState, Team);
}

void AFMPlayerState::SetAppearanceId(uint8 Id)
{
	if (HasAuthority() && FMAppearance::IsValid(Id))
	{
		AppearanceId = Id;
		OnRep_AppearanceId();
	}
}

void AFMPlayerState::OnRep_AppearanceId()
{
	if (auto* Character = Cast<AFMCharacter>(GetPawn()))
	{
		Character->ApplyAppearance(AppearanceId);
	}
}
