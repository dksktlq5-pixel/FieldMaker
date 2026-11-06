#pragma once

#include "Blueprint/UserWidget.h"
#include "Styling/SlateTypes.h"
#include "Fonts/SlateFontInfo.h"
#include "FMSettingsWidget.generated.h"

UCLASS()
class UFMSettingsWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	FSimpleDelegate OnClosed;
	FButtonStyle ButtonStyle;
	FSlateFontInfo MenuFont;

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
};
