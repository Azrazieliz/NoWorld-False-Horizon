#include "Animation/OGSharedReferenceMotion.h"

#include "Components/SkeletalMeshComponent.h"
#include "Engine/SkeletalMesh.h"
#include "Dom/JsonObject.h"
#include "HAL/PlatformFileManager.h"
#include "Misc/Compression.h"
#include "Misc/Crc.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"

namespace
{
bool Fail(FString& Error, const TCHAR* Message) { Error = Message; return false; }
FString LibraryRoot() { return FPaths::ProjectContentDir() / TEXT("NoWorld/Animation/SharedSource"); }
struct FCachedReference
{
    TSharedPtr<const FOGReferenceMotionClip, ESPMode::ThreadSafe> Clip;
    uint64 Used = 0;
};
struct FReferenceRegistry
{
    bool bLoaded = false;
    uint64 Clock = 0;
    TMap<FName, FString> Paths;
    TArray<FOGSharedMotionEntry> Routes;
    TMap<FName, TSet<FName>> Aliases;
    TMap<FName, FCachedReference> Cache;
    void Ensure()
    {
        if (bLoaded) return;
        bLoaded = true;
        FString Text;
        TSharedPtr<FJsonObject> Root;
        if (!FFileHelper::LoadFileToString(Text, *(LibraryRoot() / TEXT("SharedReferenceLibrary.json"))) ||
            !FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Text), Root) || !Root.IsValid()) return;
        FString Schema;
        if (!Root->TryGetStringField(TEXT("schema"), Schema) || Schema != TEXT("noworld.shared_reference_library.v1")) return;
        const TArray<TSharedPtr<FJsonValue>>* Clips = nullptr;
        if (!Root->TryGetArrayField(TEXT("clips"), Clips)) return;
        for (const auto& Value : *Clips)
        {
            const auto Object = Value.IsValid() ? Value->AsObject() : nullptr;
            FString Id, Path;
            if (!Object || !Object->TryGetStringField(TEXT("id"), Id) || !Object->TryGetStringField(TEXT("path"), Path) ||
                Id.IsEmpty() || !Path.StartsWith(TEXT("Clips/")) || Path.Contains(TEXT("..")) ||
                Path.Contains(TEXT(":")) || Path.Contains(TEXT("\\")) || !Path.EndsWith(TEXT(".nwmotion"))) continue;
            const FName Key(*Id);
            if (Paths.Contains(Key)) { Paths.Reset(); Routes.Reset(); return; }
            Paths.Add(Key, Path);
        }
        const TArray<TSharedPtr<FJsonValue>>* Values = nullptr;
        if (!Root->TryGetArrayField(TEXT("routes"), Values)) return;
        for (const auto& Value : *Values)
        {
            const auto O = Value.IsValid() ? Value->AsObject() : nullptr;
            if (!O) continue;
            auto Name = [&O](const TCHAR* Key) { FString V; O->TryGetStringField(Key, V); return FName(*V); };
            FOGSharedMotionEntry E;
            E.SourceClipId = Name(TEXT("clip")); E.MotionId = E.SourceClipId;
            const FName LegacyId = Name(TEXT("legacy_source_id"));
            E.GameplayAction = Name(TEXT("action")); E.GameplayContext = Name(TEXT("context"));
            E.CompatibleRigProfile = Name(TEXT("rig")); E.AnatomyProfile = Name(TEXT("anatomy"));
            E.AuthoredSceneId = Name(TEXT("scene_id")); E.SceneRole = Name(TEXT("scene_role"));
            O->TryGetBoolField(TEXT("approved_legacy"), E.bApprovedForGameplay);
            O->TryGetBoolField(TEXT("loop"), E.bLoop);
            O->TryGetBoolField(TEXT("root_motion"), E.bUsesRootMotion);
            O->TryGetBoolField(TEXT("mature_content"), E.bContainsMatureContent);
            E.ContactNotify=Name(TEXT("contact_notify"));
            double Contact=-1;
            if (O->TryGetNumberField(TEXT("contact_seconds"), Contact) && FMath::IsFinite(Contact) && Contact>=0 && Contact<=MAX_flt)
                E.ContactSeconds=static_cast<float>(Contact);
            double Count = 1, Rate = 1, Priority = 0;
            O->TryGetNumberField(TEXT("participants"), Count); O->TryGetNumberField(TEXT("play_rate"), Rate);
            O->TryGetNumberField(TEXT("priority"), Priority);
            if (!Paths.Contains(E.SourceClipId) || E.GameplayAction.IsNone() || E.GameplayContext.IsNone() ||
                !FMath::IsFinite(Count) || Count < 1 || Count > MAX_int32 || FMath::FloorToDouble(Count) != Count ||
                !FMath::IsFinite(Rate) || Rate <= 0 || Rate > MAX_flt ||
                !FMath::IsFinite(Priority) || Priority < MIN_int32 || Priority > MAX_int32) continue;
            E.RequiredParticipants = static_cast<int32>(Count); E.PlayRate = static_cast<float>(Rate);
            E.Priority = static_cast<int32>(Priority);
            const TArray<TSharedPtr<FJsonValue>>* Caps = nullptr;
            if (O->TryGetArrayField(TEXT("capabilities"), Caps)) for (const auto& Cap : *Caps)
                if (Cap.IsValid() && Cap->Type == EJson::String) E.RequiredCapabilities.Add(FName(*Cap->AsString()));
            if (!LegacyId.IsNone()) Aliases.FindOrAdd(LegacyId).Add(E.SourceClipId);
            Routes.Add(MoveTemp(E));
        }
    }
};
FReferenceRegistry& Registry() { static FReferenceRegistry Value; return Value; }
bool VectorFrames(const TSharedPtr<FJsonObject>& Track, const TCHAR* Key, int32 Width, int32 Frames,
    TArray<TArray<double>>& Out, FString& Error)
{
    const TArray<TSharedPtr<FJsonValue>>* Values = nullptr;
    if (!Track->HasField(Key)) return true;
    if (!Track->TryGetArrayField(Key, Values) || Values->Num() != Frames)
        return Fail(Error, TEXT("Shared reference channel frame count mismatch."));
    Out.Reserve(Frames);
    for (const auto& Value : *Values)
    {
        if (!Value.IsValid() || Value->Type != EJson::Array || Value->AsArray().Num() != Width)
            return Fail(Error, TEXT("Shared reference channel width mismatch."));
        TArray<double> Row; Row.Reserve(Width);
        for (const auto& Axis : Value->AsArray())
        {
            double Number = 0;
            if (!Axis.IsValid() || !Axis->TryGetNumber(Number) || !FMath::IsFinite(Number) || FMath::Abs(Number) > MAX_flt)
                return Fail(Error, TEXT("Non-finite shared source channel."));
            Row.Add(Number);
        }
        Out.Add(MoveTemp(Row));
    }
    return true;
}
}

TSharedPtr<const FOGReferenceMotionClip, ESPMode::ThreadSafe> FOGSharedReferenceLibrary::Decode(
    const TArray<uint8>& Bytes, FString& Error)
{
    Error.Reset();
    const uint8 Magic[] = {'N','W','M','R','J','0','0','1'};
    if (Bytes.Num() < 17 || FMemory::Memcmp(Bytes.GetData(), Magic, 8) != 0)
    { Fail(Error, TEXT("Invalid shared reference motion header.")); return nullptr; }
    auto U32 = [&Bytes](int32 At)
    {
        return uint32(Bytes[At]) | uint32(Bytes[At+1]) << 8 | uint32(Bytes[At+2]) << 16 | uint32(Bytes[At+3]) << 24;
    };
    const uint32 Length = U32(8), Checksum = U32(12);
    if (!Length || Length > 64u * 1024 * 1024)
    { Fail(Error, TEXT("Shared reference motion exceeds the decoded clip budget.")); return nullptr; }
    TArray<uint8> Plain; Plain.SetNumUninitialized(Length);
    if (!FCompression::UncompressMemory(NAME_Zlib, Plain.GetData(), Length, Bytes.GetData()+16, Bytes.Num()-16) ||
        FCrc::MemCrc32(Plain.GetData(), Plain.Num()) != Checksum)
    { Fail(Error, TEXT("Shared reference motion failed integrity verification.")); return nullptr; }
    FUTF8ToTCHAR Text(reinterpret_cast<const ANSICHAR*>(Plain.GetData()), Plain.Num());
    const FString Json(Text.Length(), Text.Get());
    TSharedPtr<FJsonObject> Root;
    if (!FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Json), Root) || !Root.IsValid())
    { Fail(Error, TEXT("Shared reference motion JSON is invalid.")); return nullptr; }
    FString Schema, Id; double Rate = 0, Duration = 0, Count = 0;
    if (!Root->TryGetStringField(TEXT("schema"), Schema) || Schema != TEXT("noworld.shared_reference_motion.v1") ||
        !Root->TryGetStringField(TEXT("id"), Id) || Id.IsEmpty() ||
        !Root->TryGetNumberField(TEXT("rate"), Rate) || !Root->TryGetNumberField(TEXT("duration"), Duration) ||
        !Root->TryGetNumberField(TEXT("frames"), Count) ||
        !FMath::IsFinite(Rate) || Rate <= 0 || Rate > MAX_flt ||
        !FMath::IsFinite(Duration) || Duration < 0 || Duration > MAX_flt ||
        !FMath::IsFinite(Count) || Count < 1 || Count > 1000000 || FMath::FloorToDouble(Count) != Count)
    { Fail(Error, TEXT("Shared reference motion identity or timing is invalid.")); return nullptr; }
    auto Clip = MakeShared<FOGReferenceMotionClip, ESPMode::ThreadSafe>();
    Clip->Id = FName(*Id); Clip->Rate = static_cast<float>(Rate); Clip->Duration = static_cast<float>(Duration);
    Clip->Frames = static_cast<int32>(Count);
    const TArray<TSharedPtr<FJsonValue>>* Tracks = nullptr;
    if (!Root->TryGetArrayField(TEXT("tracks"), Tracks) || Tracks->IsEmpty() || Tracks->Num() > 8192)
    { Fail(Error, TEXT("Shared reference motion needs actual source tracks.")); return nullptr; }
    TSet<FName> Names;
    for (const auto& Value : *Tracks)
    {
        const auto O = Value.IsValid() ? Value->AsObject() : nullptr;
        FString Bone;
        if (!O || !O->TryGetStringField(TEXT("bone"), Bone) || Bone.IsEmpty() || Names.Contains(FName(*Bone)))
        { Fail(Error, TEXT("Ambiguous source bone identity.")); return nullptr; }
        Names.Add(FName(*Bone));
        auto& Track = Clip->Tracks.AddDefaulted_GetRef(); Track.Bone = FName(*Bone);
        const TArray<TSharedPtr<FJsonValue>>* Aliases = nullptr;
        if (O->TryGetArrayField(TEXT("aliases"), Aliases)) for (const auto& Alias : *Aliases)
            if (Alias.IsValid() && Alias->Type == EJson::String && !Alias->AsString().IsEmpty())
                Track.Aliases.AddUnique(FName(*Alias->AsString()));
        TArray<TArray<double>> Rotation, Translation, Scale;
        if (!VectorFrames(O, TEXT("rotations"), 4, Clip->Frames, Rotation, Error) ||
            !VectorFrames(O, TEXT("translations"), 3, Clip->Frames, Translation, Error) ||
            !VectorFrames(O, TEXT("scales"), 3, Clip->Frames, Scale, Error)) return nullptr;
        if (Rotation.IsEmpty() && Translation.IsEmpty() && Scale.IsEmpty())
        { Fail(Error, TEXT("Source track contains no sampled channels.")); return nullptr; }
        for (const auto& R : Rotation)
        {
            FQuat Q(R[0], R[1], R[2], R[3]);
            if (Q.SizeSquared() < SMALL_NUMBER || FMath::Abs(Q.SizeSquared()-1.) > .01)
            { Fail(Error, TEXT("Original quaternion delta is not normalized.")); return nullptr; }
            Track.Rotations.Add(Q.GetNormalized());
        }
        for (const auto& T : Translation) Track.Translations.Add(FVector(T[0],T[1],T[2]));
        for (const auto& S : Scale) Track.Scales.Add(FVector(S[0],S[1],S[2]));
        Clip->ResidentBytes += Track.Rotations.GetAllocatedSize() + Track.Translations.GetAllocatedSize() +
            Track.Scales.GetAllocatedSize() + Track.Aliases.GetAllocatedSize();
    }
    Clip->ResidentBytes += Clip->Tracks.GetAllocatedSize();
    return Clip;
}

FTransform FOGReferenceMotionClip::Sample(int32 Index, double Seconds) const
{
    if (!Tracks.IsValidIndex(Index) || !FMath::IsFinite(Seconds) || Frames < 1) return FTransform::Identity;
    const auto& Track = Tracks[Index];
    const double Time = FMath::Clamp(Seconds, 0., static_cast<double>(Duration));
    const int32 A = FMath::Clamp(FMath::FloorToInt(Time * Rate), 0, Frames-1);
    const int32 B = FMath::Min(A+1, Frames-1);
    const double Start = A / static_cast<double>(Rate);
    const double End = B == Frames-1 ? static_cast<double>(Duration) : B / static_cast<double>(Rate);
    const double Alpha = A == B || End <= Start ? 0 : FMath::Clamp((Time-Start)/(End-Start),0.,1.);
    const FQuat Q = Track.Rotations.IsEmpty() ? FQuat::Identity :
        FQuat::Slerp(Track.Rotations[A], Track.Rotations[B], Alpha).GetNormalized();
    const FVector T = Track.Translations.IsEmpty() ? FVector::ZeroVector :
        FMath::Lerp(Track.Translations[A],Track.Translations[B],Alpha);
    const FVector S = Track.Scales.IsEmpty() ? FVector::OneVector :
        FMath::Lerp(Track.Scales[A],Track.Scales[B],Alpha);
    return FTransform(Q,T,S);
}

bool FOGReferenceRigAdapter::Bind(USkeletalMeshComponent* Part, FString& Error)
{
    Mesh.Reset(); Bones.Reset(); ReferencePose.Reset();
    USkeletalMesh* Asset = Part ? Part->GetSkeletalMeshAsset() : nullptr;
    if (!Asset || !Asset->GetSkeleton()) return Fail(Error, TEXT("Actual native rig is unavailable."));
    const auto& Ref = Asset->GetRefSkeleton();
    if (!Ref.GetNum()) return Fail(Error, TEXT("Native rig has no reference bones."));
    for (int32 I=0; I<Ref.GetNum(); ++I)
    {
        if (Bones.Contains(Ref.GetBoneName(I)) || Ref.GetRefBonePose()[I].ContainsNaN())
            return Fail(Error, TEXT("Actual native rig reference contract is invalid."));
        Bones.Add(Ref.GetBoneName(I),I);
    }
    ReferencePose = Ref.GetRefBonePose(); Mesh = Asset;
    return true;
}
bool FOGReferenceRigAdapter::Map(const FOGReferenceMotionClip& Clip, const TMap<FName,FName>& Aliases,
    TArray<int32>& Out, FString& Error) const
{
    Out.Reset();
    if (!Mesh.IsValid()) return Fail(Error,TEXT("Character rig adapter expired."));
    TSet<int32> Used;
    for (const auto& Track : Clip.Tracks)
    {
        const FName* Explicit = Aliases.Find(Track.Bone);
        const int32* Bone = Bones.Find(Explicit ? *Explicit : Track.Bone);
        if (!Bone && !Explicit) for (FName Alias : Track.Aliases)
            if ((Bone = Bones.Find(Alias))) break;
        if (!Bone || Used.Contains(*Bone))
        { Out.Reset(); return Fail(Error,TEXT("Rig needs a complete, unambiguous source-bone adapter.")); }
        Used.Add(*Bone); Out.Add(*Bone);
    }
    return true;
}
bool FOGReferenceRigAdapter::Evaluate(const FOGReferenceMotionClip& Clip, const TArray<int32>& Indices,
    double Seconds, TArray<FTransform>& Out) const
{
    if (!Mesh.IsValid() || Indices.Num()!=Clip.Tracks.Num()) return false;
    Out = ReferencePose;
    for (int32 I=0; I<Indices.Num(); ++I)
    {
        if (!Out.IsValidIndex(Indices[I])) return false;
        FTransform& Pose = Out[Indices[I]];
        const FTransform Delta = Clip.Sample(I,Seconds);
        Pose.SetRotation((Pose.GetRotation()*Delta.GetRotation()).GetNormalized());
        Pose.AddToTranslation(Delta.GetTranslation());
        Pose.SetScale3D(Pose.GetScale3D()*Delta.GetScale3D());
    }
    return true;
}

TSharedPtr<const FOGReferenceMotionClip, ESPMode::ThreadSafe> FOGSharedReferenceLibrary::Load(FName Id, FString& Error)
{
    if (!IsInGameThread()) { Fail(Error,TEXT("Shared asset loading requires the game thread.")); return nullptr; }
    auto& R = Registry(); R.Ensure();
    Id = CanonicalId(Id);
    if (auto* Cached = R.Cache.Find(Id)) { Cached->Used = ++R.Clock; return Cached->Clip; }
    const FString* Path = R.Paths.Find(Id);
    if (!Path) { Fail(Error,TEXT("Original shared motion is not installed.")); return nullptr; }
    TArray<uint8> Bytes;
    if (!FFileHelper::LoadFileToArray(Bytes, *(LibraryRoot() / *Path)))
    { Fail(Error,TEXT("Shared source file is unavailable offline.")); return nullptr; }
    auto Clip = Decode(Bytes,Error);
    if (!Clip || Clip->Id!=Id) { if (Error.IsEmpty()) Fail(Error,TEXT("Shared file identity mismatch.")); return nullptr; }
    FCachedReference Item; Item.Clip=Clip; Item.Used=++R.Clock; R.Cache.Add(Id,MoveTemp(Item));
    TrimCache();
    return Clip;
}
void FOGSharedReferenceLibrary::TrimCache(int64 Budget)
{
    check(IsInGameThread());
    auto& R=Registry(); int64 Total=0;
    for (const auto& Pair:R.Cache) Total+=Pair.Value.Clip->ResidentBytes;
    while (Total>FMath::Max(int64(0),Budget))
    {
        FName Oldest; uint64 Used=MAX_uint64;
        for (const auto& Pair:R.Cache) if (Pair.Value.Clip.IsUnique() && Pair.Value.Used<Used)
        { Oldest=Pair.Key; Used=Pair.Value.Used; }
        if (Oldest.IsNone()) break; // Playing clips remain resident.
        Total-=R.Cache[Oldest].Clip->ResidentBytes; R.Cache.Remove(Oldest);
    }
}
FName FOGSharedReferenceLibrary::CanonicalId(FName Id)
{
    if (!IsInGameThread()) return NAME_None;
    auto& R=Registry(); R.Ensure();
    if (R.Paths.Contains(Id)) return Id;
    if (const auto* Choices=R.Aliases.Find(Id); Choices && Choices->Num()==1)
        for (FName Choice:*Choices) return Choice;
    return NAME_None; // Ambiguous historical IDs never select an arbitrary original.
}
TArray<FName> FOGSharedReferenceLibrary::GetClipIds()
{
    check(IsInGameThread()); auto& R=Registry(); R.Ensure();
    TArray<FName> Ids; R.Paths.GetKeys(Ids); Ids.Sort(FNameLexicalLess()); return Ids;
}
TArray<FOGSharedMotionEntry> FOGSharedReferenceLibrary::Find(FName Action,FName Context,FName Rig,FName Anatomy,
    const TArray<FName>& Capabilities,int32 Count,FName Role)
{
    TArray<FOGSharedMotionEntry> Out;
    if (!IsInGameThread() || Action.IsNone() || Context.IsNone() || Rig.IsNone() || Count<1) return Out;
    auto& R=Registry(); R.Ensure();
    for (const auto& E:R.Routes)
    {
        if (!E.bApprovedForGameplay || E.GameplayAction!=Action || E.GameplayContext!=Context ||
            E.CompatibleRigProfile!=Rig || (!E.AnatomyProfile.IsNone() && E.AnatomyProfile!=Anatomy) ||
            E.RequiredParticipants!=Count ||
            (Count>1 ? (Role.IsNone() || E.SceneRole!=Role || E.AuthoredSceneId.IsNone()) : !E.SceneRole.IsNone())) continue;
        bool Missing=false; for (FName Required:E.RequiredCapabilities) if (!Capabilities.Contains(Required)) { Missing=true; break; }
        if (!Missing) Out.Add(E);
    }
    Out.Sort([](const FOGSharedMotionEntry& A,const FOGSharedMotionEntry& B)
    { return A.Priority==B.Priority ? A.SourceClipId.LexicalLess(B.SourceClipId) : A.Priority>B.Priority; });
    return Out;
}
