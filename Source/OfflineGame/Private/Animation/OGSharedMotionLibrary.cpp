#include "Animation/OGSharedMotionLibrary.h"

#include "Animation/AnimSequence.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/SkeletalMesh.h"
#include "Dom/JsonObject.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"

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

// Packaged, editable shared catalog. New converted source derivatives can be
// registered here without adding per-Version hardcoded C++ bindings. The
// built-in registrations above remain a recovery fallback for this checkout.
void ReadPackagedCatalog(UOGSharedMotionLibrary* Library)
{
    const FString ManifestPath = FPaths::Combine(FPaths::ProjectContentDir(),
        TEXT("NoWorld/Animation/SharedMotionCatalog.json"));
    FString Source;
    if (!FFileHelper::LoadFileToString(Source, *ManifestPath)) return;
    TSharedPtr<FJsonObject> Root;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Source);
    if (!FJsonSerializer::Deserialize(Reader, Root) || !Root.IsValid()) return;
    FString Schema;
    if (!Root->TryGetStringField(TEXT("schema"), Schema) ||
        Schema != TEXT("noworld.shared_motion_catalog.v1")) return;
    const TArray<TSharedPtr<FJsonValue>>* Items = nullptr;
    if (!Root->TryGetArrayField(TEXT("motions"), Items) || !Items) return;

    for (const TSharedPtr<FJsonValue>& ItemValue : *Items)
    {
        const TSharedPtr<FJsonObject> Item = ItemValue.IsValid()
            ? ItemValue->AsObject() : nullptr;
        if (!Item.IsValid()) continue;
        const auto ReadName = [&Item](const TCHAR* Key) -> FName
        {
            FString Value;
            return Item->TryGetStringField(Key, Value) ? FName(*Value) : NAME_None;
        };
        FOGSharedMotionEntry Entry;
        Entry.MotionId = ReadName(TEXT("motion_id"));
        Entry.SourceClipId = ReadName(TEXT("source_clip_id"));
        Entry.GameplayAction = ReadName(TEXT("action"));
        Entry.MotionCategory = ReadName(TEXT("category"));
        Entry.GameplayContext = ReadName(TEXT("context"));
        Entry.CompatibleRigProfile = ReadName(TEXT("rig"));
        Entry.AnatomyProfile = ReadName(TEXT("anatomy"));
        Entry.OutfitProfile = ReadName(TEXT("outfit"));
        Entry.RetargetProfile = ReadName(TEXT("retarget"));
        Entry.ContactNotify = ReadName(TEXT("contact_notify"));
        Item->TryGetStringField(TEXT("source_hash"), Entry.SourceObjectHash);
        Item->TryGetStringField(TEXT("source_bundle"), Entry.SourceUnityBundle);
        FString Native;
        Item->TryGetStringField(TEXT("native_sequence"), Native);
        if (Entry.MotionId.IsNone() || Entry.SourceClipId.IsNone() ||
            Entry.GameplayAction.IsNone() || Entry.GameplayContext.IsNone() ||
            Entry.CompatibleRigProfile.IsNone() ||
            !Native.StartsWith(TEXT("/Game/")) || !Native.Contains(TEXT(".")) ||
            Native.Contains(TEXT(".."))) continue;
        Entry.NativeSequence = TSoftObjectPtr<UAnimSequence>(FSoftObjectPath(Native));
        const TArray<TSharedPtr<FJsonValue>>* Caps = nullptr;
        if (Item->TryGetArrayField(TEXT("capabilities"), Caps) && Caps)
            for (const TSharedPtr<FJsonValue>& V : *Caps)
                if (V.IsValid() && V->Type == EJson::String)
                    Entry.RequiredCapabilities.Add(FName(*V->AsString()));
        Item->TryGetBoolField(TEXT("approved_for_gameplay"), Entry.bApprovedForGameplay);
        Item->TryGetBoolField(TEXT("adult_restricted"), Entry.bAdultRestricted);
        Item->TryGetBoolField(TEXT("loop"), Entry.bLoop);
        Item->TryGetBoolField(TEXT("root_motion"), Entry.bUsesRootMotion);
        double Number = 0.;
        if (Item->TryGetNumberField(TEXT("participants"), Number))
            Entry.RequiredParticipants = FMath::Clamp(FMath::RoundToInt(Number), 1, 13);
        if (Item->TryGetNumberField(TEXT("priority"), Number))
            Entry.Priority = FMath::RoundToInt(Number);
        if (Item->TryGetNumberField(TEXT("play_rate"), Number))
            Entry.PlayRate = FMath::Clamp(static_cast<float>(Number), 0.01f, 8.f);
        if (Item->TryGetNumberField(TEXT("contact_seconds"), Number))
            Entry.ContactSeconds = static_cast<float>(Number);
        if (Item->TryGetNumberField(TEXT("movement_speed_cm_s"), Number))
            Entry.MovementSpeedCmPerSecond = static_cast<float>(Number);

        // Manifest approvals replace precisely the matching installed entry.
        // De-approved definitions suppress the fallback instead of reviving it.
        Library->Motions.RemoveAll([Id = Entry.MotionId](const FOGSharedMotionEntry& Existing)
            { return Existing.MotionId == Id; });
        if (Entry.bApprovedForGameplay) Library->Motions.Add(MoveTemp(Entry));
    }
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
        ReadPackagedCatalog(Installed);
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
