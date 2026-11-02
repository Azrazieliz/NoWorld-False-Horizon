#include "Animation/OGSharedReferenceAnimInstance.h"
#include "Animation/AnimInstanceProxy.h"
#include "Animation/AnimNodeBase.h"

namespace
{
class FOGReferencePoseProxy final : public FAnimInstanceProxy
{
public:
    explicit FOGReferencePoseProxy(UAnimInstance* Instance) : FAnimInstanceProxy(Instance) {}
    virtual void PreUpdate(UAnimInstance* Instance, float DeltaSeconds) override
    {
        FAnimInstanceProxy::PreUpdate(Instance, DeltaSeconds);
        const auto* Shared = Cast<UOGSharedReferenceAnimInstance>(Instance);
        Pose = Shared ? Shared->GetSharedPose() : TArray<FTransform>();
    }
    virtual bool Evaluate(FPoseContext& Output) override
    {
        Output.ResetToRefPose();
        const FBoneContainer& Required = Output.Pose.GetBoneContainer();
        for (FCompactPoseBoneIndex Bone : Output.Pose.ForEachBoneIndex())
        {
            const int32 Index = Required.MakeMeshPoseIndex(Bone).GetInt();
            if (Pose.IsValidIndex(Index)) Output.Pose[Bone] = Pose[Index];
        }
        return true;
    }
private:
    TArray<FTransform> Pose;
};
}

void UOGSharedReferenceAnimInstance::SetSharedPose(const TArray<FTransform>& InPose)
{
    check(IsInGameThread());
    PendingPose = InPose;
}
FAnimInstanceProxy* UOGSharedReferenceAnimInstance::CreateAnimInstanceProxy()
{
    return new FOGReferencePoseProxy(this);
}
void UOGSharedReferenceAnimInstance::DestroyAnimInstanceProxy(FAnimInstanceProxy* InProxy)
{
    delete InProxy;
}
