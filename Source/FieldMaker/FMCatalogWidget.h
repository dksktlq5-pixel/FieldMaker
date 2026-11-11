#pragma once

#include "Blueprint/UserWidget.h"
#include "Styling/SlateBrush.h"
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
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	virtual FReply NativeOnKeyDown(const FGeometry& Geometry, const FKeyEvent& Event) override;
private:
	struct FEntry
	{
		FString Name;
		FString Details;
		TSoftObjectPtr<class UStaticMesh> Mesh;
	};
	TArray<FEntry> Entries;
	int32 Selected = INDEX_NONE;
	FSlateBrush PreviewBrush;
	FTimerHandle PreviewCaptureTimer;
	UPROPERTY(Transient) TObjectPtr<class AStaticMeshActor> Model;
	UPROPERTY(Transient) TObjectPtr<class ASceneCapture2D> Capture;
	UPROPERTY(Transient) TObjectPtr<class APointLight> Light;
	UPROPERTY(Transient) TObjectPtr<class UTextureRenderTarget2D> Target;
	void Select(int32 Index);
};
