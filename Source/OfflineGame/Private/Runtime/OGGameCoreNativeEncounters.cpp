#include "Runtime/OGGameCoreSubsystem.h"
#include "Combat/OGNativeEncounterDefinition.h"
#include "Combat/OGNativeTurnAgentComponent.h"
#include "Characters/OGCharacterGameplayDefinition.h"
#include "Progression/OGRankService.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/MemoryWriter.h"
#include "Serialization/ObjectAndNameAsStringProxyArchive.h"
#include "Misc/Base64.h"

bool UOGGameCoreSubsystem::StartOwnedNativeEncounter(const UOGNativeEncounterDefinition& Definition,
    const TArray<FOGResolvedEncounterParticipant>& Inputs,const FOGEntityId& Ruler,
    FOGIdentityExclusivityContext IdentityContext,FString& Error,FName RecoveryProvider)
{
    Error.Reset();
    if (!IsInGameThread() || bSubmittingNativeAction || !bCoreReady || bReleasingCanonicalRuntime ||
        !WorldStore || !WorldStore->IsOpen() || !Ruler.IsValid() || !Definition.ValidateDefinition(Error))
    { if (Error.IsEmpty()) Error=TEXT("Native encounter requires the canonical World and a valid shared plan."); return false; }
    auto* Library=UOGDefinitionLibrary::Get(); auto Participants=Inputs;
    for (auto& Input:Participants)
    {
        auto& Unit=Input.Unit; bool Exists=false; FName Kind; FString Json; int64 Revision=0;
        if (!WorldStore->TryReadEntity(Unit.UnitEntityId,Exists,Kind,Json,Revision,Error)) return false;
        TSharedPtr<FJsonObject> Entity;
        if (!Exists || !FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Json),Entity) ||
            !Entity || Entity->HasField(TEXT("diagnosticFixture")))
        { Error=TEXT("Encounter participant has no existing canonical entity."); return false; }
        const auto* Kit=UOGCharacterGameplayDefinition::Find(Unit.IdentityId,Unit.SkillSet.CurrentForm,Error);
        if (!Kit) return false;
        bool HasCopy=false; FOGCharacterManifestationRecord Copy;
        if (!WorldStore->TryReadCharacterManifestation(Unit.UnitEntityId,HasCopy,Copy,Error)) return false;
        if (HasCopy)
        {
            if (Copy.IdentityId!=Unit.IdentityId || Copy.ActiveVersionId!=Unit.SkillSet.CurrentForm || Copy.LifecycleState!=TEXT("active"))
            { Error=TEXT("Encounter unit does not match its actual current Manifestation."); return false; }
        }
        else if (Unit.UnitEntityId==Ruler)
        {
            if (Kind!=TEXT("ruler")) { Error=TEXT("The Ruler encounter unit has a different canonical entity kind."); return false; }
        }
        else
        {
            FName IdentityField=Definition.CanonicalIdentityField,VersionField=Definition.CanonicalVersionField;
            if (Kit->TurnPolicy.Id.IsValid())
            {
                const auto* Policy=Library?Cast<UOGNativeTurnPolicy>(Library->Resolve(Kit->TurnPolicy.System,Kit->TurnPolicy.Id)):nullptr;
                if (!Policy || !Policy->Validate(Error)) { if (Error.IsEmpty()) Error=TEXT("Canonical NPC policy is not installed."); return false; }
                IdentityField=Policy->CanonicalIdentityField; VersionField=Policy->CanonicalVersionField;
            }
            FString Identity,Version;
            if (!Entity->TryGetStringField(IdentityField.ToString(),Identity) || Identity!=Unit.IdentityId.ToString() ||
                !Entity->TryGetStringField(VersionField.ToString(),Version) || Version!=Unit.SkillSet.CurrentForm.ToString())
            { Error=TEXT("NPC encounter source differs from its canonical identity/version fields."); return false; }
        }
        Unit.SkillSet=Kit->Skills; bool HasRank=false;
        if (!FOGRankService(*WorldStore).ResolveEffectiveRank(Unit.UnitEntityId,HasRank,Unit.RankProjection,Error)) return false;
        if (!HasRank) Unit.RankProjection=FOGResolvedRankProjection();
    }
    FOGTurnBattleState State;
    if (!Definition.Assemble(Participants,FOGEntityId::NewId(),State,Error)) return false;
    FOGRankSuppressionResolver RankResolver;
    FString RankCheckpoint;
    if (Definition.RankRules.Id.IsValid())
    {
        const auto* Tuning=Library?Cast<UOGEncounterRankDefinition>(Library->Resolve(Definition.RankRules.System,Definition.RankRules.Id)):nullptr;
        if (!Tuning || !Tuning->ValidateDefinition(Error)) { if (Error.IsEmpty()) Error=TEXT("Shared encounter rank tuning is missing."); return false; }
        auto RulesSnapshot=Tuning->Rules; TArray<uint8> RankBytes;
        FMemoryWriter Memory(RankBytes,true); FObjectAndNameAsStringProxyArchive Archive(Memory,false);
        FOGEncounterRankRules::StaticStruct()->SerializeItem(Archive,&RulesSnapshot,nullptr);
        if (Archive.IsError()) { Error=TEXT("Cannot retain the exact encounter rank profile."); return false; }
        RankCheckpoint=FBase64::Encode(RankBytes);
        RankResolver=[Rules=Tuning->Rules](const FOGRankSuppressionQuery& Query,int32& Multiplier,FString& Failure)
        { return Rules.Resolve(Query,Multiplier,Failure); };
    }
    int32 Ranked=0; for (const auto& Unit:State.Units) if (Unit.RankProjection.RankId.IsValid()) ++Ranked;
    if (Ranked>1 && !RankResolver) { Error=TEXT("Ranked encounter participants require their original shared suppression profile."); return false; }
    TGuardValue<FString> RankScope(PendingNativeRankRecovery,RankCheckpoint);
    return StartOwnedLuminousTurnBattle(State,{}, {},MoveTemp(IdentityContext),MoveTemp(RankResolver),Ruler,{},Error,RecoveryProvider);
}
