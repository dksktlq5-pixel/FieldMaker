#include "FMShopWidget.h"
#include "FMCharacter.h"
#include "FMWeaponVisualData.h"
#include "Widgets/Images/SImage.h"
#include "FMPlayerController.h"
#include "FMGameState.h"
#include "FMPlayerState.h"
#include "FMStructure.h"
#include "Engine/Texture2D.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/SceneCapture2D.h"
#include "Engine/PointLight.h"
#include "Components/SceneCaptureComponent2D.h"
#include "Components/StaticMeshComponent.h"
#include "Components/PointLightComponent.h"
#include "Engine/World.h"
#include "Styling/CoreStyle.h"
#include "Brushes/SlateColorBrush.h"
#include "Rendering/DrawElements.h"
#include "Widgets/SLeafWidget.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/SOverlay.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SScaleBox.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Text/STextBlock.h"

namespace
{
bool IsArmor(FName Id){return Id==TEXT("LightArmor") || Id==TEXT("MediumArmor");}
const FLinearColor Mint(.85f,.69f,.29f), Ink(.035f,.039f,.043f), Paper(.90f,.90f,.88f), Muted(.53f,.54f,.54f);
int32 TextureIndexForIcon(int32 Icon)
{
 switch (Icon)
 {
 case 0: return 2; // rifle
 case 1: return 0; // pistol
 case 5: return 1; // SMG
 case 6: return 3; // sniper
 case 7: return 5; // shotgun
 case 8: return 4; // machine gun
 case 2: return 6; // knife
 case 3: return 7; // grenade
 default: return INDEX_NONE;
 }
}
TSharedRef<STextBlock> Text(const FText& Value,int32 Size,FLinearColor Color=Paper)
{
 return SNew(STextBlock).Text(Value).Font(FCoreStyle::GetDefaultFontStyle("Regular",Size)).ColorAndOpacity(Color).AutoWrapText(true);
}
TSharedRef<STextBlock> Text(const TCHAR* Value,int32 Size,FLinearColor Color=Paper) {return Text(FText::FromString(Value),Size,Color);}
// Lightweight original schematic icons; no external weapon artwork or runtime scene captures.
class SEquipmentIcon : public SLeafWidget
{
public:
 SLATE_BEGIN_ARGS(SEquipmentIcon) : _Item(0) {} SLATE_ATTRIBUTE(int32,Item) SLATE_END_ARGS()
 void Construct(const FArguments& Args) {Item=Args._Item;}
 virtual FVector2D ComputeDesiredSize(float) const override {return FVector2D(180,110);}
 virtual int32 OnPaint(const FPaintArgs&,const FGeometry& G,const FSlateRect&,FSlateWindowElementList& Out,int32 Layer,const FWidgetStyle&,bool) const override
 {
  const FVector2D Size=G.GetLocalSize();const float S=FMath::Min(Size.X/220.f,Size.Y/130.f);
  const FVector2D O((Size.X-220*S)/2,(Size.Y-130*S)/2);
  auto Box=[&](float X,float Y,float W,float H,FLinearColor C=Mint){FSlateDrawElement::MakeBox(Out,Layer,G.ToPaintGeometry(FVector2D(W*S,H*S),FSlateLayoutTransform(O+FVector2D(X*S,Y*S))),FCoreStyle::Get().GetBrush("WhiteBrush"),ESlateDrawEffect::None,C);};
  auto Line=[&](TArray<FVector2D> P,float Width=3.f){for(auto& V:P)V=O+V*S;FSlateDrawElement::MakeLines(Out,Layer+1,G.ToPaintGeometry(),P,ESlateDrawEffect::None,Mint,true,Width*S);};
  switch(Item.Get())
  {
  case 0: Box(45,47,120,17);Box(16,50,34,7);Box(163,48,39,13);Box(158,59,15,24);Box(116,63,13,27);Box(56,63,14,25);Box(67,39,10,8);Box(140,41,10,6);Line({{75,64},{75,76},{107,76},{112,64}},2);break;
  case 5: Box(40,51,120,20);Box(14,56,30,7);Box(150,49,40,9);Box(116,68,14,30);Box(71,69,14,28);break;
  case 6: Box(40,62,140,12);Box(10,65,40,5);Box(176,62,30,23);Box(134,71,13,25);Box(83,44,65,11);Box(94,55,8,8);Box(135,55,8,8);break;
  case 7: Box(22,51,155,11);Box(49,64,80,12);Box(165,56,40,21);Box(137,63,13,25);break;
  case 8: Box(35,49,140,19);Box(9,53,33,7);Box(166,50,40,22);Box(85,68,47,35);Box(141,68,12,25);break;
  case 1: Box(55,40,100,19);Box(139,56,18,49);Box(59,58,53,8);Line({{111,60},{110,84},{137,84}},3);Box(67,36,8,4);Box(145,36,7,4);break;
  case 2: Line({{23,38},{128,64},{133,79},{23,38},{119,81},{133,79}},4);Line({{132,62},{124,91}},5);Line({{134,80},{191,99}},15);break;
  case 3: Box(85,47,47,60);Box(91,34,31,13);Box(105,29,33,7);Line({{131,35},{145,47},{145,80}},5);Box(91,58,35,2,Muted);Box(91,73,35,2,Muted);Box(91,88,35,2,Muted);break;
  case 9: Box(44,48,132,53);Line({{44,65},{176,65}},2);Line({{44,83},{176,83}},2);break;
  case 4: Line({{110,20},{157,41},{151,79},{110,111},{69,79},{63,41},{110,20}},5);Box(102,45,16,39);Box(90,57,40,15);break;
  }
  return Layer+1;
 }
private:TAttribute<int32> Item;
};
}

FName UFMShopWidget::SelectedId() const {return Catalog.IsValidIndex(Selection)?Catalog[Selection]:NAME_None;}
TSharedRef<SWidget> UFMShopWidget::MakeItemImage(FName Id, int32 Icon)
{
 static const TSet<FName> CustomIds = {
  TEXT("Glock"),TEXT("Pistol"),TEXT("P250"),TEXT("FiveSeven"),TEXT("Deagle"),
  TEXT("MAC10"),TEXT("UMP45"),TEXT("MP5SD"),TEXT("MP7"),TEXT("P90"),
  TEXT("Nova"),TEXT("Negev"),TEXT("Grenade"),TEXT("Smoke"),TEXT("Fire"),TEXT("Flash"),TEXT("Decoy")};
 if(CustomIds.Contains(Id))
 {
  if(!ItemIconBrushes.Contains(Id))
  {
   const FString AssetName=FString::Printf(TEXT("T_UIWeapon_%s"),*Id.ToString());
   const FString Path=FString::Printf(TEXT("/Game/FieldMaker/Weapons/BlenderModeled/Icons/%s.%s"),*AssetName,*AssetName);
   if(UTexture2D* Texture=LoadObject<UTexture2D>(nullptr,*Path))
   {
    ItemIconTextures.Add(Texture);
    FSlateBrush Brush;Brush.SetResourceObject(Texture);Brush.ImageSize=FVector2D(768.f,512.f);Brush.DrawAs=ESlateBrushDrawType::Image;
    ItemIconBrushes.Add(Id,Brush);
   }
  }
  if(ItemIconBrushes.Contains(Id))
   return SNew(SBox).HeightOverride(44.f)
    [SNew(SScaleBox).Stretch(EStretch::ScaleToFit)
     [SNew(SImage).Image_Lambda([this,Id]() -> const FSlateBrush*
      {return ItemIconBrushes.Find(Id);})]];
 }
 const int32 TextureIndex = TextureIndexForIcon(Icon);
 if (!WeaponIconBrushes.IsValidIndex(TextureIndex)) return SNew(SEquipmentIcon).Item(Icon);
 return SNew(SBox).HeightOverride(44.f)
  [SNew(SScaleBox).Stretch(EStretch::ScaleToFit)
   [SNew(SImage).Image_Lambda([this, TextureIndex]() -> const FSlateBrush*
    { return WeaponIconBrushes.IsValidIndex(TextureIndex) ? &WeaponIconBrushes[TextureIndex] : nullptr; })]];
}
void UFMShopWidget::SelectItem(int32 Item)
{
 if(!Catalog.IsValidIndex(Item))return;
 Selection=Item;
 UpdateWeaponPreview();
}
void UFMShopWidget::SelectWeaponById(FName Id){const int32 I=Catalog.IndexOfByKey(Id);if(I!=INDEX_NONE)Selection=I;}
FText UFMShopWidget::SelectedName() const
{
 const auto Id=SelectedId();
 if (const auto* Item = AFMStructure::GetItem(AFMStructure::FindItem(Id))) return FText::FromString(Item->Name);
 if(IsArmor(Id))if(const auto* P=GetOwningPlayerPawn<AFMCharacter>())return P->GetCatalogArmorRules(Id).DisplayName;
 if(Id==TEXT("Flash"))return FText::FromString(TEXT("섬광탄"));
 if(Id==TEXT("Smoke"))return FText::FromString(TEXT("연막탄"));
 if(Id==TEXT("Fire"))return FText::FromString(TEXT("화염병 / 소이탄"));
 if(Id==TEXT("Decoy"))return FText::FromString(TEXT("유인 수류탄"));
 if(const auto* P=GetOwningPlayerPawn<AFMCharacter>())return P->GetCatalogWeaponData(Id).DisplayName;
 return FText::GetEmpty();
}
int32 UFMShopWidget::IconFor(FName Id) const
{
 if (AFMStructure::FindItem(Id) != INDEX_NONE) return 9;
 if(IsArmor(Id))return 4;
 if(Id==TEXT("Knife"))return 2;
 if(Id==TEXT("Grenade") || Id==TEXT("Flash") || Id==TEXT("Smoke") || Id==TEXT("Fire") || Id==TEXT("Decoy"))return 3;
 const auto* P=GetOwningPlayerPawn<AFMCharacter>();if(!P)return 0;
 switch(P->GetCatalogWeaponData(Id).Category){case EFMWeaponCategory::Pistol:return 1;case EFMWeaponCategory::SMG:return 5;case EFMWeaponCategory::Sniper:return 6;case EFMWeaponCategory::Shotgun:return 7;case EFMWeaponCategory::MachineGun:return 8;default:return 0;}
}
FText UFMShopWidget::DetailText() const
{
 const auto* P=GetOwningPlayerPawn<AFMCharacter>();if(!P)return FText::GetEmpty();
 const auto Id=SelectedId();
 const int32 Index = AFMStructure::FindItem(Id);
 if (const auto* Item = AFMStructure::GetItem(Index))
 {
  const auto* Member = P->GetPlayerState<AFMPlayerState>();
  return FText::FromString(FString::Printf(TEXT("내구도  %.0f\n가격  $%d\n보유  %d / 3\n\n구축 시간에 C로 모드를 켜고 %d 키로 선택한 뒤 좌클릭으로 배치합니다."), Item->Health, Item->Price,
   Member ? Member->GetStructureCount(Index) : 0, Index + 1));
 }
 if(IsArmor(Id))return FText::FromString(FString::Printf(TEXT("방어력  %.0f / %.0f\n총탄 피해 감소  %.0f%%\n\n머리·칼·폭발 피해는 우회합니다.\n흡수한 피해만큼 방어력이 소모됩니다."),P->GetArmor(),P->GetCatalogArmorRules(Id).MaximumArmor,P->GetCatalogArmorRules(Id).AbsorptionFraction*100));
 if(Id==TEXT("Flash") || Id==TEXT("Smoke") || Id==TEXT("Fire") || Id==TEXT("Decoy"))return FText::FromString(TEXT("카탈로그 예정 항목\n\n효과와 전용 투사체는 아직 구현되지 않았습니다."));
 if(Id==TEXT("Grenade"))return FText::FromString(TEXT("파편 수류탄\n신관  1.8초\n폭발 반경  4.5m\n중심 피해  85\n\n폭발 피해는 방탄복을 우회합니다."));
 const auto D=P->GetCatalogWeaponData(Id);
 const auto V=P->GetCatalogWeaponVisual(Id);
 const FString ModelNote=V.ModelLabel.IsEmpty()?TEXT("모델 준비 중"):V.ModelLabel.ToString()+(V.bRepresentative?TEXT(" (대체 외형)"):TEXT(""));
 return FText::FromString(FString::Printf(TEXT("피해  %.0f%s\n장탄 / 예비탄  %d / %d\n공격 간격  %.2f초\n재장전  %.2f초\n머리 / 팔다리  ×%.2g / ×%.2g\n\n%s\n\n%s\n기존 팔 동작 · 전용 모션 미연결"),D.Damage,D.PelletCount>1?*FString::Printf(TEXT(" × %d발"),D.PelletCount):TEXT(""),D.MagazineSize,D.ReserveAmmo,D.AttackInterval,D.ReloadTime,D.HeadMultiplier,D.LimbMultiplier,D.bAutomatic?TEXT("자동 사격"):TEXT("단발 사격"),*ModelNote));
}
bool UFMShopWidget::CanActivate() const
{
 const auto* P=GetOwningPlayerPawn<AFMCharacter>();const auto* G=GetWorld()->GetGameState<AFMGameState>();
 if (!P || P->GetHealth() <= 0 || !G) return false;
 const auto Phase = G->GetMatchSnapshot().Phase;
 if (Phase != EFMMatchPhase::CombatTest && Phase != EFMMatchPhase::Purchase) return false;
 const int32 Index = AFMStructure::FindItem(SelectedId());
 if (const auto* Item = AFMStructure::GetItem(Index))
 {
  const auto* Member = P->GetPlayerState<AFMPlayerState>();
  return Phase == EFMMatchPhase::Purchase && G->GetPhaseTimeRemaining() > 0.f && Member &&
   Member->GetCredits() >= Item->Price && Member->GetStructureCount(Index) < 3;
 }
 if(!P->GetCatalogWeaponVisual(SelectedId()).bAvailable)return false;
 return IconFor(SelectedId())!=3 || SelectedId()==TEXT("Grenade");
}
void UFMShopWidget::UpdateWeaponPreview()
{
 if(!GetWorld() || !Catalog.IsValidIndex(Selection)) return;
 const auto* Pawn=GetOwningPlayerPawn<AFMCharacter>();
 if(!Pawn) return;
 const FFMWeaponVisualRow Visual=Pawn->GetCatalogWeaponVisual(Catalog[Selection]);
 UStaticMesh* Mesh=Visual.Mesh.LoadSynchronous();
 if(!Mesh)
 {
  WeaponPreviewBrush.SetResourceObject(nullptr);
  return;
 }

 const FVector Origin(0,0,20000);
 FActorSpawnParameters Params;
 Params.ObjectFlags|=RF_Transient;
 Params.SpawnCollisionHandlingOverride=ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
 if(!WeaponPreviewModel)
 {
  WeaponPreviewModel=GetWorld()->SpawnActor<AStaticMeshActor>(Origin,FRotator::ZeroRotator,Params);
  if(!WeaponPreviewModel)return;
  WeaponPreviewModel->SetActorEnableCollision(false);
  WeaponPreviewModel->GetStaticMeshComponent()->SetMobility(EComponentMobility::Movable);
 }
 if(!WeaponPreviewCapture)
 {
  WeaponPreviewCapture=GetWorld()->SpawnActor<ASceneCapture2D>(Origin,FRotator::ZeroRotator,Params);
  if(!WeaponPreviewCapture)return;
  WeaponPreviewLight=GetWorld()->SpawnActor<APointLight>(Origin+FVector(150,-220,220),FRotator::ZeroRotator,Params);
  if(WeaponPreviewLight)
  {
   if(auto* Component=WeaponPreviewLight->FindComponentByClass<UPointLightComponent>())
   {
    Component->SetMobility(EComponentMobility::Movable);
    Component->SetAttenuationRadius(900.f);
    Component->SetIntensity(42000.f);
    Component->SetCastShadows(false);
   }
  }
  WeaponPreviewTarget=NewObject<UTextureRenderTarget2D>(this);
  WeaponPreviewTarget->InitAutoFormat(640,360);
  auto* Scene=WeaponPreviewCapture->GetCaptureComponent2D();
  Scene->TextureTarget=WeaponPreviewTarget;
  Scene->CaptureSource=SCS_FinalColorLDR;
  Scene->bCaptureEveryFrame=false;
  Scene->bCaptureOnMovement=false;
  Scene->PrimitiveRenderMode=ESceneCapturePrimitiveRenderMode::PRM_UseShowOnlyList;
  Scene->ShowOnlyActorComponents(WeaponPreviewModel);
  Scene->ShowFlags.SetAtmosphere(false);
  Scene->ShowFlags.SetFog(false);
  Scene->FOVAngle=38.f;
  Scene->PostProcessSettings.bOverride_AutoExposureMinBrightness=true;
  Scene->PostProcessSettings.bOverride_AutoExposureMaxBrightness=true;
  Scene->PostProcessSettings.AutoExposureMinBrightness=8.f;
  Scene->PostProcessSettings.AutoExposureMaxBrightness=8.f;
 }
 if(!WeaponPreviewTarget || !WeaponPreviewCapture)return;
 const FBoxSphereBounds Bounds=Mesh->GetBounds();
 const double Scale=105.0/FMath::Max(1.0,Bounds.BoxExtent.GetMax());
 WeaponPreviewModel->GetStaticMeshComponent()->SetStaticMesh(Mesh);
 WeaponPreviewModel->SetActorScale3D(FVector(Scale));
 WeaponPreviewModel->SetActorLocation(Origin-Bounds.Origin*Scale);
 const FVector Camera=Origin+FVector(115,-365,145);
 WeaponPreviewCapture->SetActorLocationAndRotation(Camera,(Origin-Camera).Rotation());
 WeaponPreviewBrush.SetImageSize(FVector2D(560,315));
 WeaponPreviewBrush.SetResourceObject(WeaponPreviewTarget);
 WeaponPreviewCapture->GetCaptureComponent2D()->CaptureScene();
}
FText UFMShopWidget::ActionText() const
{
 if(!CanActivate())return FText::FromString(TEXT("장착 불가 / 준비 중"));
 if (GetWorld()->GetGameState<AFMGameState>()->GetMatchSnapshot().Phase == EFMMatchPhase::Purchase)
  return FText::FromString(TEXT("구매"));
 if(IsArmor(SelectedId()))return FText::FromString(TEXT("테스트 착용 / 보충"));
 return FText::FromString(TEXT("테스트 장착"));
}
void UFMShopWidget::ActivateSelection()
{
 if(!CanActivate())return;
 if(auto* P=GetOwningPlayerPawn<AFMCharacter>())
 {
  const auto Id=SelectedId();
  const int32 Index = AFMStructure::FindItem(Id);
  if (Index != INDEX_NONE) P->ServerBuyStructure(Index);
  else if(IsArmor(Id))P->ServerEquipTestArmor(Id);
  else if(Id==TEXT("Knife"))P->ServerEquipWeapon(EFMWeaponSlot::Melee);
  else if (Id == TEXT("Grenade") && GetWorld()->GetGameState<AFMGameState>()->GetMatchSnapshot().Phase == EFMMatchPhase::CombatTest)
   P->ServerEquipWeapon(EFMWeaponSlot::Throwable);
  else P->ServerSelectCatalogWeapon(Id);
 }
}
FReply UFMShopWidget::NativeOnKeyDown(const FGeometry& G,const FKeyEvent& E)
{
 if(E.GetKey()==EKeys::B || E.GetKey()==EKeys::Escape)
 {
  if(!E.IsRepeat())if(auto* PC=GetOwningPlayer<AFMPlayerController>())PC->CloseShop();
  return FReply::Handled();
 }
 return Super::NativeOnKeyDown(G,E);
}
TSharedRef<SWidget> UFMShopWidget::RebuildWidget()
{
 SetIsFocusable(true);
 static const TCHAR* IconNames[] = {TEXT("Pistol"),TEXT("SMG"),TEXT("Rifle"),TEXT("Sniper"),TEXT("MachineGun"),TEXT("Shotgun"),TEXT("Knife"),TEXT("Grenade")};
 WeaponIconTextures.Reset();
 WeaponIconBrushes.Reset();
 ItemIconTextures.Reset();
 ItemIconBrushes.Reset();
 for (const TCHAR* Name : IconNames)
 {
  const FString Path = FString::Printf(TEXT("/Game/FieldMaker/UI/WeaponIcons/T_UIWeapon_%s.T_UIWeapon_%s"), Name, Name);
  UTexture2D* Texture = LoadObject<UTexture2D>(nullptr, *Path);
  WeaponIconTextures.Add(Texture);
  FSlateBrush Brush;
  Brush.SetResourceObject(Texture);
  Brush.ImageSize = FVector2D(180.f, 110.f);
  Brush.DrawAs = ESlateBrushDrawType::Image;
  WeaponIconBrushes.Add(Brush);
 }
 if(const auto* P=GetOwningPlayerPawn<AFMCharacter>())Catalog=P->GetCatalogWeaponIds();
 Catalog.Append({TEXT("LightArmor"),TEXT("MediumArmor"),TEXT("Flash"),TEXT("Smoke"),TEXT("Fire"),TEXT("Decoy")});
 if (const auto* State = GetWorld()->GetGameState<AFMGameState>(); State && State->GetMatchSnapshot().Phase == EFMMatchPhase::Purchase)
  Catalog.Append({TEXT("Sandbags"), TEXT("BallisticWall"), TEXT("StepCrate"), TEXT("BuildContainer")});
 SelectWeaponById(TEXT("M4"));
 UpdateWeaponPreview();
 CardStyle=FCoreStyle::Get().GetWidgetStyle<FButtonStyle>("Button");
 CardStyle.SetNormal(FSlateColorBrush(FLinearColor(.06f,.065f,.07f,.85f)));
 CardStyle.SetHovered(FSlateColorBrush(FLinearColor(.18f,.17f,.13f)));
 CardStyle.SetPressed(FSlateColorBrush(FLinearColor(.28f,.24f,.13f)));
 ActionStyle=CardStyle;ActionStyle.SetNormal(FSlateColorBrush(Mint));
 auto Columns=SNew(SHorizontalBox);
 const TCHAR* Headers[]={TEXT("장비"),TEXT("권총"),TEXT("기관단총"),TEXT("라이플"),TEXT("저격소총"),TEXT("중화기"),TEXT("수류탄"),TEXT("구조물")};
 const int32 ColumnCount = Catalog.Contains(TEXT("Sandbags")) ? UE_ARRAY_COUNT(Headers) : UE_ARRAY_COUNT(Headers) - 1;
 for(int32 Column=0;Column<ColumnCount;++Column)
 {
  auto Rows=SNew(SVerticalBox);
  for(int32 I=0;I<Catalog.Num();++I)
  {
   const FName Id=Catalog[I];const int32 Icon=IconFor(Id);
   const int32 Group=Icon==9?7:Icon==4 || Icon==2?0:Icon==1?1:Icon==5?2:Icon==0?3:Icon==6?4:Icon==7 || Icon==8?5:6;
   if(Group!=Column)continue;
   const auto* P=GetOwningPlayerPawn<AFMCharacter>();
   const auto* Structure = AFMStructure::GetItem(AFMStructure::FindItem(Id));
   const FText Name=Structure?FText::FromString(Structure->Name):IsArmor(Id)?P->GetCatalogArmorRules(Id).DisplayName:Id==TEXT("Flash")?FText::FromString(TEXT("섬광탄")):Id==TEXT("Smoke")?FText::FromString(TEXT("연막탄")):Id==TEXT("Fire")?FText::FromString(TEXT("화염병 / 소이탄")):Id==TEXT("Decoy")?FText::FromString(TEXT("유인 수류탄")):P->GetCatalogWeaponData(Id).DisplayName;
   const int32 Price=Structure?Structure->Price:IsArmor(Id)?P->GetCatalogArmorRules(Id).Price:P->GetCatalogWeaponData(Id).Price;
   const bool Pending=Icon==3 && Id!=TEXT("Grenade");
   Rows->AddSlot().AutoHeight().Padding(0,0,0,6)
    [SNew(SBox).HeightOverride(91)
     [SNew(SButton).ButtonStyle(&CardStyle).ContentPadding(0).Cursor(EMouseCursor::Hand)
      .OnHovered_Lambda([this,I]{SelectItem(I);})
      .OnClicked_Lambda([this,I]{SelectItem(I);ActivateSelection();return FReply::Handled();})
      [SNew(SBorder).BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush")).Padding(1)
       .BorderBackgroundColor_Lambda([this,I]{return Selection==I?Mint:FLinearColor(.22f,.23f,.24f);})
       [SNew(SBorder).BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush")).Padding(7)
        .BorderBackgroundColor_Lambda([this,I]{return Selection==I?FLinearColor(.14f,.14f,.12f):Ink;})
        [SNew(SVerticalBox)
         +SVerticalBox::Slot().AutoHeight()[Text(Name,12,Pending?Muted:Paper)]
         +SVerticalBox::Slot().FillHeight(1)[MakeItemImage(Id,Icon)]
         +SVerticalBox::Slot().AutoHeight()
          [SNew(SHorizontalBox)
           +SHorizontalBox::Slot().FillWidth(1)[Text(Pending?FText::FromString(TEXT("준비 중")):FText::FromString(FString::Printf(TEXT("$%d"),Price)),10,Pending?Muted:Mint)]
           +SHorizontalBox::Slot().AutoWidth()
            [SNew(STextBlock).Font(FCoreStyle::GetDefaultFontStyle("Regular",9)).ColorAndOpacity(Paper)
             .Text_Lambda([this,Id]{const auto* Pawn=GetOwningPlayerPawn<AFMCharacter>();return FText::FromString(Pawn && (Pawn->GetWeaponId(EFMWeaponSlot::Primary)==Id || Pawn->GetWeaponId(EFMWeaponSlot::Secondary)==Id)?TEXT("보유"):TEXT(""));})]]
        ]]]]];
  }
  Columns->AddSlot().FillWidth(1).Padding(0,0,7,0)
   [SNew(SVerticalBox)
    +SVerticalBox::Slot().AutoHeight().Padding(0,0,0,10)[Text(Headers[Column],14,Paper)]
    +SVerticalBox::Slot().FillHeight(1)[SNew(SScrollBox).ScrollBarThickness(FVector2D(3,3))+SScrollBox::Slot()[Rows]]];
 }
 auto Details=SNew(SVerticalBox)
  +SVerticalBox::Slot().AutoHeight()[Text(TEXT("FIELDMAKER  /  LOADOUT"),11,Mint)]
  +SVerticalBox::Slot().AutoHeight().Padding(0,8,0,10)
   [SNew(SBox).HeightOverride(205)
    [SNew(SBorder).BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush")).BorderBackgroundColor(FLinearColor(.06f,.08f,.10f))
     [SNew(SImage).Image_Lambda([this]() -> const FSlateBrush*
      {
       if(WeaponPreviewBrush.GetResourceObject())return &WeaponPreviewBrush;
       if(const FSlateBrush* ItemBrush=ItemIconBrushes.Find(SelectedId()))return ItemBrush;
       const int32 I=TextureIndexForIcon(IconFor(SelectedId()));
       return WeaponIconBrushes.IsValidIndex(I)?&WeaponIconBrushes[I]:FCoreStyle::Get().GetBrush("WhiteBrush");
      })]]]
  +SVerticalBox::Slot().AutoHeight().Padding(0,0,0,16)[SNew(STextBlock).Text_Lambda([this]{return SelectedName();}).Font(FCoreStyle::GetDefaultFontStyle("Bold",27)).ColorAndOpacity(Mint)]
  +SVerticalBox::Slot().FillHeight(1)[SNew(STextBlock).Text_Lambda([this]{return DetailText();}).Font(FCoreStyle::GetDefaultFontStyle("Regular",14)).ColorAndOpacity(Paper).AutoWrapText(true)]
  +SVerticalBox::Slot().AutoHeight().Padding(0,12,0,10)[SNew(STextBlock).ColorAndOpacity(Muted).Font(FCoreStyle::GetDefaultFontStyle("Regular",11))
   .Text_Lambda([this]
   {
    const auto* State = GetWorld()->GetGameState<AFMGameState>();
    return FText::FromString(State && State->GetMatchSnapshot().Phase == EFMMatchPhase::Purchase
     ? TEXT("구매 시 자금 차감 · 팀 스폰 구역에서만 구매")
     : TEXT("전투 테스트 · 자금 차감 없음"));
   })]
  +SVerticalBox::Slot().AutoHeight()
   [SNew(SButton).ButtonStyle(&ActionStyle).ContentPadding(13).IsEnabled_Lambda([this]{return CanActivate();})
    .OnClicked_Lambda([this]{ActivateSelection();return FReply::Handled();})
    [SNew(STextBlock).Text_Lambda([this]{return ActionText();}).Font(FCoreStyle::GetDefaultFontStyle("Bold",14)).ColorAndOpacity(Ink)]];
 return SNew(SBorder).BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush")).BorderBackgroundColor(FLinearColor(.012f,.014f,.016f,.92f)).Padding(24)
  [SNew(SScaleBox).Stretch(EStretch::ScaleToFit)
   [SNew(SBox).WidthOverride(1450).HeightOverride(800)
    [SNew(SVerticalBox)
     +SVerticalBox::Slot().AutoHeight().Padding(0,0,0,27)
      [SNew(SHorizontalBox)
       +SHorizontalBox::Slot().FillWidth(1)[Text(TEXT("장비 · 구조물 상점"),25,Mint)]
       +SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0,0,24,0)
        [SNew(STextBlock).ColorAndOpacity(Mint).Text_Lambda([this]
        {
         const auto* State = GetWorld()->GetGameState<AFMGameState>();
         const auto* P = GetOwningPlayerPawn<AFMCharacter>();
         const auto* Member = P ? P->GetPlayerState<AFMPlayerState>() : nullptr;
         return State && Member && State->GetMatchSnapshot().Phase == EFMMatchPhase::Purchase
          ? FText::FromString(FString::Printf(TEXT("$%d   |   %d초"), Member->GetCredits(), FMath::CeilToInt(State->GetPhaseTimeRemaining())))
          : FText::FromString(TEXT("테스트 카탈로그"));
        })]
       +SHorizontalBox::Slot().AutoWidth()[SNew(SButton).ButtonStyle(&CardStyle).ContentPadding(12).OnClicked_Lambda([this]{if(auto* PC=GetOwningPlayer<AFMPlayerController>())PC->CloseShop();return FReply::Handled();})[Text(TEXT("B / ESC   닫기"),13)]]]
     +SVerticalBox::Slot().FillHeight(1)
      [SNew(SHorizontalBox)
       +SHorizontalBox::Slot().FillWidth(3.8f)[Columns]
       +SHorizontalBox::Slot().FillWidth(1).Padding(20,0,0,0)[SNew(SBorder).BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush")).BorderBackgroundColor(FLinearColor(.045f,.048f,.05f,.95f)).Padding(20)[Details]]]
     +SVerticalBox::Slot().AutoHeight().Padding(0,20,0,0)[Text(TEXT("마우스를 올려 상세 보기  ·  클릭하여 구매 또는 테스트 장착  ·  각 열에서 스크롤  |  1 주무기 / 2 보조무기 / 3 칼 / 4 투척 슬롯 유지"),12,Muted)]
    ]]];
}

void UFMShopWidget::NativeDestruct()
{
 WeaponPreviewBrush.SetResourceObject(nullptr);
 if(WeaponPreviewCapture)WeaponPreviewCapture->Destroy();
 if(WeaponPreviewLight)WeaponPreviewLight->Destroy();
 if(WeaponPreviewModel)WeaponPreviewModel->Destroy();
 WeaponPreviewCapture=nullptr;
 WeaponPreviewLight=nullptr;
 WeaponPreviewModel=nullptr;
 WeaponPreviewTarget=nullptr;
 Super::NativeDestruct();
}
