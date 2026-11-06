#pragma once
#include "Blueprint/UserWidget.h"
#include "Styling/SlateTypes.h"
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
 UPROPERTY(Transient) TObjectPtr<class UFMCustomizeWidget> CharacterPreview;
 FName SelectedId() const;
 FText SelectedName() const;
 int32 IconFor(FName Id) const;
 FButtonStyle CardStyle;
 FButtonStyle ActionStyle;
 FText DetailText() const;
 FText ActionText() const;
 bool CanActivate() const;
};
