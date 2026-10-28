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

    // Rejected choreography and QA-only source clips remain unapproved for
    // GAMEPLAY solely for technical/artistic reasons. Mature content is a
    // production category, not a separate invented action-eligibility gate;
    // existing SFW/privacy handling belongs to presentation, not this library.
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
        Entry.AuthoredSceneId = ReadName(TEXT("scene_id"));
        Entry.SceneRole = ReadName(TEXT("scene_role"));
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
        // Legacy `adult_restricted` describes source content; it is NOT an
        // independently invented gameplay authorization rule. Honor optional
        // newer content metadata without changing the underlying source.
        Item->TryGetBoolField(TEXT("adult_restricted"), Entry.bAdultRestricted);
        Entry.bContainsMatureContent = Entry.bAdultRestricted;
        Item->TryGetBoolField(TEXT("mature_content"), Entry.bContainsMatureContent);
        Item->TryGetBoolField(TEXT("loop"), Entry.bLoop);
        Item->TryGetBoolField(TEXT("root_motion"), Entry.bUsesRootMotion);
        double Number = 0.;
        if (Item->TryGetNumberField(TEXT("participants"), Number))
        {
            if (!FMath::IsFinite(Number) || Number < 1 || Number > MAX_int32 ||
                FMath::FloorToDouble(Number) != Number) continue;
            Entry.RequiredParticipants = static_cast<int32>(Number);
        }
        if (Item->TryGetNumberField(TEXT("priority"), Number))
            Entry.Priority = FMath::RoundToInt(Number);
        if (Item->TryGetNumberField(TEXT("play_rate"), Number))
            Entry.PlayRate = FMath::Clamp(static_cast<float>(Number), 0.01f, 8.f);
        if (Item->TryGetNumberField(TEXT("contact_seconds"), Number))
            Entry.ContactSeconds = static_cast<float>(Number);
        if (Item->TryGetNumberField(TEXT("movement_speed_cm_s"), Number))
            Entry.MovementSpeedCmPerSecond = static_cast<float>(Number);

        // Optional authored morph curves survive cooking as pure data. They
        // are evaluated alongside the skeletal track without an AnimBP swap.
        const TArray<TSharedPtr<FJsonValue>>* TrackValues = nullptr;
        if (Item->TryGetArrayField(TEXT("morph_tracks"), TrackValues) && TrackValues)
        for (const TSharedPtr<FJsonValue>& TrackValue : *TrackValues)
        {
            const TSharedPtr<FJsonObject> TrackData =
                TrackValue.IsValid() ? TrackValue->AsObject() : nullptr;
            if (!TrackData.IsValid()) continue;
            FOGSharedMorphTrack Track;
            FString Name;
            if (TrackData->TryGetStringField(TEXT("layer"), Name))
                Track.Layer = FName(*Name);
            if (TrackData->TryGetStringField(TEXT("morph"), Name))
                Track.Morph = FName(*Name);
            if (Track.Layer.IsNone() || Track.Morph.IsNone()) continue;
            const TArray<TSharedPtr<FJsonValue>>* Keys = nullptr;
            if (!TrackData->TryGetArrayField(TEXT("keys"), Keys) || !Keys) continue;
            bool bValid = true;
            float PreviousTime = -1.f;
            for (const TSharedPtr<FJsonValue>& KeyValue : *Keys)
            {
                const TSharedPtr<FJsonObject> Key =
                    KeyValue.IsValid() ? KeyValue->AsObject() : nullptr;
                double Time = 0., Weight = 0.;
                if (!Key.IsValid() ||
                    !Key->TryGetNumberField(TEXT("time"), Time) ||
                    !Key->TryGetNumberField(TEXT("weight"), Weight) ||
                    !FMath::IsFinite(Time) || !FMath::IsFinite(Weight) ||
                    Time < 0. || Time <= PreviousTime)
                { bValid = false; break; }
                FOGSharedMorphKey Frame;
                Frame.TimeSeconds = static_cast<float>(Time);
                Frame.Weight = FMath::Clamp(static_cast<float>(Weight), -1.f, 1.f);
                Track.Keys.Add(Frame);
                PreviousTime = Frame.TimeSeconds;
            }
            if (bValid && !Track.Keys.IsEmpty())
                Entry.MorphTracks.Add(MoveTemp(Track));
        }

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
    const FOGSharedMotionEntry** OutEntry, FName SceneRole)
{
    if (OutEntry) *OutEntry = nullptr;
    if (!Library || Action.IsNone() || RigProfile.IsNone() ||
        ActiveParts.IsEmpty() || ParticipantCount < 1)
        return nullptr;

    // Gameplay authority has already resolved the action before presentation.
    // Keep this legacy parameter for Blueprint/native call-site compatibility;
    // do NOT make an additional blanket adult-content permission decision here.
    (void)bAdultContextAuthorized;
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
            // Wardrobe is a preference, not a separate authored motion library.
            // Cross-wardrobe derivatives are allowed ONLY if the actual live
            // skeletal component matches the animation's native Skeleton.
            (!Entry.GameplayContext.IsNone() && Entry.GameplayContext != Context) ||
            Entry.RequiredParticipants != ParticipantCount ||
            (ParticipantCount > 1
                ? (SceneRole.IsNone() || Entry.SceneRole != SceneRole ||
                   Entry.AuthoredSceneId.IsNone())
                : !Entry.SceneRole.IsNone()))
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
        // Retain authored priority and deterministic source ordering. Exact
        // outfit match is ranked separately in Resolve, before skeleton load.
        const int32 ScoreA = A.Priority * 4 +
            (!A.GameplayContext.IsNone() ? 1 : 0);
        const int32 ScoreB = B.Priority * 4 +
            (!B.GameplayContext.IsNone() ? 1 : 0);
        if (ScoreA != ScoreB) return ScoreA > ScoreB;
        return A.MotionId.LexicalLess(B.MotionId);
    });

    // Prefer exact wardrobe derivative, then wardrobe-neutral clip, then
    // another wardrobe's derivative ONLY when the live skeleton is identical.
    // This eliminates per-wardrobe motion authoring for compatible actors,
    // without substituting animation on the basis of a display name.
    for (int32 OutfitPass = 0; OutfitPass < 3; ++OutfitPass)
    {
        for (const FOGSharedMotionEntry* Entry : Candidates)
        {
            const int32 Class = Entry->OutfitProfile == OutfitProfile ? 0 :
                (Entry->OutfitProfile.IsNone() ? 1 : 2);
            if (Class != OutfitPass) continue;
            UAnimSequence* Sequence = Entry->NativeSequence.LoadSynchronous();
            if (!Sequence || !Sequence->GetSkeleton()) continue;
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
    }
    return nullptr; // Never synthesize or silently substitute incorrect motion.
}
