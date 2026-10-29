#include "Runtime/OGGameCoreSubsystem.h"
#include "Runtime/OGGate12RulerSessionSubsystem.h"
#include "Engine/GameInstance.h"
#include "Combat/OGNativeEncounterDefinition.h"
#include "Combat/OGNativeTurnAgentComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Combat/OGCanonicalCombatActorBindingComponent.h"
#include "Combat/OGWorldAbilityExecutionComponent.h"
#include "Characters/OGCharacterGameplayDefinition.h"
#include "Dom/JsonObject.h"
#include "Misc/Base64.h"
#include "Misc/SecureHash.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"
#include "Serialization/MemoryReader.h"
#include "Serialization/MemoryWriter.h"
#include "Serialization/ObjectAndNameAsStringProxyArchive.h"

namespace
{
FOGEntityId RecoveryId(const FOGEntityId& Ruler)
{
    FGuid Id; FGuid::ParseExact(FMD5::HashAnsiString(*(TEXT("native.encounter.recovery.v1|")+Ruler.ToString())),EGuidFormats::Digits,Id);
    return FOGEntityId(Id);
}
bool ReadId(const FJsonObject& O,const TCHAR* Key,FOGEntityId& Out)
{
    FString S; FGuid Id;
    if (!O.TryGetStringField(Key,S) || !FGuid::Parse(S,Id) || !Id.IsValid()) return false;
    Out=FOGEntityId(Id); return true;
}
}
bool UOGGameCoreSubsystem::PersistNativeEncounter(const FOGTurnBattle& Candidate,const FOGEntityId& Ruler,
    const FOGWorldEvent& Receipt,const FOGResolvedCombatAction* Root,bool Opening,FString& Error)
{
    const bool Supported=Opening?bPendingNativeRecovery:bActiveNativeRecovery;
    if (!WorldStore || !Ruler.IsValid() || !Receipt.EventId.IsValid() || Receipt.WorldTick<0)
    { Error=TEXT("Encounter recovery requires its exact committed owner and receipt."); return false; }
    auto Record=MakeShared<FJsonObject>();
    Record->SetStringField(TEXT("schema"),TEXT("noworld.native_encounter_recovery.v1"));
    Record->SetStringField(TEXT("ruler_id"),Ruler.ToString());
    Record->SetStringField(TEXT("battle_id"),Candidate.GetState().BattleId.ToString());
    Record->SetStringField(TEXT("receipt_id"),Receipt.EventId.ToString());
    Record->SetStringField(TEXT("world_tick_exact"),LexToString(Receipt.WorldTick));
    FOGEntityId Controlled=Ruler;
    if (const auto* Session=GetGameInstance()?GetGameInstance()->GetSubsystem<UOGGate12RulerSessionSubsystem>():nullptr;
        Session && Session->BoundRulerId==Ruler && Session->SelectedManifestationId.IsValid())
        Controlled=Session->SelectedManifestationId;
    Record->SetStringField(TEXT("controlled_entity_id"),Controlled.ToString());
    const bool Complete=Candidate.GetState().Status==EOGTurnBattleStatus::Completed;
    Record->SetStringField(TEXT("state"),Complete?TEXT("completed"):(Supported?TEXT("available"):TEXT("provider_required")));
    if (Supported && !Complete)
    {
        const FName Provider=Opening?PendingNativeRecoveryProvider:ActiveNativeRecoveryProvider;
        int32 ProviderVersion=0; FString ProviderPayload;
        if (!CaptureCombatRecoveryProvider(Provider,Candidate,Ruler,ProviderVersion,ProviderPayload,Error)) return false;
        Record->SetStringField(TEXT("recovery_provider"),Provider.ToString());
        Record->SetNumberField(TEXT("provider_version"),ProviderVersion);
        Record->SetStringField(TEXT("provider_payload"),ProviderPayload);
        TArray<uint8> Bytes; if (!Candidate.ExportNativeCheckpoint(Bytes,Error)) return false;
        Record->SetStringField(TEXT("runtime_base64"),FBase64::Encode(Bytes));
        Record->SetStringField(TEXT("rank_base64"),Opening?PendingNativeRankRecovery:ActiveNativeRankRecovery);
        TSet<FOGEntityId> Seen=Opening?TSet<FOGEntityId>():CommittedNativeActionIds;
        if (Root && Root->ActionId.IsValid()) Seen.Add(Root->ActionId);
        for (const auto& R:Candidate.GetLastNativeReactions()) if (R.bExecuted) Seen.Add(R.Action.ActionId);
        TArray<FString> Sorted; for (const auto& Id:Seen) Sorted.Add(Id.ToString()); Sorted.Sort();
        TArray<TSharedPtr<FJsonValue>> Ids; for (const auto& Id:Sorted) Ids.Add(MakeShared<FJsonValueString>(Id));
        Record->SetArrayField(TEXT("committed_action_ids"),MoveTemp(Ids));
        FOGWorldEncounterPresentationCheckpoint Physical;
        if (!Opening && bNativeRecoveryAwaitingWorld) Physical=NativeWorldRecoveryPresentation;
        else
        {
            for (const auto& Unit:Candidate.GetState().Units)
                if (const auto* Actor=UOGCanonicalCombatActorBindingComponent::FindActualWorldActor(
                    GetWorld(),Candidate.GetState().BattleId,Unit.UnitEntityId))
                {
                    FOGCombatBodyCheckpoint Body; Body.UnitId=Unit.UnitEntityId; Body.Transform=Actor->GetActorTransform();
                    if (const auto* Character=Cast<ACharacter>(Actor))
                    {
                        const auto* Movement=Character->GetCharacterMovement(); Body.bCharacterMovement=true;
                        Body.Velocity=Movement->Velocity; Body.MovementMode=Movement->MovementMode; Body.CustomMovementMode=Movement->CustomMovementMode;
                    }
                    Physical.Bodies.Add(Body);
                }
            TArray<FOGWorldAbilityCastView> Casts; Candidate.GetWorldCasts(Casts);
            for (const auto& Cast:Casts)
            {
                FOGWorldCastPresentationCheckpoint Saved; Saved.CastId=Cast.CastId;
                const auto* Actor=UOGCanonicalCombatActorBindingComponent::FindActualWorldActor(
                    GetWorld(),Candidate.GetState().BattleId,Cast.SourceUnitId);
                const auto* Runner=Actor?Actor->FindComponentByClass<UOGWorldAbilityExecutionComponent>():nullptr;
                if (Runner && Runner->GetActiveCastId()==Cast.CastId) Runner->CapturePresentationCheckpoint(Saved);
                Physical.Casts.Add(MoveTemp(Saved));
            }
        }
        TArray<uint8> Geometry; FMemoryWriter Memory(Geometry,true); FObjectAndNameAsStringProxyArchive Archive(Memory,false);
        FOGWorldEncounterPresentationCheckpoint::StaticStruct()->SerializeItem(Archive,&Physical,nullptr);
        if (Archive.IsError()) { Error=TEXT("Cannot retain native encounter physical state."); return false; }
        Record->SetStringField(TEXT("presentation_base64"),FBase64::Encode(Geometry));
    }
    FString Json;
    if (!FJsonSerializer::Serialize(Record.ToSharedRef(),TJsonWriterFactory<>::Create(&Json)))
    { Error=TEXT("Cannot encode native encounter recovery record."); return false; }
    return WorldStore->UpsertEntity(RecoveryId(Ruler),TEXT("native_encounter_recovery"),Receipt.WorldTick,Json,Error);
}
bool UOGGameCoreSubsystem::CheckpointOwnedNativeEncounter(FString& Error)
{
    Error.Reset();
    if (!OwnedLuminousTurnBattle) return true;
    if (bSubmittingNativeAction) { Error=TEXT("Encounter checkpoint waits for the current action transaction."); return false; }
    if (!WorldStore || !WorldStore->IsOpen() || !LastNativeBattleReceipt.IsValid())
    { Error=TEXT("No committed native receipt is available for encounter recovery."); return false; }
    FOGWorldEvent Receipt; Receipt.EventId=LastNativeBattleReceipt;
    Receipt.WorldTick=CanonicalClock?CanonicalClock->GetCanonicalWorldTick():-1;
    if (!WorldStore->BeginTransaction(Error)) return false;
    if (!PersistNativeEncounter(*OwnedLuminousTurnBattle,ActiveCombatRulerId,Receipt,nullptr,false,Error) ||
        !WorldStore->CommitTransaction(Error))
    { FString Ignored; WorldStore->RollbackTransaction(Ignored); return false; }
    return true;
}
bool UOGGameCoreSubsystem::RestoreOwnedNativeEncounter(const FOGEntityId& Ruler,bool& Restored,FString& Error)
{
    Restored=false; Error.Reset();
    if (!IsInGameThread() || bSubmittingNativeAction || !bCoreReady || bReleasingCanonicalRuntime ||
        !WorldStore || !WorldStore->IsOpen() || !Ruler.IsValid())
    { Error=TEXT("Encounter restore requires the live canonical World and stored Ruler."); return false; }
    if (OwnedLuminousTurnBattle)
    {
        if (ActiveCombatRulerId!=Ruler) { Error=TEXT("Another Ruler owns the current native encounter."); return false; }
        return true;
    }
    TGuardValue<bool> Guard(bSubmittingNativeAction,true);
    PendingNativeRecoveryRuler=Ruler;
    bool Found=false; FName Kind; FString Json; int64 Revision=0;
    if (!WorldStore->TryReadEntity(Ruler,Found,Kind,Json,Revision,Error)) return false;
    if (!Found || Kind!=TEXT("ruler")) { Error=TEXT("Recovery owner is not an existing canonical Ruler."); return false; }
    if (!WorldStore->TryReadEntity(RecoveryId(Ruler),Found,Kind,Json,Revision,Error)) return false;
    if (!Found) return true;
    TSharedPtr<FJsonObject> Record; FString Schema,State,Runtime,Rank,Tick;
    FOGEntityId Owner,BattleId,Receipt,Controlled;
    if (Kind!=TEXT("native_encounter_recovery") || !FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Json),Record) ||
        !Record || !Record->TryGetStringField(TEXT("schema"),Schema) || Schema!=TEXT("noworld.native_encounter_recovery.v1") ||
        !ReadId(*Record,TEXT("ruler_id"),Owner) || Owner!=Ruler || !ReadId(*Record,TEXT("battle_id"),BattleId) ||
        !ReadId(*Record,TEXT("receipt_id"),Receipt) || !ReadId(*Record,TEXT("controlled_entity_id"),Controlled) ||
        !Record->TryGetStringField(TEXT("state"),State) ||
        !Record->TryGetStringField(TEXT("world_tick_exact"),Tick) || Tick!=LexToString(Revision) || Revision>GetCanonicalWorldTick())
    { Error=TEXT("Native encounter recovery record is inconsistent; original bytes preserved."); return false; }
    if (State==TEXT("completed")) { NativeEncounterRecoveryStatus=TEXT("Previous encounter completed; no opening or rewards replayed."); return true; }
    if (State==TEXT("provider_required"))
    { NativeEncounterRecoveryStatus=TEXT("Previous encounter requires its specialized native recovery provider."); return true; }
    if (State!=TEXT("available") || !Record->TryGetStringField(TEXT("runtime_base64"),Runtime) ||
        Runtime.Len()>24*1024*1024 || !Record->TryGetStringField(TEXT("rank_base64"),Rank) || Rank.Len()>1024*1024)
    { Error=TEXT("Native encounter recovery payload is absent or invalid."); return false; }
    FOGRankSuppressionResolver RankResolver;
    if (!Rank.IsEmpty())
    {
        TArray<uint8> Data; if (!FBase64::Decode(Rank,Data)) { Error=TEXT("Encounter rank checkpoint is invalid."); return false; }
        FMemoryReader Memory(Data,true); FObjectAndNameAsStringProxyArchive Archive(Memory,false);
        FOGEncounterRankRules Rules; FOGEncounterRankRules::StaticStruct()->SerializeItem(Archive,&Rules,nullptr);
        if (Archive.IsError() || Memory.Tell()!=Memory.TotalSize() || !Rules.Validate(Error)) return false;
        RankResolver=[Rules](const FOGRankSuppressionQuery& Q,int32& Bps,FString& E){return Rules.Resolve(Q,Bps,E);};
    }
    TArray<uint8> Bytes; if (!FBase64::Decode(Runtime,Bytes)) { Error=TEXT("Encounter runtime checkpoint is invalid."); return false; }
    FString ProviderName=TEXT("core.native"),ProviderPayload; int32 ProviderVersion=1;
    if (Record->HasField(TEXT("recovery_provider")))
    {
        double VersionValue=0;
        if (!Record->TryGetStringField(TEXT("recovery_provider"),ProviderName) || ProviderName.IsEmpty() ||
            !Record->TryGetNumberField(TEXT("provider_version"),VersionValue) || !FMath::IsFinite(VersionValue) ||
            VersionValue<1 || VersionValue>MAX_int32 || FMath::TruncToDouble(VersionValue)!=VersionValue ||
            !Record->TryGetStringField(TEXT("provider_payload"),ProviderPayload))
        { Error=TEXT("Native callback recovery descriptor is invalid."); return false; }
        ProviderVersion=static_cast<int32>(VersionValue);
    }
    const FName Provider(*ProviderName); FOGCombatRecoveryBindings Bindings;
    if (!RestoreCombatRecoveryProvider(Provider,ProviderVersion,ProviderPayload,Ruler,Bindings,Error)) return false;
    if (!Rank.IsEmpty()) Bindings.Rank=MoveTemp(RankResolver); // Keep the original typed profile when one was admitted.
    auto Candidate=MakeUnique<FOGTurnBattle>();
    if (!Candidate->RestoreNativeCheckpointWithBindings(Bytes,MoveTemp(Bindings),Error) || Candidate->GetState().BattleId!=BattleId)
    { if (Error.IsEmpty()) Error=TEXT("Encounter recovery identity differs."); return false; }
    int32 Ranked=0;
    for (const auto& Unit:Candidate->GetState().Units)
    {
        if (Unit.RankProjection.RankId.IsValid()) ++Ranked;
        if (!WorldStore->TryReadEntity(Unit.UnitEntityId,Found,Kind,Json,Revision,Error)) return false;
        if (!Found || Json.Contains(TEXT("\"diagnosticFixture\"")))
        { Error=TEXT("Recovered encounter participant has no actual canonical entity."); return false; }
        const bool SpecializedUnit=Unit.IdentityId==FOGContentId(TEXT("character:luminous_valentine.identity"));
        const UOGCharacterGameplayDefinition* Kit=nullptr;
        if (!SpecializedUnit)
        {
            Kit=UOGCharacterGameplayDefinition::Find(Unit.IdentityId,Unit.SkillSet.CurrentForm,Error);
            if (!Kit || !Kit->MatchesKit(Unit.SkillSet))
            { if (Error.IsEmpty()) Error=TEXT("Recovered participant's installed kit changed; checkpoint preserved."); return false; }
        } // Specialized binding above validates its original source kit and actual owned Manifestation.
        bool HasCopy=false; FOGCharacterManifestationRecord Copy;
        if (!WorldStore->TryReadCharacterManifestation(Unit.UnitEntityId,HasCopy,Copy,Error)) return false;
        if (HasCopy && (Copy.IdentityId!=Unit.IdentityId || Copy.ActiveVersionId!=Unit.SkillSet.CurrentForm ||
            Copy.LifecycleState!=TEXT("active")))
        { Error=TEXT("Recovered Manifestation no longer matches its canonical identity/version/lifecycle."); return false; }
        if (!HasCopy && Unit.UnitEntityId!=Ruler)
        {
            auto* Library=UOGDefinitionLibrary::Get();
            const auto* Plan=Library?Cast<UOGNativeEncounterDefinition>(Library->Resolve(TEXT("encounter"),Candidate->GetState().EncounterDefinitionId)):nullptr;
            if (!Plan) { Error=TEXT("Recovered NPC encounter requires its original shared plan."); return false; }
            FName IdentityField=Plan->CanonicalIdentityField,VersionField=Plan->CanonicalVersionField;
            if (Kit && Kit->TurnPolicy.Id.IsValid())
            {
                const auto* Policy=Cast<UOGNativeTurnPolicy>(Library->Resolve(Kit->TurnPolicy.System,Kit->TurnPolicy.Id));
                if (!Policy || !Policy->Validate(Error)) return false;
                IdentityField=Policy->CanonicalIdentityField; VersionField=Policy->CanonicalVersionField;
            }
            TSharedPtr<FJsonObject> Entity; FString Identity,Version;
            if (!FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Json),Entity) || !Entity ||
                !Entity->TryGetStringField(IdentityField.ToString(),Identity) || Identity!=Unit.IdentityId.ToString() ||
                !Entity->TryGetStringField(VersionField.ToString(),Version) || Version!=Unit.SkillSet.CurrentForm.ToString())
            { Error=TEXT("Recovered NPC no longer matches its actual canonical identity/version."); return false; }
        }
    }
    if (Ranked>1 && Rank.IsEmpty() && Provider==TEXT("core.native"))
    { Error=TEXT("Ranked encounter recovery has no original suppression profile."); return false; }
    const TArray<TSharedPtr<FJsonValue>>* SavedIds=nullptr; TSet<FOGEntityId> Seen;
    if (!Record->TryGetArrayField(TEXT("committed_action_ids"),SavedIds)) { Error=TEXT("Encounter request receipts are absent."); return false; }
    for (const auto& Value:*SavedIds)
    {
        FGuid Id; FString Text;
        if (!Value || !Value->TryGetString(Text) || !FGuid::Parse(Text,Id) || !Id.IsValid() || Seen.Contains(FOGEntityId(Id)))
        { Error=TEXT("Encounter request receipt is invalid or repeated."); return false; }
        Seen.Add(FOGEntityId(Id));
    }
    if (Candidate->GetState().ExecutionMode==EOGCombatExecutionMode::World &&
        !ValidateOwnedWorldParticipants(Candidate->GetState(),Ruler,Error)) return false;
    FString Presentation; TArray<uint8> Geometry; FOGWorldEncounterPresentationCheckpoint Physical;
    if (!Record->TryGetStringField(TEXT("presentation_base64"),Presentation) || Presentation.Len()>8*1024*1024 ||
        !FBase64::Decode(Presentation,Geometry))
    { Error=TEXT("Encounter physical checkpoint is absent or invalid."); return false; }
    FMemoryReader GeometryMemory(Geometry,true); FObjectAndNameAsStringProxyArchive GeometryArchive(GeometryMemory,false);
    FOGWorldEncounterPresentationCheckpoint::StaticStruct()->SerializeItem(GeometryArchive,&Physical,nullptr);
    if (GeometryArchive.IsError() || GeometryMemory.Tell()!=GeometryMemory.TotalSize())
    { Error=TEXT("Encounter physical checkpoint could not be restored."); return false; }
    TSet<FOGEntityId> Bodies,PhysicalCasts;
    for (const auto& Body:Physical.Bodies)
    {
        if (!Candidate->GetState().Units.ContainsByPredicate([&](const auto& U){return U.UnitEntityId==Body.UnitId;}) ||
            Bodies.Contains(Body.UnitId) || Body.Transform.ContainsNaN() || !Body.Transform.GetRotation().IsNormalized() ||
            Body.Velocity.ContainsNaN() || (Body.bCharacterMovement && Body.MovementMode>=MOVE_MAX))
        { Error=TEXT("Encounter physical body checkpoint is inconsistent."); return false; }
        Bodies.Add(Body.UnitId);
    }
    for (const auto& Cast:Physical.Casts)
    {
        FOGWorldAbilityCastView View;
        if (PhysicalCasts.Contains(Cast.CastId) || !Candidate->ReadWorldCast(Cast.CastId,View))
        { Error=TEXT("Encounter physical cast checkpoint is inconsistent."); return false; }
        PhysicalCasts.Add(Cast.CastId);
    }
    NativeWorldCastRequests.Reset();
    TArray<FOGWorldAbilityCastView> Casts; Candidate->GetWorldCasts(Casts);
    for (const auto& Cast:Casts)
    {
        if (!PhysicalCasts.Contains(Cast.CastId)) { Error=TEXT("Accepted cast physical checkpoint is missing."); return false; }
        FOGResolvedCombatAction Root; FOGWorldAbilitySpecification Spec; TSet<int32> Completed; int64 Started=0;
        if (!Candidate->ReadWorldCastRecovery(Cast.CastId,Root,Spec,Completed,Started)) return false;
        NativeWorldCastRequests.Add(Cast.CastId,MoveTemp(Root));
    }
    OwnedLuminousTurnBattle=MoveTemp(Candidate); ActiveCombatRulerId=Ruler;
    CommittedNativeActionIds=MoveTemp(Seen); LastNativeBattleReceipt=Receipt; LastNativeOpeningReceipt=FOGEntityId();
    LastNativeCombatEvent=FOGNativeCombatEventProvenance(); LastNativeBattleOutcome=FOGNativeBattleOutcome();
    bActiveNativeRecovery=true; bNativeEncounterRestored=true; ActiveNativeRecoveryProvider=Provider; ActiveNativeRankRecovery=Rank;
    NativeWorldRecoveryPresentation=MoveTemp(Physical); bNativeRecoveryBodiesRestored=false;
    bNativeRecoveryAwaitingWorld=OwnedLuminousTurnBattle->GetState().ExecutionMode==EOGCombatExecutionMode::World;
    LastWorldCombatSeconds=-1; WorldCombatFractionalMilliseconds=0; WorldCombatClockWorld.Reset();
    NativeEncounterRecoveryStatus=bNativeRecoveryAwaitingWorld
        ? TEXT("Encounter recovered; waiting for its real World bodies and cast runners.")
        : TEXT("Turn encounter recovered at its last committed action.");
    PendingNativeRecoveryRuler=FOGEntityId(); Restored=true;
    if (auto* Session=GetGameInstance()?GetGameInstance()->GetSubsystem<UOGGate12RulerSessionSubsystem>():nullptr;
        Session && Session->BoundRulerId==Ruler)
    {
        FString SelectionError; bool Selected=false;
        if (Controlled==Ruler) Selected=Session->SelectRulerForWorld(SelectionError);
        else
        {
            bool HasCopy=false; FOGCharacterManifestationRecord Copy;
            if (WorldStore->TryReadCharacterManifestation(Controlled,HasCopy,Copy,SelectionError) && HasCopy && Copy.OwningRulerId==Ruler)
                Selected=Session->SelectOwnedManifestation(Copy.IdentityId,Controlled,SelectionError);
        }
        if (!Selected) NativeEncounterRecoveryStatus+=TEXT(" Select an available owned character to continue.");
    }
    // Restore is a presentation refresh, not a new combat action or World event.
    const auto RestoredState=OwnedLuminousTurnBattle->GetState();
    OnNativeBattleStateCommitted.Broadcast(RestoredState,Receipt);
    return true;
}
bool UOGGameCoreSubsystem::CompleteNativeWorldRecovery(FString& Error)
{
    Error.Reset();
    if (!IsInGameThread() || bSubmittingNativeAction || !bCoreReady || bReleasingCanonicalRuntime)
    { Error=TEXT("World recovery waits for the settled native game thread."); return false; }
    if (!bNativeRecoveryAwaitingWorld) return true;
    if (!OwnedLuminousTurnBattle || !GetWorld()) { Error=TEXT("Recovered World is not available."); return false; }
    const auto& State=OwnedLuminousTurnBattle->GetState();
    for (const auto& Unit:State.Units) if (Unit.CanAct())
        if (!UOGCanonicalCombatActorBindingComponent::FindActualWorldActor(GetWorld(),State.BattleId,Unit.UnitEntityId))
        { Error=TEXT("Recovered encounter waits for its actual participant bodies."); return false; }
    for (const auto& Cast:NativeWorldRecoveryPresentation.Casts)
    {
        FOGWorldAbilityCastView View;
        if (!OwnedLuminousTurnBattle->ReadWorldCast(Cast.CastId,View)) { Error=TEXT("Recovered native cast disappeared before physical reconnection."); return false; }
        auto* Actor=UOGCanonicalCombatActorBindingComponent::FindActualWorldActor(GetWorld(),State.BattleId,View.SourceUnitId);
        auto* Runner=Actor?Actor->FindComponentByClass<UOGWorldAbilityExecutionComponent>():nullptr;
        if (!Runner) { Error=TEXT("Recovered encounter waits for its native cast runner."); return false; }
        if (!Runner->CanRecoverAcceptedCast(Cast,Error)) return false;
    }
    for (const auto& Cast:NativeWorldRecoveryPresentation.Casts)
    {
        FOGWorldAbilityCastView View; OwnedLuminousTurnBattle->ReadWorldCast(Cast.CastId,View);
        auto* Actor=UOGCanonicalCombatActorBindingComponent::FindActualWorldActor(GetWorld(),State.BattleId,View.SourceUnitId);
        auto* Runner=Actor?Actor->FindComponentByClass<UOGWorldAbilityExecutionComponent>():nullptr;
        if (!Runner || !Runner->RecoverAcceptedCast(Cast,Error)) return false;
    }
    if (!bNativeRecoveryBodiesRestored)
    {
        for (const auto& Body:NativeWorldRecoveryPresentation.Bodies)
            if (auto* Actor=UOGCanonicalCombatActorBindingComponent::FindActualWorldActor(GetWorld(),State.BattleId,Body.UnitId))
            {
                Actor->SetActorTransform(Body.Transform,false,nullptr,ETeleportType::TeleportPhysics);
                if (Body.bCharacterMovement) if (auto* Character=Cast<ACharacter>(Actor))
                {
                    auto* Movement=Character->GetCharacterMovement();
                    Movement->SetMovementMode(static_cast<EMovementMode>(Body.MovementMode),Body.CustomMovementMode); Movement->Velocity=Body.Velocity;
                }
            }
        bNativeRecoveryBodiesRestored=true;
    }
    bNativeRecoveryAwaitingWorld=false; LastWorldCombatSeconds=-1; WorldCombatFractionalMilliseconds=0;
    for (const auto& Cast:NativeWorldRecoveryPresentation.Casts)
    {
        FOGWorldAbilityCastView View; OwnedLuminousTurnBattle->ReadWorldCast(Cast.CastId,View);
        auto* Actor=UOGCanonicalCombatActorBindingComponent::FindActualWorldActor(GetWorld(),State.BattleId,View.SourceUnitId);
        if (auto* Runner=Actor?Actor->FindComponentByClass<UOGWorldAbilityExecutionComponent>():nullptr)
        { FString PresentationError; Runner->RecoverAcceptedCast(Cast,PresentationError); }
    }
    NativeEncounterRecoveryStatus=TEXT("World encounter recovered; native bodies and cast runners reconnected.");
    return true;
}
