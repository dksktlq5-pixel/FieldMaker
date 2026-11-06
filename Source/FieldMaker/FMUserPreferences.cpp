#include "FMUserPreferences.h"
#include "Misc/ConfigCacheIni.h"

namespace
{
constexpr TCHAR Section[] = TEXT("FieldMaker.LocalPreferences");
const TCHAR* const AudioKeys[] = {TEXT("Master"), TEXT("Music"), TEXT("Effects"), TEXT("UI")};
FMLocalPreferences Read()
{
	FMLocalPreferences Values;
	GConfig->GetFloat(Section, TEXT("Sensitivity"), Values.Sensitivity, GGameUserSettingsIni);
	Values.Sensitivity = FMath::Clamp(Values.Sensitivity, 0.1f, 3.0f);
	GConfig->GetBool(Section, TEXT("InvertY"), Values.bInvertY, GGameUserSettingsIni);
	GConfig->GetBool(Section, TEXT("ToggleADS"), Values.bToggleADS, GGameUserSettingsIni);
	GConfig->GetFloat(Section, TEXT("ADSSensitivity"), Values.ADSSensitivity, GGameUserSettingsIni);
	Values.ADSSensitivity = FMath::Clamp(Values.ADSSensitivity, 0.1f, 1.5f);
	for (int32 Index = 0; Index < UE_ARRAY_COUNT(AudioKeys); ++Index)
	{
		GConfig->GetFloat(Section, AudioKeys[Index], Values.Volume[Index], GGameUserSettingsIni);
		Values.Volume[Index] = FMath::Clamp(Values.Volume[Index], 0.0f, 1.0f);
		GConfig->GetBool(Section, *(FString(AudioKeys[Index]) + TEXT("Muted")), Values.Muted[Index], GGameUserSettingsIni);
	}
	return Values;
}

FMLocalPreferences& LocalValues()
{
	static FMLocalPreferences Values = Read();
	return Values;
}
}

const FMLocalPreferences& FMLocalPreferences::Get()
{
	return LocalValues();
}

void FMLocalPreferences::Save() const
{
	LocalValues() = *this;
	GConfig->SetFloat(Section, TEXT("Sensitivity"), Sensitivity, GGameUserSettingsIni);
	GConfig->SetBool(Section, TEXT("InvertY"), bInvertY, GGameUserSettingsIni);
	GConfig->SetBool(Section, TEXT("ToggleADS"), bToggleADS, GGameUserSettingsIni);
	GConfig->SetFloat(Section, TEXT("ADSSensitivity"), ADSSensitivity, GGameUserSettingsIni);
	for (int32 Index = 0; Index < UE_ARRAY_COUNT(AudioKeys); ++Index)
	{
		GConfig->SetFloat(Section, AudioKeys[Index], Volume[Index], GGameUserSettingsIni);
		GConfig->SetBool(Section, *(FString(AudioKeys[Index]) + TEXT("Muted")), Muted[Index], GGameUserSettingsIni);
	}
	GConfig->Flush(false, GGameUserSettingsIni);
}
