#pragma once
#include "CoreMinimal.h"
#include "Content/OGDefinitionLibrary.h"
#include "Gacha/OGApprovedGachaBannerCatalog.h"
#include "OGSharedGachaBannerDefinition.generated.h"

/** Reusable acquisition definition; ownership/payment/pity remain in the existing Gacha service. */
UCLASS(BlueprintType)
class OFFLINEGAME_API UOGSharedGachaBannerDefinition : public UOGReusableDefinition
{
    GENERATED_BODY()
public:
    UOGSharedGachaBannerDefinition();
    UPROPERTY(EditAnywhere,BlueprintReadOnly) FText DisplayName;
    UPROPERTY(EditAnywhere,BlueprintReadOnly) FOGInstalledScheduledGachaBanner Schedule;
    virtual bool ValidateDefinition_Implementation(FString& Error) const override;
};
/** Collect current approved source schedules from shared bundles and the original installed catalog. */
class OFFLINEGAME_API FOGInstalledGachaBannerLibrary
{
public:
    static bool Collect(int64 WorldTick,TArray<FOGGachaBannerDefinition>& Out,FString& Error);
    static bool FilterSchedules(const TArray<FOGInstalledScheduledGachaBanner>& Schedules,int64 WorldTick,
        TArray<FOGGachaBannerDefinition>& Out,FString& Error);
    static FString DisplayName(const FOGContentId& BannerId);
};
