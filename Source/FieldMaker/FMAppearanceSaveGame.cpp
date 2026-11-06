#include "FMAppearanceSaveGame.h"
#include "Kismet/GameplayStatics.h"

namespace
{
const FString SlotName(TEXT("FieldMaker.Appearance"));
}

uint8 UFMAppearanceSaveGame::Load()
{
	const auto* Save = Cast<UFMAppearanceSaveGame>(UGameplayStatics::LoadGameFromSlot(SlotName, 0));
	return Save && FMAppearance::IsValid(Save->AppearanceId) ? Save->AppearanceId : FMAppearance::Default;
}

bool UFMAppearanceSaveGame::Save(uint8 Id)
{
	if (!FMAppearance::IsValid(Id))
	{
		return false;
	}

	auto* Save = CastChecked<UFMAppearanceSaveGame>(UGameplayStatics::CreateSaveGameObject(StaticClass()));
	Save->AppearanceId = Id;
	return UGameplayStatics::SaveGameToSlot(Save, SlotName, 0);
}
