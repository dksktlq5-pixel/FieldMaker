#include "FMAppearanceSaveGame.h"
#include "Kismet/GameplayStatics.h"

namespace
{
const FString SlotName(TEXT("FieldMaker.Appearance"));
}

uint32 UFMAppearanceSaveGame::Load()
{
	const auto* Save = Cast<UFMAppearanceSaveGame>(UGameplayStatics::LoadGameFromSlot(SlotName, 0));
	const uint32 Code=Save?(Save->bHasAppearanceCode?Save->AppearanceCode:Save->AppearanceId):FMAppearance::Default;
 return FMAppearance::IsValid(Code)?Code:FMAppearance::Default;
}

bool UFMAppearanceSaveGame::Save(uint32 Id)
{
	if (!FMAppearance::IsValid(Id))
	{
		return false;
	}

	auto* Save = CastChecked<UFMAppearanceSaveGame>(UGameplayStatics::CreateSaveGameObject(StaticClass()));
	Save->AppearanceCode = Id; Save->bHasAppearanceCode=true;
	return UGameplayStatics::SaveGameToSlot(Save, SlotName, 0);
}
