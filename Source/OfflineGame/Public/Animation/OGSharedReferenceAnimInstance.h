#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "OGSharedReferenceAnimInstance.generated.h"

struct FAnimInstanceProxy;

/** Native character-side connection to the shared animation engine. */
UCLASS(Transient, NotBlueprintable)
class OFFLINEGAME_API UOGSharedReferenceAnimInstance : public UAnimInstance
{
    GENERATED_BODY()
public:
    void SetSharedPose(const TArray<FTransform>& InPose);
    const TArray<FTransform>& GetSharedPose() const { return PendingPose; }
protected:
    virtual FAnimInstanceProxy* CreateAnimInstanceProxy() override;
    virtual void DestroyAnimInstanceProxy(FAnimInstanceProxy* InProxy) override;
private:
    TArray<FTransform> PendingPose;
};
