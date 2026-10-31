#include "Koikatsu/OGLuminousExactOriginalPreviewActor.h"
#include "Koikatsu/OGExactOriginalLuminousParts.h"
#include "Animation/OGSharedMotionControllerComponent.h"
#include "Components/SceneComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/GameInstance.h"
#include "Runtime/OGGameCoreSubsystem.h"
#include "Persistence/OGWorldStore.h"
#include "World/OGWorldStateRecords.h"

AOGLuminousExactOriginalPreviewActor::AOGLuminousExactOriginalPreviewActor()
{
    PrimaryActorTick.bCanEverTick = false;
    NativeRoot = CreateDefaultSubobject<USceneComponent>(TEXT("OriginalModelRoot"));
    RootComponent = NativeRoot;
    Motion = CreateDefaultSubobject<UOGSharedMotionControllerComponent>(
        TEXT("WholeCharacterSharedMotion"));
    Motion->RigProfile = TEXT("koikatsu_original");
    Motion->AnatomyProfile = TEXT("humanoid");
    Motion->bGroupPartsByReferencePose = true;
    Motion->bUseIdenticalTopologyLeaderPose = false;
    SetActorEnableCollision(false);
}

bool AOGLuminousExactOriginalPreviewActor::ConfigureOwnedPreview_Implementation(
    const FOGEntityId& RulerId, const FOGContentId& IdentityId,
    const FOGContentId& VersionId, const FOGEntityId& ManifestationId,
    bool bSfwPresentation)
{
    // All currently imported sources are genuinely clothed, so both SFW
    // and normal presentation may render them. No outfit or save writes.
    (void)bSfwPresentation;
    static const FOGContentId LuminousIdentity(
        TEXT("character:luminous_valentine.identity"));
    if (!RulerId.IsValid() || !ManifestationId.IsValid() ||
        IdentityId != LuminousIdentity || !Motion || !Parts.IsEmpty())
        return false;
    UGameInstance* GI = GetGameInstance();
    UOGGameCoreSubsystem* Core = GI
        ? GI->GetSubsystem<UOGGameCoreSubsystem>() : nullptr;
    IOGWorldStore* Store = Core ? Core->GetWorldStore() : nullptr;
    if (!Core || !Core->IsCoreReady() || !Store || !Store->IsOpen())
        return false;
    FOGCharacterManifestationRecord Owned;
    bool bFound = false;
    FString Error;
    if (!Store->TryReadCharacterManifestation(
            ManifestationId, bFound, Owned, Error) ||
        !bFound || Owned.OwningRulerId != RulerId ||
        Owned.IdentityId != IdentityId ||
        Owned.ActiveVersionId != VersionId ||
        Owned.LifecycleState != FName(TEXT("active")))
        return false;

    TArray<FSoftObjectPath> Paths;
    int32 SourceBoneCount = 0;
    if (!FOGExactOriginalLuminousParts::GetClothed(
            VersionId.ToString(), Paths, SourceBoneCount) ||
        Paths.Num() != 9 || SourceBoneCount < 1 ||
        !Motion->BindCanonicalCharacter(IdentityId))
        return false;

    TArray<USkeletalMesh*> Meshes;
    int32 WholeRigBones = INDEX_NONE;
    for (const FSoftObjectPath& Path : Paths)
    {
        USkeletalMesh* Mesh = Cast<USkeletalMesh>(Path.TryLoad());
        const int32 Bones = Mesh ? Mesh->GetRefSkeleton().GetNum() : 0;
        if (!Mesh || !Mesh->GetSkeleton() ||
            Bones < SourceBoneCount || Bones > SourceBoneCount + 1 ||
            (WholeRigBones != INDEX_NONE && WholeRigBones != Bones))
            return false;
        WholeRigBones = Bones;
        Meshes.Add(Mesh);
    }

    TArray<USkeletalMeshComponent*> Components;
    for (int32 Index = 0; Index < Meshes.Num(); ++Index)
    {
        USkeletalMeshComponent* Part =
            NewObject<USkeletalMeshComponent>(this);
        if (!Part) return false;
        Part->SetupAttachment(NativeRoot);
        Part->SetRelativeLocation(FVector(0.f, 0.f, -96.f));
        Part->SetRelativeRotation(FRotator(0.f, -90.f, 0.f));
        Part->SetSkeletalMeshAsset(Meshes[Index]);
        Part->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        Part->SetCastShadow(true);
        AddInstanceComponent(Part);
        Part->RegisterComponent();
        Parts.Add(Part);
        Components.Add(Part);
    }
    Motion->SetCurrentOutfit(TEXT("CLOTHED"));
    if (!Motion->RegisterNativeParts(Components))
    {
        for (USkeletalMeshComponent* Part : Parts)
            if (Part) Part->DestroyComponent();
        Parts.Reset();
        return false;
    }
    UE_LOG(LogTemp, Display,
        TEXT("G12_EXACT_ORIGINAL_LUMINOUS_PREVIEW=PASS version=%s parts=%d source_bones=%d native_bones=%d"),
        *VersionId.ToString(), Parts.Num(), SourceBoneCount, WholeRigBones);
    return true;
}

void AOGLuminousExactOriginalPreviewActor::EndPlay(
    const EEndPlayReason::Type Reason)
{
    if (Motion) Motion->StopAction();
    Parts.Reset();
    Super::EndPlay(Reason);
}
