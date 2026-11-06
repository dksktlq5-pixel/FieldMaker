#pragma once

#include "Blueprint/UserWidget.h"
#include "Styling/SlateTypes.h"
#include "FMCustomizeWidget.generated.h"

DECLARE_DELEGATE_OneParam(FFMAppearanceApplied,uint32);

UCLASS()
class UFMCustomizeWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	FSimpleDelegate OnClosed;
 FFMAppearanceApplied OnApplied;
	FButtonStyle ButtonStyle;
	FSlateFontInfo MenuFont;
	UFUNCTION(BlueprintCallable, Category="Appearance") void SelectStyle(int32 CategoryIndex,int32 StyleIndex);
 UFUNCTION(BlueprintPure, Category="Appearance") int32 GetDraftAppearance() const { return int32(DraftAppearance); }
 UFUNCTION(BlueprintCallable, Category="Appearance") bool ApplyDraft();
 void InitializePreview(float ExposureEV = 0.f, bool bHideEnvironment = false);
 void CleanupPreview();
 const FSlateBrush* GetPreviewBrush() const { return &PreviewBrush; }

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeDestruct() override;
 virtual FReply NativeOnKeyDown(const FGeometry& Geometry,const FKeyEvent& Event) override;

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
	uint32 DraftAppearance = 0;
	int32 Category = 0;
};
