#include "Content/OGDefinitionLibrary.h"
#include "Combat/OGNativeTurnAgentComponent.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "AssetRegistry/AssetData.h"
#include "Characters/OGCharacterAppearanceDefinition.h"
#include "Combat/OGNativeCombatAbility.h"
#include "Effects/OGNativeCombatEffect.h"
#include "Interaction/OGInteractionDefinition.h"
#include "Animation/OGSharedMotionLibrary.h"
#include "Animation/OGSharedReferenceMotion.h"
#include "Runtime/OGAdaptiveScoreComponent.h"
#include "Progression/OGCharacterDevelopmentService.h"
#include "World/OGOwnedEquipmentService.h"
#include "Narrative/OGNarrativeService.h"
#include "Dom/JsonObject.h"
#include "Engine/Engine.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Modules/ModuleManager.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"

namespace
{
FString Key(FName System,const FOGContentId& Id) { return System.ToString().ToLower()+TEXT("|")+Id.ToString(); }
bool Fail(FString& Error,const FString& Text) { Error=Text; return false; }
bool CheckJson(const FString& Json,FString& Error)
{
    struct FFrame { bool Object=false; TSet<FString> Keys; };
    TArray<FFrame> Stack; auto Reader=TJsonReaderFactory<>::Create(Json); EJsonNotation Token;
    while (Reader->ReadNext(Token))
    {
        if (Token==EJsonNotation::Error) return Fail(Error,Reader->GetErrorMessage());
        if (Token==EJsonNotation::ObjectEnd || Token==EJsonNotation::ArrayEnd)
        { if (Stack.IsEmpty()) return Fail(Error,TEXT("Invalid definition JSON nesting.")); Stack.Pop(); continue; }
        if (!Stack.IsEmpty() && Stack.Last().Object)
        {
            const FString Name=Reader->GetIdentifier();
            if (Stack.Last().Keys.Contains(Name)) return Fail(Error,TEXT("Duplicate definition JSON key: ")+Name);
            Stack.Last().Keys.Add(Name);
        }
        if (Token==EJsonNotation::ObjectStart || Token==EJsonNotation::ArrayStart)
        { FFrame F; F.Object=Token==EJsonNotation::ObjectStart; Stack.Add(MoveTemp(F)); }
    }
    return Stack.IsEmpty() && Reader->GetErrorMessage().IsEmpty();
}
void Space(const FString& S,int32& I) { while (I<S.Len() && FChar::IsWhitespace(S[I])) ++I; }
bool SkipValue(const FString& S,int32& I)
{
    Space(S,I); if (I>=S.Len()) return false;
    bool String=false,Escape=false; int32 Depth=0;
    for (;I<S.Len();++I)
    {
        const TCHAR C=S[I];
        if (String) { if (Escape) Escape=false; else if (C=='\\') Escape=true; else if (C=='"') String=false; continue; }
        if (C=='"') { String=true; continue; }
        if (C=='[' || C=='{') { ++Depth; continue; }
        if (C==']' || C=='}') { if (!Depth) return true; if (--Depth==0) { ++I; return true; } continue; }
        if (C==',' && !Depth) return true;
    }
    return !String && Depth==0;
}
bool RawArrays(const FString& Json,const TArray<FString>& Fields,const TArray<FString>& MetadataFields,
    FString& Schema,TMap<FString,FString>& Arrays,TMap<FString,FString>& Metadata,FString& Error)
{
    if (!CheckJson(Json,Error)) return false;
    TSharedPtr<FJsonObject> Root;
    if (!FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Json),Root) || !Root ||
        !Root->TryGetStringField(TEXT("schema"),Schema)) return Fail(Error,TEXT("Definition catalog requires a schema."));
    for (const auto& Pair:Root->Values)
        if (Pair.Key!=TEXT("schema") && !Fields.Contains(Pair.Key) && !MetadataFields.Contains(Pair.Key))
            return Fail(Error,FString(TEXT("Unknown catalog root field: "))+FString(Pair.Key.ToString()));
    int32 I=0; Space(Json,I); if (I>=Json.Len() || Json[I++]!='{') return false;
    while (I<Json.Len())
    {
        Space(Json,I); if (I<Json.Len() && Json[I]=='}') break;
        if (I>=Json.Len() || Json[I]!='"') return false;
        const int32 Start=++I; while (I<Json.Len() && Json[I]!='"') { if (Json[I]=='\\') return Fail(Error,TEXT("Catalog root keys must use their literal schema names.")); ++I; }
        const FString Name=Json.Mid(Start,I-Start); ++I; Space(Json,I);
        if (I>=Json.Len() || Json[I++]!=':') return false;
        Space(Json,I); const int32 Begin=I; if (!SkipValue(Json,I)) return false;
        if (Fields.Contains(Name))
        {
            if (Json[Begin]!='[' || I<=Begin+1 || Json[I-1]!=']') return Fail(Error,TEXT("Catalog field must be an array: ")+Name);
            Arrays.Add(Name,Json.Mid(Begin+1,I-Begin-2).TrimStartAndEnd());
        }
        else if (MetadataFields.Contains(Name)) Metadata.Add(Name,Json.Mid(Begin,I-Begin).TrimStartAndEnd());
        Space(Json,I); if (I<Json.Len() && Json[I]==',') ++I; else break;
    }
    for (const auto& Field:Fields) if (!Arrays.Contains(Field)) return Fail(Error,TEXT("Missing catalog array: ")+Field);
    return true;
}
void CollectAssetReferences(UDataAsset* Asset,TArray<FOGDefinitionReference>& Out)
{
    if (const auto* D=Cast<UOGReusableDefinition>(Asset)) D->CollectRequiredDefinitions(Out);
    if (const auto* Ability=Cast<UOGNativeCombatAbility>(Asset))
        for (const auto& Op:Ability->Definition.Effects)
            if (Op.Operation==EOGNativeEffectOperation::Apply)
            { FOGDefinitionReference Ref; Ref.System=TEXT("effect"); Ref.Id=Op.Id; Out.Add(Ref); }
}
bool ValidateAsset(const FOGDefinitionAssetEntry& E,UDataAsset* A,FString& Error)
{
    if (!A || A->HasAnyFlags(RF_Transient|RF_ClassDefaultObject) || !A->GetPathName().StartsWith(TEXT("/Game/")) ||
        A->GetPathName()!=E.Asset.ToSoftObjectPath().ToString()) return Fail(Error,TEXT("Definition asset is missing, redirected or not installed."));
    if (const auto* D=Cast<UOGCharacterAppearanceDefinition>(A))
        return (E.System==TEXT("appearance") && E.Id==D->VersionId && D->ValidateContract(Error)) || Fail(Error,Error.IsEmpty()?TEXT("Appearance definition key mismatch."):Error);
    if (const auto* D=Cast<UOGNativeCombatAbility>(A))
        return (E.System==TEXT("ability") && E.Id==D->Definition.SkillId && D->ValidateInstalled(Error)) || Fail(Error,Error.IsEmpty()?TEXT("Ability definition key mismatch."):Error);
    if (const auto* D=Cast<UOGInteractionDefinition>(A))
    { FOGInteractionActionGraph Graph; return (E.System==TEXT("interaction") && E.Id==D->GraphId && D->BuildGraph(Graph,Error)) || Fail(Error,Error.IsEmpty()?TEXT("Interaction definition key mismatch."):Error); }
    if (const auto* D=Cast<UOGNativeTurnPolicy>(A))
        return (E.System==TEXT("agent") && D->Validate(Error)) || Fail(Error,Error.IsEmpty()?TEXT("Turn policy definition key mismatch."):Error);
    if (const auto* D=Cast<UOGAdaptiveScoreDefinition>(A))
        return (E.System==TEXT("score") && D->Validate(Error)) || Fail(Error,Error.IsEmpty()?TEXT("Score definition key mismatch."):Error);
    if (const auto* Motion=Cast<UOGSharedMotionLibrary>(A))
        return (E.System==TEXT("motion") && Motion->ValidateLibrary(Error)) || Fail(Error,Error.IsEmpty()?TEXT("Motion definition key mismatch."):Error);
    if (const auto* D=Cast<UOGReusableDefinition>(A))
        return (D->System==E.System && D->DefinitionId==E.Id && D->ValidateDefinition(Error)) || Fail(Error,Error.IsEmpty()?TEXT("Reusable definition key mismatch."):Error);
    return Fail(Error,TEXT("Definition type must implement the reusable definition contract."));
}
}
bool UOGReusableDefinition::ValidateDefinition_Implementation(FString& Error) const
{
    Error.Reset();
    if (System.IsNone() || !DefinitionId.IsValid()) return Fail(Error,TEXT("Definition requires a system and stable ID."));
    for (const auto& Ref:Dependencies)
        if (Ref.System.IsNone() || !Ref.Id.IsValid() || Ref.MinimumVersion<1) return Fail(Error,TEXT("Invalid definition dependency."));
    return true;
}
UOGDefinitionLibrary* UOGDefinitionLibrary::Get()
{ return GEngine?GEngine->GetEngineSubsystem<UOGDefinitionLibrary>():nullptr; }
void UOGDefinitionLibrary::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection); FString Error;
    RegisterCatalogSystem(TEXT("development"),TEXT("NoWorld/Progression/DevelopmentCatalog.json"),
        TEXT("{\"schema\":\"noworld.character_development.v1\",\"offers\":[]}"),{TEXT("offers")},
        [](const FString& J,FString& E){TArray<FOGCharacterDevelopmentDefinition> D;return FOGCharacterDevelopmentCatalog::Parse(J,D,E);},Error);
    RegisterCatalogSystem(TEXT("equipment"),TEXT("NoWorld/Equipment/EquipmentCatalog.json"),
        TEXT("{\"schema\":\"noworld.equipment.v1\",\"slots\":[],\"items\":[],\"rules\":[]}"),{TEXT("slots"),TEXT("items"),TEXT("rules")},
        [](const FString& J,FString& E){FOGEquipmentCatalog D;return FOGEquipmentCatalog::Parse(J,D,E);},Error);
    RegisterCatalogSystem(TEXT("narrative"),TEXT("NoWorld/Narrative/NarrativeCatalog.json"),
        TEXT("{\"schema\":\"noworld.narrative.v1\",\"quests\":[],\"dialogues\":[]}"),{TEXT("quests"),TEXT("dialogues")},
        [](const FString& J,FString& E){FOGNarrativeCatalog D;return FOGNarrativeCatalog::Parse(J,D,E);},Error);
    // Reuse the actual original calibration contract; no invented Skeleton fallback.
    FString SourceText; TSharedPtr<FJsonObject> SourceRoot;
    const FString SourcePath=TEXT("NoWorld/Animation/SharedSource/SharedReferenceLibrary.json");
    if (FFileHelper::LoadFileToString(SourceText,*(FPaths::ProjectContentDir()/SourcePath)) &&
        FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(SourceText),SourceRoot) && SourceRoot)
    {
        FString Schema,Basis,Skeleton;
        if (SourceRoot->TryGetStringField(TEXT("schema"),Schema) &&
            SourceRoot->TryGetStringField(TEXT("calibration_basis"),Basis) &&
            SourceRoot->TryGetStringField(TEXT("calibration_skeleton"),Skeleton))
        {
            auto Empty=MakeShared<FJsonObject>(); Empty->SetStringField(TEXT("schema"),Schema);
            Empty->SetStringField(TEXT("calibration_basis"),Basis); Empty->SetStringField(TEXT("calibration_skeleton"),Skeleton);
            Empty->SetArrayField(TEXT("clips"),{}); Empty->SetArrayField(TEXT("routes"),{});
            FString EmptyJson; FJsonSerializer::Serialize(Empty,TJsonWriterFactory<>::Create(&EmptyJson));
            if (!RegisterCatalogSystem(TEXT("source_motion"),SourcePath,EmptyJson,{TEXT("clips"),TEXT("routes")},
                [](const FString& Json,FString& Failure){return FOGSharedReferenceLibrary::ValidateManifestJson(Json,Failure,true);},
                Error,{TEXT("calibration_basis"),TEXT("calibration_skeleton"),TEXT("coverage")}))
                UE_LOG(LogTemp,Error,TEXT("Original shared source catalog admission failed: %s"),*Error);
        }
    }

}
void UOGDefinitionLibrary::Deinitialize()
{
    OnDefinitionsChanged.Clear(); Entries.Reset(); CatalogFragments.Reset(); RetainedAssets.Reset(); ActiveBundles.Reset();
    CatalogAdapters.Reset(); Installed.Reset(); Super::Deinitialize();
}
bool UOGDefinitionLibrary::RegisterCatalogSystem(FName System,const FString& Path,const FString& Empty,
    const TArray<FString>& Arrays,FOGDefinitionCatalogValidator Validator,FString& Error,const TArray<FString>& Metadata)
{
    if (System.IsNone() || Path.Contains(TEXT("..")) || Path.Contains(TEXT(":")) || FPaths::IsRelative(Path)==false ||
        Arrays.IsEmpty() || !Validator || CatalogAdapters.Contains(System)) return Fail(Error,TEXT("Invalid or duplicate catalog adapter."));
    TSet<FString> SeenFields;
    TArray<FString> Fields=Arrays; Fields.Append(Metadata);
    for (const auto& Field:Fields)
    {
        if (Field.IsEmpty() || Field==TEXT("schema") || SeenFields.Contains(Field))
            return Fail(Error,TEXT("Catalog array fields must be distinct literal schema names."));
        for (TCHAR C:Field) if (!FChar::IsAlnum(C) && C!='_')
            return Fail(Error,TEXT("Catalog array fields require literal alphanumeric/underscore names."));
        SeenFields.Add(Field);
    }
    if (!Validator(Empty,Error)) return false;
    FCatalogAdapter A; A.BasePath=Path; A.EmptyJson=Empty; A.Arrays=Arrays; A.Metadata=Metadata; A.Validate=MoveTemp(Validator);
    CatalogAdapters.Add(System,MoveTemp(A)); return true;
}
bool UOGDefinitionLibrary::MergeCatalogDocuments(const FString& Base,const TArray<FString>& Fragments,
    const TArray<FString>& Fields,FString& Out,FString& Error,const TArray<FString>& MetadataFields)
{
    Out.Reset(); Error.Reset(); TMap<FString,FString> Combined,Metadata; FString Schema;
    if (!RawArrays(Base,Fields,MetadataFields,Schema,Combined,Metadata,Error)) return false;
    for (const auto& Fragment:Fragments)
    {
        TMap<FString,FString> More,OtherMetadata; FString Other;
        if (!RawArrays(Fragment,Fields,MetadataFields,Other,More,OtherMetadata,Error)) return false;
        for (const auto& Pair:OtherMetadata)
            if (!Metadata.Contains(Pair.Key) || Metadata[Pair.Key]!=Pair.Value)
                return Fail(Error,TEXT("A source catalog fragment cannot replace its calibration/provenance metadata."));
        if (Other!=Schema) return Fail(Error,TEXT("Catalog schema mismatch."));
        for (const auto& Field:Fields) if (!More[Field].IsEmpty())
        { if (!Combined[Field].IsEmpty()) Combined[Field]+=TEXT(","); Combined[Field]+=More[Field]; }
    }
    // Array payloads stay verbatim: int64 amounts and large-number text never round-trip through doubles.
    FString Prefix; const auto Writer=TJsonWriterFactory<>::Create(&Prefix);
    Writer->WriteObjectStart(); Writer->WriteValue(TEXT("schema"),Schema); Writer->WriteObjectEnd(); Writer->Close();
    Prefix.TrimEndInline(); Prefix.RemoveAt(Prefix.Len()-1);
    Out=MoveTemp(Prefix);
    for (const auto& Field:MetadataFields) if (const auto* Raw=Metadata.Find(Field)) Out+=TEXT(",\"")+Field+TEXT("\":")+*Raw;
    for (const auto& Field:Fields) Out+=TEXT(",\"")+Field+TEXT("\":[")+Combined[Field]+TEXT("]");
    Out+=TEXT("}"); return true;
}
bool UOGDefinitionLibrary::BuildCandidate(const TArray<FInstalledBundle>& Bundles,
    TMap<FString,FResolvedEntry>& OutEntries,TMap<FName,TArray<FString>>& OutCatalogs,TArray<UDataAsset*>& OutAssets,FString& Error) const
{
    OutEntries.Reset(); OutCatalogs.Reset(); OutAssets.Reset(); Error.Reset();
    TMap<FOGContentId,const FInstalledBundle*> ById;
    for (const auto& Snapshot:Bundles)
    {
        const auto* B=&Snapshot;
        if (!B->BundleId.IsValid() || B->Version<1 || ById.Contains(B->BundleId))
            return Fail(Error,TEXT("Invalid, duplicate or noninstalled definition bundle."));
        ById.Add(B->BundleId,B);
    }
    TSet<FOGContentId> Done,Visiting;
    TFunction<bool(const FInstalledBundle*)> Visit=[&](const FInstalledBundle* B)
    {
        if (Done.Contains(B->BundleId)) return true;
        if (Visiting.Contains(B->BundleId)) return Fail(Error,TEXT("Definition bundle dependency cycle."));
        Visiting.Add(B->BundleId); TSet<FOGContentId> Seen;
        for (const auto& D:B->Dependencies)
        {
            const FInstalledBundle* const* Found=ById.Find(D.BundleId);
            if (!D.BundleId.IsValid() || D.MinimumVersion<1 || Seen.Contains(D.BundleId) ||
                !Found || (*Found)->Version<D.MinimumVersion) return Fail(Error,TEXT("Missing, duplicate or outdated bundle dependency."));
            Seen.Add(D.BundleId); if (!Visit(*Found)) return false;
        }
        Visiting.Remove(B->BundleId); Done.Add(B->BundleId); return true;
    };
    for (const auto& B:Bundles) if (!Visit(&B)) return false;
    for (const auto& B:Bundles)
    {
        for (const auto& E:B.Assets)
        {
            const FString K=Key(E.System,E.Id);
            if (E.System.IsNone() || !E.Id.IsValid() || E.Version<1 || OutEntries.Contains(K))
                return Fail(Error,TEXT("Duplicate or invalid shared definition: ")+K);
            auto* A=E.Asset.LoadSynchronous();
            if (!ValidateAsset(E,A,Error)) return false;
            FResolvedEntry R; R.Entry=E; R.Asset=A; OutEntries.Add(K,R); OutAssets.AddUnique(A);
        }
        for (const auto& C:B.Catalogs)
        { if (!CatalogAdapters.Contains(C.System)) return Fail(Error,TEXT("No registered catalog adapter: ")+C.System.ToString()); OutCatalogs.FindOrAdd(C.System).Add(C.Json); }
    }
    for (const auto& Pair:OutEntries)
    {
        TArray<FOGDefinitionReference> Refs=Pair.Value.Entry.Dependencies;
        CollectAssetReferences(Pair.Value.Asset,Refs);
        for (const auto& Ref:Refs)
        {
            const auto* Target=OutEntries.Find(Key(Ref.System,Ref.Id));
            if (Ref.System.IsNone() || !Ref.Id.IsValid() || Ref.MinimumVersion<1 || !Target || Target->Entry.Version<Ref.MinimumVersion)
                return Fail(Error,TEXT("Missing or outdated definition dependency for ")+Pair.Key);
        }
    }
    // Reject definition cycles independently of bundle load ordering.
    TMap<FString,int32> Pending; TMap<FString,TArray<FString>> Dependents; TArray<FString> Ready;
    for (const auto& Pair:OutEntries)
    {
        TArray<FOGDefinitionReference> Refs=Pair.Value.Entry.Dependencies;
        CollectAssetReferences(Pair.Value.Asset,Refs);
        TSet<FString> Unique;
        for (const auto& Ref:Refs) Unique.Add(Key(Ref.System,Ref.Id));
        Pending.Add(Pair.Key,Unique.Num());
        for (const auto& Target:Unique) Dependents.FindOrAdd(Target).Add(Pair.Key);
        if (Unique.IsEmpty()) Ready.Add(Pair.Key);
    }
    for (int32 I=0;I<Ready.Num();++I)
        if (const auto* Children=Dependents.Find(Ready[I]))
            for (const auto& Child:*Children) if (--Pending.FindChecked(Child)==0) Ready.Add(Child);
    if (Ready.Num()!=OutEntries.Num()) return Fail(Error,TEXT("Shared definition dependency cycle."));
    const FOGDefinitionResolver Resolve=[&OutEntries](FName System,const FOGContentId& Id)->UDataAsset*
    { const auto* Entry=OutEntries.Find(Key(System,Id)); return Entry?Entry->Asset:nullptr; };
    for (const auto& Pair:OutEntries)
        if (const auto* D=Cast<UOGReusableDefinition>(Pair.Value.Asset); D && !D->ValidateResolvedReferences(Resolve,Error)) return false;
    for (const auto& Pair:OutEntries)
        if (const auto* Ability=Cast<UOGNativeCombatAbility>(Pair.Value.Asset))
            for (const auto& Op:Ability->Definition.Effects)
                if (Op.Operation==EOGNativeEffectOperation::Apply &&
                    !Cast<UOGNativeCombatEffect>(Resolve(TEXT("effect"),Op.Id)))
                    return Fail(Error,TEXT("Shared ability effect reference has the wrong native definition class."));
    TArray<UOGSharedMotionLibrary*> MotionLibraries;
    for (const auto& Pair:OutEntries)
        if (auto* Motion=Cast<UOGSharedMotionLibrary>(Pair.Value.Asset)) MotionLibraries.Add(Motion);
    TArray<FOGSharedMotionEntry> SharedMotions;
    if (!FOGSharedMotionResolver::ComposeLibraries(*FOGSharedMotionResolver::GetBuiltInLibrary(),MotionLibraries,SharedMotions,Error)) return false;
    for (const auto& Pair:OutCatalogs)
    {
        const auto& A=CatalogAdapters[Pair.Key]; FString Base,Merged;
        if (!FFileHelper::LoadFileToString(Base,*(FPaths::ProjectContentDir()/A.BasePath))) Base=A.EmptyJson;
        if (!MergeCatalogDocuments(Base,Pair.Value,A.Arrays,Merged,Error,A.Metadata) || !A.Validate(Merged,Error)) return false;
    }
    return true;
}
bool UOGDefinitionLibrary::Publish(const TArray<FInstalledBundle>& Input,FString& Error)
{
    if (!IsInGameThread() || bPublishing) return Fail(Error,TEXT("Definition publication requires a nonreentrant game-thread transaction."));
    TGuardValue<bool> Publishing(bPublishing,true);
    auto Bundles=Input; Bundles.Sort([](const FInstalledBundle& A,const FInstalledBundle& B){return A.BundleId.Value<B.BundleId.Value;});
    TMap<FString,FResolvedEntry> Candidate; TMap<FName,TArray<FString>> Catalogs; TArray<UDataAsset*> Assets;
    if (!BuildCandidate(Bundles,Candidate,Catalogs,Assets,Error)) return false;
    ActiveBundles.Reset(); RetainedAssets.Reset();
    for (const auto& B:Bundles) ActiveBundles.Add(B.Source);
    Installed=Bundles;
    for (auto* A:Assets) RetainedAssets.Add(A);
    Entries=MoveTemp(Candidate); CatalogFragments=MoveTemp(Catalogs); ++Revision;
    OnDefinitionsChanged.Broadcast(Revision); return true;
}
bool UOGDefinitionLibrary::InstallBundle(UOGDefinitionBundle* Bundle,FString& Error)
{
    if (!IsInGameThread() || bPublishing || !Bundle || !Bundle->BundleId.IsValid() || Bundle->Version<1 ||
        Bundle->HasAnyFlags(RF_Transient|RF_ClassDefaultObject) || !Bundle->GetPathName().StartsWith(TEXT("/Game/")))
        return Fail(Error,TEXT("A definition bundle must be an installed asset, published on the game thread without reentry."));
    auto Candidate=Installed;
    const int32 Existing=Candidate.IndexOfByPredicate([&](const auto& B){return B.BundleId==Bundle->BundleId;});
    if (Existing!=INDEX_NONE)
    {
        const auto& Previous=Candidate[Existing];
        if (Previous.Source==Bundle && Previous.Version==Bundle->Version) { Error.Reset(); return true; }
        if (Bundle->Version<=Previous.Version) return Fail(Error,TEXT("Definition bundle updates require a newer version."));
        Candidate.RemoveAt(Existing);
    }
    FInstalledBundle Snapshot; Snapshot.BundleId=Bundle->BundleId; Snapshot.Version=Bundle->Version;
    Snapshot.Source=Bundle; Snapshot.Dependencies=Bundle->Dependencies; Snapshot.Assets=Bundle->Assets; Snapshot.Catalogs=Bundle->Catalogs;
    Candidate.Add(MoveTemp(Snapshot)); return Publish(Candidate,Error);
}
bool UOGDefinitionLibrary::RemoveBundle(const FOGContentId& Id,FString& Error)
{
    auto Candidate=Installed;
    if (!Candidate.RemoveAll([&](const auto& B){return B.BundleId==Id;}))
        return Fail(Error,TEXT("Definition bundle is not installed."));
    return Publish(Candidate,Error);
}
void UOGDefinitionLibrary::RefreshDiscovery()
{
    if (!IsInGameThread() || bPublishing || bDiscovering) return;
    bDiscovered=false; EnsureDiscovered();
}
void UOGDefinitionLibrary::EnsureDiscovered()
{
    if (bDiscovered || bDiscovering || bPublishing || !IsInGameThread()) return;
    TGuardValue<bool> Discovering(bDiscovering,true);
    auto& Registry=FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry")).Get();
    if (Registry.IsLoadingAssets()) return;
    bDiscovered=true;
    FARFilter Filter; Filter.PackagePaths.Add(TEXT("/Game/NoWorld/Definitions")); Filter.bRecursivePaths=true;
    Filter.ClassPaths.Add(UOGDefinitionBundle::StaticClass()->GetClassPathName()); Filter.bRecursiveClasses=true;
    TArray<FAssetData> Assets; Registry.GetAssets(Filter,Assets);
    Assets.Sort([](const FAssetData& A,const FAssetData& B){return A.GetSoftObjectPath().ToString()<B.GetSoftObjectPath().ToString();});
    TArray<UOGDefinitionBundle*> Pending; for (const auto& A:Assets) if (auto* B=Cast<UOGDefinitionBundle>(A.GetAsset())) Pending.Add(B);
    // Install a complete discovery set first, so cross-bundle definition references
    // do not require redundant authoring of load-order dependencies.
    auto Complete=Installed;
    for (auto* B:Pending)
    {
        const int32 Index=Complete.IndexOfByPredicate([&](const auto& Old){return Old.BundleId==B->BundleId;});
        if (Index!=INDEX_NONE && Complete[Index].Version>=B->Version) continue;
        if (Index!=INDEX_NONE) Complete.RemoveAt(Index);
        FInstalledBundle Snapshot; Snapshot.BundleId=B->BundleId; Snapshot.Version=B->Version; Snapshot.Source=B;
        Snapshot.Dependencies=B->Dependencies; Snapshot.Assets=B->Assets; Snapshot.Catalogs=B->Catalogs;
        Complete.Add(MoveTemp(Snapshot));
    }
    if (Complete.Num()!=Installed.Num() || Complete.ContainsByPredicate([&](const auto& B)
        { return !Installed.ContainsByPredicate([&](const auto& Old){return Old.BundleId==B.BundleId && Old.Version==B.Version;}); }))
    {
        FString BatchError;
        if (Publish(Complete,BatchError)) return;
    }
    bool Progress=true;
    while (Progress && !Pending.IsEmpty())
    {
        Progress=false;
        for (int32 I=Pending.Num()-1;I>=0;--I)
        {
            auto* B=Pending[I]; bool Ready=true;
            for (const auto& D:B->Dependencies)
                if (!Installed.ContainsByPredicate([&D](const auto& A){return A.BundleId==D.BundleId && A.Version>=D.MinimumVersion;})) { Ready=false; break; }
            if (!Ready) continue;
            FString Error; if (!InstallBundle(B,Error)) UE_LOG(LogTemp,Error,TEXT("Definition bundle %s: %s"),*B->BundleId.ToString(),*Error);
            Pending.RemoveAt(I); Progress=true;
        }
    }
    for (auto* B:Pending) UE_LOG(LogTemp,Error,TEXT("Unresolved definition dependencies for %s"),*B->BundleId.ToString());
}
UDataAsset* UOGDefinitionLibrary::Resolve(FName System,const FOGContentId& Id)
{
    EnsureDiscovered(); const auto* Found=Entries.Find(Key(System,Id)); return Found?Found->Asset:nullptr;
}
void UOGDefinitionLibrary::List(FName System,UClass* Class,TArray<UDataAsset*>& Out)
{
    EnsureDiscovered(); Out.Reset(); TArray<FString> Keys; Entries.GetKeys(Keys); Keys.Sort();
    for (const auto& K:Keys) { const auto& E=Entries[K]; if (E.Entry.System==System && (!Class || E.Asset->IsA(Class))) Out.Add(E.Asset); }
}
bool UOGDefinitionLibrary::LoadCatalog(FName System,const FString& Base,FString& Out,FString& Error)
{
    auto* Library=Get();
    if (!Library) { Out=Base; Error.Reset(); return true; }
    Library->EnsureDiscovered(); const auto* Adapter=Library->CatalogAdapters.Find(System);
    if (!Adapter) return Fail(Error,TEXT("No catalog adapter registered."));
    const auto* More=Library->CatalogFragments.Find(System);
    return MergeCatalogDocuments(Base.IsEmpty()?Adapter->EmptyJson:Base,More?*More:TArray<FString>(),Adapter->Arrays,Out,Error,Adapter->Metadata);
}
