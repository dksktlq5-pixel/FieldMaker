#pragma once

#include "Blueprint/UserWidget.h"
#include "Styling/SlateTypes.h"
#include "FMCustomizeWidget.generated.h"

UCLASS()
class UFMCustomizeWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	FSimpleDelegate OnClosed;
	FButtonStyle ButtonStyle;
	FSlateFontInfo MenuFont;
	void InitializePreview(float ExposureEV = 0.f, bool bHideEnvironment = false);
 void CleanupPreview();
 const FSlateBrush* GetPreviewBrush() const { return &PreviewBrush; }

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeDestruct() override;

private:
	UPROPERTY(Transient)
	TObjectPtr<class AFMCharacter> Preview;
	UPROPERTY(Transient)
	TObjectPtr<class ASceneCapture2D> Capture;
	UPROPERTY(Transient)
	TObjectPtr<class APointLight> KeyLight;
	UPROPERTY(Transient)
	TObjectPtr<class APointLight> FillLight;
	UPROPERTY(Transient)
	TObjectPtr<class UTextureRenderTarget2D> PreviewTarget;

	FSlateBrush PreviewBrush;
	FText Status;
	uint8 DraftAppearance = 0;
	int32 Category = 0;
};
