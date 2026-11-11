#pragma once

#include "Blueprint/UserWidget.h"
#include "FMCatalogWidget.generated.h"

UCLASS()
class FIELDMAKER_API UFMCatalogWidget : public UUserWidget
{
	GENERATED_BODY()
public:
	bool bStructures = false;
	FSimpleDelegate OnClosed;

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual FReply NativeOnKeyDown(const FGeometry& Geometry, const FKeyEvent& Event) override;
};
