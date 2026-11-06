#pragma once

#include "GameFramework/SaveGame.h"
#include "FMAppearanceSaveGame.generated.h"

namespace FMAppearance
{
 enum EPart : uint32 { Cap=1,Vest=2,Drops=4,Holster=8,Patch=16 };
 constexpr uint32 Default=Cap|Vest|Drops|Holster|Patch;
 inline int32 StyleShift(int32 Category){return Category==0?5:Category==2?7:Category==6?9:Category==3?11:-1;}
 inline int32 StyleCount(int32 Category){return Category==0 || Category==3?3:Category==2 || Category==6?4:0;}
 inline uint32 WithStyle(uint32 Id,int32 Category,int32 Style){const int32 Shift=StyleShift(Category);return Shift<0 || Style<0 || Style>=StyleCount(Category)?Id:(Id & ~(3u<<Shift)) | (uint32(Style)<<Shift);}
 inline int32 GetStyle(uint32 Id,int32 Category){const int32 Shift=StyleShift(Category);return Shift<0?0:int32((Id>>Shift)&3u);}
 inline bool IsValid(uint32 Id){return (Id & ~8191u)==0 && (!(Id & Patch) || (Id & Vest)) && GetStyle(Id,0)<3 && GetStyle(Id,3)<3;}
}

UCLASS()
class UFMAppearanceSaveGame : public USaveGame
{
	GENERATED_BODY()

public:
	static uint32 Load();
	static bool Save(uint32 Id);

private:
	UPROPERTY(SaveGame)
	uint8 AppearanceId = uint8(FMAppearance::Default);
 UPROPERTY(SaveGame) uint32 AppearanceCode = FMAppearance::Default;
 UPROPERTY(SaveGame) bool bHasAppearanceCode = false;
};
