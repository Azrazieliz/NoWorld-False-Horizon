#include "Narrative/OGNarrativeKnowledge.h"

#include "Engine/GameInstance.h"
#include "Narrative/OGNarrativeJson.h"
#include "Narrative/OGNarrativeSessionSubsystem.h"
#include "Runtime/OGGameCoreSubsystem.h"
#include "Runtime/OGGate12RulerSessionSubsystem.h"
#include "UI/OGUiViewModelService.h"

namespace
{
const TCHAR* DiscoveryKeyPrefix = TEXT("noworld.narrative.codex.");
const TCHAR* DiscoverySchema = TEXT("noworld.narrative_discovery.v1");

FName DiscoveryKey(const FOGContentId& EntryId)
{
    return FName(*(FString(DiscoveryKeyPrefix) + EntryId.Value));
}

bool IsCodexCategory(FName Category)
{
    static const TArray<FName> Categories = FOGUiViewModelService::BuildCodex({},FString(),NAME_None).Categories;
    return Categories.Contains(Category);
}
}

namespace OGNarrative
{
bool IsNarrativeDiscoveryId(const FOGContentId& Id)
{
    return Id.IsValid() && Id.Value.Len() <= 256 && !Id.Value.StartsWith(TEXT("diagnostic:"));
}

bool ReadDiscoveryFact(IOGWorldStore& Store, const FOGEntityId& Ruler, const FOGContentId& Id,
    bool& bFound, FOGKnowledgeFactRecord& Out, FString& Error)
{
    return Store.TryReadKnowledgeFact(Ruler,DiscoveryKey(Id),FOGEntityId(),bFound,Out,Error);
}

bool ValidateDiscoveries(const TArray<FOGNarrativeDiscovery>& Discoveries, const FString& At, FString& Error)
{
    if (Discoveries.Num() > 64) return Fail(Error,At,TEXT("too many discoveries"));
    TSet<FOGContentId> Seen;
    for (const FOGNarrativeDiscovery& Discovery : Discoveries)
    {
        if (!IsNarrativeDiscoveryId(Discovery.EntryId) || Seen.Contains(Discovery.EntryId) ||
            !IsCodexCategory(Discovery.Category) || Discovery.DisplayName.TrimStartAndEnd().IsEmpty() ||
            Discovery.Summary.TrimStartAndEnd().IsEmpty() ||
            (Discovery.BeliefState != TEXT("confirmed") && Discovery.BeliefState != TEXT("estimated") &&
                Discovery.BeliefState != TEXT("rumor")) ||
            Discovery.ConfidenceBps < 0 || Discovery.ConfidenceBps > 10000)
            return Fail(Error,At,TEXT("invalid or duplicate Codex discovery"));
        Seen.Add(Discovery.EntryId);
    }
    return true;
}

bool RecordDiscoveries(IOGWorldStore& Store, const TArray<FOGNarrativeDiscovery>& Discoveries,
    const FOGEntityId& Ruler, const FOGEntityId& Source, const FOGEntityId& Receipt, int64 Tick, FString& Error)
{
    for (const FOGNarrativeDiscovery& Discovery : Discoveries)
    {
        FOGKnowledgeFactRecord Fact;
        bool bFound = false;
        // A content entry can be learned from several scenes. Preserve the
        // first committed excerpt and its provenance for this Ruler.
        if (!ReadDiscoveryFact(Store,Ruler,Discovery.EntryId,bFound,Fact,Error)) return false;
        if (bFound) continue;

        FObject Value = MakeShared<FJsonObject>();
        Value->SetStringField(TEXT("schema"),DiscoverySchema);
        Value->SetStringField(TEXT("entry_id"),Discovery.EntryId.Value);
        Value->SetStringField(TEXT("category"),Discovery.Category.ToString());
        Value->SetStringField(TEXT("display_name"),Discovery.DisplayName);
        Value->SetStringField(TEXT("summary"),Discovery.Summary);

        Fact = FOGKnowledgeFactRecord();
        Fact.OwnerEntityId = Ruler;
        Fact.FactKey = DiscoveryKey(Discovery.EntryId);
        Fact.ValueJson = Write(Value);
        Fact.LearnedWorldTick = Tick;
        Fact.UpdatedWorldTick = Tick;
        Fact.BeliefState = Discovery.BeliefState;
        Fact.ConfidenceBps = Discovery.ConfidenceBps;
        Fact.SourceEntityId = Source;
        Fact.SourceEventId = Receipt;
        Fact.bHasEvidenceWorldTick = true;
        Fact.EvidenceWorldTick = Tick;
        if (!Store.UpsertKnowledgeFact(Fact,Error)) return false;
    }
    return true;
}
}

bool UOGNarrativeSessionSubsystem::ReadCodexDiscoveries(const FString& SearchText, FName CategoryFilter,
    TArray<FOGNarrativeCodexEntry>& Out, FString& Error) const
{
    using namespace OGNarrative;
    Out.Reset();
    Error.Reset();
    const UGameInstance* Game = GetGameInstance();
    const UOGGameCoreSubsystem* Core = BoundCore.Get();
    const auto* Ruler = Game ? Game->GetSubsystem<UOGGate12RulerSessionSubsystem>() : nullptr;
    IOGWorldStore* Store = Core ? Core->GetWorldStore() : nullptr;
    if (!Core || !Core->IsCoreReady() || !Store || !Store->IsOpen() || Store->IsWriteTransactionActive() ||
        !Ruler || !Ruler->IsBoundToStoredRuler())
        return Fail(Error,TEXT("Codex"),TEXT("canonical Ruler knowledge is unavailable"));

    TArray<FOGKnowledgeFactRecord> Facts;
    if (!Store->ListKnowledgeFactsByOwner(Ruler->BoundRulerId,Facts,Error)) return false;

    const int64 Now = Core->GetCanonicalWorldTick();
    TArray<FOGCodexEntryViewModel> KnownEntries;
    TMap<FOGContentId,FString> KnownSummaries;
    for (const FOGKnowledgeFactRecord& Fact : Facts)
    {
        if (!Fact.FactKey.ToString().StartsWith(DiscoveryKeyPrefix) || Fact.SubjectEntityId.IsValid() ||
            !Fact.SourceEventId.IsValid() || Fact.LearnedWorldTick > Now || Fact.UpdatedWorldTick > Now)
            continue;

        FObject Value;
        FString ParseError, Schema, Id, Category;
        FOGCodexEntryViewModel Entry;
        if (!ReadObject(Fact.ValueJson,Value,ParseError) ||
            !Value->TryGetStringField(TEXT("schema"),Schema) || Schema != DiscoverySchema ||
            !Value->TryGetStringField(TEXT("entry_id"),Id) ||
            !Value->TryGetStringField(TEXT("category"),Category) ||
            !Value->TryGetStringField(TEXT("display_name"),Entry.DisplayNameKey) || Entry.DisplayNameKey.IsEmpty())
            continue;

        Entry.EntryId = FOGContentId(MoveTemp(Id));
        Entry.Category = FName(*Category);
        if (!IsNarrativeDiscoveryId(Entry.EntryId) ||
            Fact.FactKey != DiscoveryKey(Entry.EntryId) || !IsCodexCategory(Entry.Category))
            continue;

        const FOGKnowledgeFactViewModel Knowledge = FOGUiViewModelService::ProjectKnowledgeFact(&Fact,false);
        Entry.KnowledgeState = Knowledge.KnowledgeState;
        FString Summary;
        if (Knowledge.bExactValueVisible && Value->TryGetStringField(TEXT("summary"),Summary))
            KnownSummaries.Add(Entry.EntryId,MoveTemp(Summary));
        KnownEntries.Add(MoveTemp(Entry));
    }

    FOGCodexViewModel Codex = FOGUiViewModelService::BuildCodex(KnownEntries,SearchText,CategoryFilter);
    for (FOGCodexEntryViewModel& Entry : Codex.VisibleEntries)
    {
        FOGNarrativeCodexEntry& View = Out.AddDefaulted_GetRef();
        View.Summary = KnownSummaries.FindRef(Entry.EntryId);
        View.Entry = MoveTemp(Entry);
    }
    return true;
}
