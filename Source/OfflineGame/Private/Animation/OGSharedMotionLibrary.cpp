#include "Animation/OGSharedMotionLibrary.h"

#include "Animation/AnimSequence.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/SkeletalMesh.h"

namespace
{
void RegisterInstalled(FOGSharedMotionEntry& Entry, UOGSharedMotionLibrary* Library,
    const TCHAR* MotionId, const TCHAR* SourceId, const TCHAR* Action,
    const TCHAR* Context, const TCHAR* Wardrobe, const FString& NativePath,
    bool bLoop, const TCHAR* Capability)
{
    Entry.MotionId = FName(MotionId);
    Entry.SourceClipId = FName(SourceId);
    Entry.GameplayAction = FName(Action);
    Entry.GameplayContext = FName(Context);
    Entry.MotionCategory = FName(Context);
    Entry.CompatibleRigProfile = TEXT("koikatsu_original");
    Entry.AnatomyProfile = TEXT("humanoid");
    Entry.OutfitProfile = FName(Wardrobe);
    if (Capability && *Capability)
        Entry.RequiredCapabilities.Add(FName(Capability));
    Entry.NativeSequence = TSoftObjectPtr<UAnimSequence>(FSoftObjectPath(NativePath));
    Entry.bLoop = bLoop;
    Entry.bApprovedForGameplay = true; // Existing gameplay bindings; not a new artistic approval.
    Library->Motions.Add(MoveTemp(Entry));
}

void Install(UOGSharedMotionLibrary* Library)
{
    // Only source-backed native files already present in the production checkout.
    // Alter's wardrobe derivatives keep ONE canonical CMU source identity per
    // action. They never imply that the three skeletons are interchangeable.
    for (const TCHAR* Wardrobe : {TEXT("CLOTHED"), TEXT("UNDERWEAR"), TEXT("NUDE")})
    {
        for (const TCHAR* Action : {TEXT("Walk"), TEXT("Run"), TEXT("Jump")})
        {
            const FString Stem = FString::Printf(
                TEXT("AN_G12_Alter_%s_RealCMU_%s"), Wardrobe, Action);
            const FString Asset = FString::Printf(
                TEXT("/Game/NoWorld/Characters/Luminous/Alter/NativeMocapRealCMU/%s.%s"),
                *Stem, *Stem);
            const FString Semantic = FString::Printf(TEXT("Traversal.%s"), Action);
            const FString Identifier = FString::Printf(
                TEXT("cmu.alter.%s.%s"), Wardrobe, Action);
            const FString Origin = FString::Printf(TEXT("cmu.humanoid.%s"), Action);
            FOGSharedMotionEntry Entry;
            RegisterInstalled(Entry, Library, *Identifier, *Origin, *Semantic,
                TEXT("Traversal"), Wardrobe, Asset, FCString::Strcmp(Action, TEXT("Jump")) != 0,
                TEXT("grounded"));
        }
    }

    const FString Root = TEXT("/Game/Gate12NativeKoikatsu20261020/FlightDanceSwimming/");
    for (const auto& Motion : {
        TPair<const TCHAR*, const TCHAR*>(TEXT("Flight_Hover"), TEXT("Traversal.FlightHover")),
        TPair<const TCHAR*, const TCHAR*>(TEXT("Flight_Start"), TEXT("Traversal.FlightTakeoff")),
        TPair<const TCHAR*, const TCHAR*>(TEXT("Swimming_Stroke"), TEXT("Traversal.Swim"))})
    {
        const FString Name = FString::Printf(TEXT("G12_LuminousV3_Koikatsu_%s"), Motion.Key);
        const bool bSwimming = FCString::Strcmp(Motion.Key, TEXT("Swimming_Stroke")) == 0;
        const bool bTakeoff = FCString::Strcmp(Motion.Key, TEXT("Flight_Start")) == 0;
        FOGSharedMotionEntry Entry;
        const FString Identifier = FString::Printf(TEXT("kk.base.%s"), Motion.Key);
        const FString Origin = FString::Printf(TEXT("koikatsu.%s"), Motion.Key);
        RegisterInstalled(Entry, Library, *Identifier, *Origin, Motion.Value,
            TEXT("Traversal"), TEXT("CLOTHED"), Root + Name + TEXT(".") + Name,
            !bTakeoff, bSwimming ? TEXT("swimming") : TEXT("flying"));
    }

    struct FEmoteAsset { const TCHAR* Semantic; const TCHAR* Stem; const TCHAR* Folder; };
    const FEmoteAsset Emotes[] = {
        {TEXT("Emote.Thoughtful"), TEXT("G09_LuminousV3_Idle_01_Thoughtful"), TEXT("TestSingle")},
        {TEXT("Emote.Greeting"), TEXT("G09_LuminousV3_Idle_02_Greeting"), TEXT("NativeKoikatsu")},
        {TEXT("Emote.Expressive"), TEXT("G09_LuminousV3_Idle_03_Expressive"), TEXT("NativeKoikatsu")},
        {TEXT("Emote.Bow"), TEXT("G09_LuminousV3_Emote_Bow_Source"), TEXT("NativeKoikatsu")}
    };
    for (const FEmoteAsset& Asset : Emotes)
    {
        const FString Path = FString::Printf(
            TEXT("/Game/Gate09_QA_20261019/%s/%s.%s"),
            Asset.Folder, Asset.Stem, Asset.Stem);
        FOGSharedMotionEntry Entry;
        const FString Id = FString::Printf(TEXT("kk.base.%s"), Asset.Stem);
        RegisterInstalled(Entry, Library, *Id, *Id, Asset.Semantic,
            TEXT("Interaction"), TEXT("CLOTHED"), Path, false, TEXT("emote"));
    }

    // The rejected dance and unconverted source-only H clips are intentionally
    // not claimed as usable gameplay assets. Eligible future adult assets
    // require explicit approval, privacy gating and a real native sequence.
}
}

const UOGSharedMotionLibrary* FOGSharedMotionResolver::GetInstalledLibrary()
{
    // Non-destructive packaged fallback. A verified authored shared DataAsset
    // can override it; never depend on an Editor-only registry at runtime.
    static UOGSharedMotionLibrary* Installed = nullptr;
    if (!Installed)
    {
        Installed = NewObject<UOGSharedMotionLibrary>(GetTransientPackage());
        Installed->AddToRoot();
        Install(Installed);
    }
    return Installed;
}

UAnimSequence* FOGSharedMotionResolver::Resolve(
    const UOGSharedMotionLibrary* Library,
    FName Action, FName RigProfile, FName AnatomyProfile,
    FName OutfitProfile, FName Context,
    const TArray<FName>& Capabilities,
    const TArray<TObjectPtr<USkeletalMeshComponent>>& ActiveParts,
    bool bAdultContextAuthorized, int32 ParticipantCount,
    const FOGSharedMotionEntry** OutEntry)
{
    if (OutEntry) *OutEntry = nullptr;
    if (!Library || Action.IsNone() || RigProfile.IsNone() ||
        ActiveParts.IsEmpty() || ParticipantCount < 1 || ParticipantCount > 13)
        return nullptr;

    // Rank only source-approved, contextual, exact-rig candidates. Never
    // assign a clip from another character based on display name or form.
    TArray<const FOGSharedMotionEntry*> Candidates;
    for (const FOGSharedMotionEntry& Entry : Library->Motions)
    {
        if (!Entry.bApprovedForGameplay || Entry.MotionId.IsNone() ||
            Entry.SourceClipId.IsNone() || Entry.NativeSequence.IsNull() ||
            Entry.GameplayAction != Action ||
            Entry.CompatibleRigProfile != RigProfile ||
            (!Entry.AnatomyProfile.IsNone() && Entry.AnatomyProfile != AnatomyProfile) ||
            (!Entry.OutfitProfile.IsNone() && Entry.OutfitProfile != OutfitProfile) ||
            (!Entry.GameplayContext.IsNone() && Entry.GameplayContext != Context) ||
            (Entry.bAdultRestricted && !bAdultContextAuthorized) ||
            Entry.RequiredParticipants != ParticipantCount)
            continue;

        bool bMissingCapability = false;
        for (FName Required : Entry.RequiredCapabilities)
        {
            if (!Capabilities.Contains(Required))
            {
                bMissingCapability = true;
                break;
            }
        }
        if (!bMissingCapability) Candidates.Add(&Entry);
    }

    Candidates.Sort([](const FOGSharedMotionEntry& A, const FOGSharedMotionEntry& B)
    {
        // Explicit wardrobe derivatives precede genuine wardrobe-neutral
        // clips. Priority is authored, ties are stable by persistent MotionId.
        const int32 ScoreA = A.Priority * 4 + (!A.OutfitProfile.IsNone() ? 2 : 0) +
            (!A.GameplayContext.IsNone() ? 1 : 0);
        const int32 ScoreB = B.Priority * 4 + (!B.OutfitProfile.IsNone() ? 2 : 0) +
            (!B.GameplayContext.IsNone() ? 1 : 0);
        if (ScoreA != ScoreB) return ScoreA > ScoreB;
        return A.MotionId.LexicalLess(B.MotionId);
    });

    for (const FOGSharedMotionEntry* Entry : Candidates)
    {
        UAnimSequence* Sequence = Entry->NativeSequence.LoadSynchronous();
        if (!Sequence || !Sequence->GetSkeleton())
            continue;
        for (const TObjectPtr<USkeletalMeshComponent>& Part : ActiveParts)
        {
            const USkeletalMesh* Mesh = Part ? Part->GetSkeletalMeshAsset() : nullptr;
            if (Mesh && Mesh->GetSkeleton() == Sequence->GetSkeleton())
            {
                if (OutEntry) *OutEntry = Entry;
                return Sequence;
            }
        }
    }
    return nullptr; // Never synthesize or silently substitute incorrect motion.
}
