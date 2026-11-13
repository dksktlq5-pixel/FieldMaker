#pragma once
#include "Blueprint/UserWidget.h"
#include "Styling/SlateTypes.h"
#include "Styling/SlateBrush.h"
#include "FMShopWidget.generated.h"

UCLASS()
class FIELDMAKER_API UFMShopWidget : public UUserWidget
{
 GENERATED_BODY()
public:
 UFUNCTION(BlueprintCallable, Category="Shop") void SelectItem(int32 Item);
 UFUNCTION(BlueprintCallable, Category="Shop") void ActivateSelection();
 UFUNCTION(BlueprintPure, Category="Shop") int32 GetSelectedItem() const { return Selection; }
 UFUNCTION(BlueprintCallable, Category="Shop") void SelectWeaponById(FName Id);
 UFUNCTION(BlueprintPure, Category="Shop") int32 GetCatalogCount() const { return Catalog.Num(); }
protected:
 virtual TSharedRef<SWidget> RebuildWidget() override;
 virtual void NativeDestruct() override;
 virtual FReply NativeOnKeyDown(const FGeometry& Geometry, const FKeyEvent& Event) override;
private:
 int32 Selection = 0;
 TArray<FName> Catalog;
 UPROPERTY(Transient) TArray<TObjectPtr<class UTexture2D>> WeaponIconTextures;
 UPROPERTY(Transient) TArray<TObjectPtr<class UTexture2D>> ItemIconTextures;
 UPROPERTY(Transient) TObjectPtr<class AStaticMeshActor> WeaponPreviewModel;
 UPROPERTY(Transient) TObjectPtr<class ASceneCapture2D> WeaponPreviewCapture;
 UPROPERTY(Transient) TObjectPtr<class APointLight> WeaponPreviewLight;
 UPROPERTY(Transient) TObjectPtr<class UTextureRenderTarget2D> WeaponPreviewTarget;
 TArray<FSlateBrush> WeaponIconBrushes;
 TMap<FName, FSlateBrush> ItemIconBrushes;
 FSlateBrush WeaponPreviewBrush;
 TSharedRef<SWidget> MakeItemImage(FName Id, int32 Icon);
 void UpdateWeaponPreview();
 FName SelectedId() const;
 FText SelectedName() const;
 int32 IconFor(FName Id) const;
 FButtonStyle CardStyle;
 FButtonStyle ActionStyle;
 FText DetailText() const;
 FText ActionText() const;
 bool CanActivate() const;
};
