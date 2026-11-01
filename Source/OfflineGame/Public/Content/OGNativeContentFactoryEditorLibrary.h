#pragma once
#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "OGNativeContentFactoryEditorLibrary.generated.h"

/** Source-authentic skeletal parts -> one real native appearance UAsset.
 * Editorial authoring API; NEVER an Android gameplay authority or a character
 * grant/Registry mutation. The source bank is shared, not per-Version. */
UCLASS()
class OFFLINEGAME_API UOGNativeContentFactoryEditorLibrary final : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()
public:
    UFUNCTION(BlueprintCallable, Category="NoWorld|5Star|SourceAuthoring")
    static bool AuthorNativeAppearance(
        const FString& IdentityId,
        const FString& VersionId,
        const FString& LongAssetPath,
        const FString& SharedMotionLibraryPath,
        const TArray<FString>& RealSkeletalMeshPaths,
        const FString& WardrobeId,
        const FString& RigProfile,
        const FString& AnatomyProfile,
        const FString& WardrobeStateField,
        const TArray<FString>& Capabilities);
};
