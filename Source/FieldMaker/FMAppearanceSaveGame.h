#pragma once

#include "GameFramework/SaveGame.h"
#include "FMAppearanceSaveGame.generated.h"

namespace FMAppearance
{
	enum EPart : uint8 { Cap = 1, Vest = 2, Drops = 4, Holster = 8, Patch = 16 };
	constexpr uint8 Default = Cap | Vest | Drops | Holster | Patch;

	inline bool IsValid(uint8 Id)
	{
		return (Id & ~Default) == 0 && (!(Id & Patch) || (Id & Vest));
	}
}

UCLASS()
class UFMAppearanceSaveGame : public USaveGame
{
	GENERATED_BODY()

public:
	static uint8 Load();
	static bool Save(uint8 Id);

private:
	UPROPERTY(SaveGame)
	uint8 AppearanceId = FMAppearance::Default;
};
