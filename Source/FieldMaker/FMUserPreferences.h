#pragma once

#include "CoreMinimal.h"

struct FMLocalPreferences
{
	float Sensitivity = 1.0f;
	bool bInvertY = false;
	bool bToggleADS = false;
	float ADSSensitivity = 0.65f;
	float Volume[4] = {1, 1, 1, 1};
	bool Muted[4] = {};

	static const FMLocalPreferences& Get();
	void Save() const;
};
