#include "Combat/OGNativeEncounterDefinition.h"
#include "Combat/OGCanonicalCombatActorBindingComponent.h"
#include "Interaction/OGInteractionParticipantComponent.h"
#include "EngineUtils.h"
#include "Runtime/OGGameCoreSubsystem.h"
#include "Runtime/OGGate12RulerSessionSubsystem.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"

bool FOGEncounterRankRules::Validate(FString& Error) const
{
    Error.Reset(); TSet<FOGContentId> Seen;
    if (OrderedRanks.IsEmpty() || Bands.IsEmpty()) { Error=TEXT("Rank rules require original ordering and explicit channel bands."); return false; }
    for (const auto& Rank:OrderedRanks)
    { if (!Rank.IsValid() || Seen.Contains(Rank)) { Error=TEXT("Rank ordering contains invalid or duplicate source IDs."); return false; } Seen.Add(Rank); }
    for (const auto& B:Bands)
        if (B.Channel.IsNone() || B.MinimumRankGap>B.MaximumRankGap || B.MultiplierBps<0 || B.MultiplierBps>10000 ||
            B.MinimumSourceLevel<1 || B.MaximumSourceLevel<B.MinimumSourceLevel ||
            B.MinimumTargetLevel<1 || B.MaximumTargetLevel<B.MinimumTargetLevel)
        { Error=TEXT("Rank channel bands require bounded source intervals and explicit multipliers."); return false; }
    for (int32 I=0;I<Bands.Num();++I) for (int32 J=I+1;J<Bands.Num();++J)
    {
        const auto& A=Bands[I]; const auto& B=Bands[J];
        if (A.Channel==B.Channel && A.MultiplierBps!=B.MultiplierBps &&
            A.MinimumRankGap<=B.MaximumRankGap && B.MinimumRankGap<=A.MaximumRankGap &&
            A.MinimumSourceLevel<=B.MaximumSourceLevel && B.MinimumSourceLevel<=A.MaximumSourceLevel &&
            A.MinimumTargetLevel<=B.MaximumTargetLevel && B.MinimumTargetLevel<=A.MaximumTargetLevel)
        { Error=TEXT("Original rank intervals overlap with conflicting channel multipliers."); return false; }
    }
    return true;
}
bool FOGEncounterRankRules::Resolve(const FOGRankSuppressionQuery& Q,int32& Out,FString& Error) const
{
    Out=0; Error.Reset(); const int32 Source=OrderedRanks.IndexOfByKey(Q.SourceRankId),Target=OrderedRanks.IndexOfByKey(Q.TargetRankId);
    if (Source==INDEX_NONE || Target==INDEX_NONE || Q.SourceLevel<1 || Q.TargetLevel<1 || Q.ChannelId.IsNone())
    { Error=TEXT("Rank query does not match installed source ordering."); return false; }
    const int32 Gap=Target-Source; bool Found=false;
    for (const auto& B:Bands)
        if (B.Channel==Q.ChannelId && Gap>=B.MinimumRankGap && Gap<=B.MaximumRankGap &&
            Q.SourceLevel>=B.MinimumSourceLevel && Q.SourceLevel<=B.MaximumSourceLevel &&
            Q.TargetLevel>=B.MinimumTargetLevel && Q.TargetLevel<=B.MaximumTargetLevel)
        {
            if (Found && Out!=B.MultiplierBps) { Out=0; Error=TEXT("Overlapping rank bands give conflicting source multipliers."); return false; }
            Found=true; Out=B.MultiplierBps;
        }
    if (!Found) { Error=TEXT("No original rank band covers this actual query."); return false; }
    return true;
}
bool UOGEncounterRankDefinition::ValidateDefinition_Implementation(FString& Error) const
{
    return Super::ValidateDefinition_Implementation(Error) && System==TEXT("rank_suppression") &&
        !OriginalSourceReference.TrimStartAndEnd().IsEmpty() && Rules.Validate(Error);
}
bool UOGNativeEncounterDefinition::ValidateDefinition_Implementation(FString& Error) const
{
    if (!Super::ValidateDefinition_Implementation(Error)) return false;
    if (System!=TEXT("encounter") || OriginalSourceReference.TrimStartAndEnd().IsEmpty() || Roles.IsEmpty() ||
        CanonicalIdentityField.IsNone() || CanonicalVersionField.IsNone() ||
        static_cast<uint8>(Mode)>static_cast<uint8>(EOGCombatExecutionMode::World))
    { Error=TEXT("Encounter plans require original source, roles, canonical field contracts and a supported mode."); return false; }
    TSet<FName> Names; TSet<int32> Teams;
    for (const auto& R:Roles)
    {
        if (R.Role.IsNone() || Names.Contains(R.Role) || R.MinimumParticipants<0 ||
            R.MaximumParticipants < -1 || (R.MaximumParticipants>=0 && R.MaximumParticipants<R.MinimumParticipants))
        { Error=TEXT("Encounter roles require distinct names and valid source participant intervals."); return false; }
        Names.Add(R.Role); Teams.Add(R.TeamIndex);
    }
    if (Teams.Num()<2) { Error=TEXT("An encounter needs at least two authored teams."); return false; }
    if (!RankRules.Id.IsEmpty() && (RankRules.System!=TEXT("rank_suppression") || RankRules.MinimumVersion<1))
    { Error=TEXT("Encounter rank rules must reference their shared native subsystem."); return false; }
    return true;
}
void UOGNativeEncounterDefinition::CollectRequiredDefinitions(TArray<FOGDefinitionReference>& Out) const
{ Super::CollectRequiredDefinitions(Out); if (RankRules.Id.IsValid()) Out.Add(RankRules); }
bool UOGNativeEncounterDefinition::ValidateResolvedReferences(const FOGDefinitionResolver& Resolve,FString& Error) const
{
    if (RankRules.Id.IsEmpty()) return true;
    const auto* Rules=Cast<UOGEncounterRankDefinition>(Resolve(RankRules.System,RankRules.Id));
    if (!Rules) { Error=TEXT("Encounter rank rules have no installed native definition."); return false; }
    return Rules->ValidateDefinition(Error);
}
bool UOGNativeEncounterDefinition::Assemble(const TArray<FOGResolvedEncounterParticipant>& Inputs,const FOGEntityId& Id,
    FOGTurnBattleState& Out,FString& Error) const
{
    Out=FOGTurnBattleState(); if (!ValidateDefinition(Error)) return false;
    if (!Id.IsValid()) { Error=TEXT("An encounter requires its own valid session identity."); return false; }
    FOGTurnBattleState Candidate; Candidate.BattleId=Id; Candidate.ExecutionMode=Mode; Candidate.EncounterDefinitionId=DefinitionId;
    TSet<FOGEntityId> Entities; TMap<FName,int32> Counts; TMap<int32,TMap<int32,FOGEntityId>> Formations;
    TMap<int32,int32> Rosters;
    for (const auto& Input:Inputs)
    {
        const auto* Plan=Roles.FindByPredicate([&](const auto& R){return R.Role==Input.Role;});
        const auto& U=Input.Unit;
        if (!Plan || !U.UnitEntityId.IsValid() || Entities.Contains(U.UnitEntityId) || !U.IdentityId.IsValid() ||
            !U.SkillSet.CurrentForm.IsValid() || U.Stats.MaxHp.GetSign()<=0 || U.CurrentHp.GetSign()<0 ||
            FOGLargeNumber::Compare(U.CurrentHp,U.Stats.MaxHp)>0 || U.DefaultActionDelay<1 || Input.OpeningActionValue<0 ||
            U.Presence==EOGCombatPresence::Removed)
        { Error=TEXT("Encounter participants must be distinct actual resolved units bound to authored roles."); return false; }
        Entities.Add(U.UnitEntityId); ++Counts.FindOrAdd(Input.Role); ++Rosters.FindOrAdd(Plan->TeamIndex);
        auto Unit=U; Unit.TeamIndex=Plan->TeamIndex; Unit.NextActionValue=Input.OpeningActionValue;
        if (Mode==EOGCombatExecutionMode::Turn)
        {
            if (Input.FormationIndex<0 || Input.FormationIndex>=FOGTurnTeamState::MaxRosterSize ||
                Formations.FindOrAdd(Plan->TeamIndex).Contains(Input.FormationIndex))
            { Error=TEXT("Turn formation slots must be distinct within the existing 18-unit team contract."); return false; }
            Formations.FindChecked(Plan->TeamIndex).Add(Input.FormationIndex,U.UnitEntityId);
            Unit.Presence=Unit.IsAlive()?(Input.FormationIndex<FOGTurnTeamState::MaxActiveSize?
                EOGCombatPresence::Active:EOGCombatPresence::Reserve):EOGCombatPresence::Defeated;
            Unit.OccupiedLaneIndex=Unit.CanAct()?Input.FormationIndex%FOGTurnTeamState::MaxActiveSize:INDEX_NONE;
        }
        else
        {
            Unit.Presence=Unit.IsAlive()?(Input.bWorldActive?EOGCombatPresence::Active:EOGCombatPresence::Reserve):EOGCombatPresence::Defeated;
            Unit.OccupiedLaneIndex=INDEX_NONE;
        }
        Candidate.Units.Add(Unit);
    }
    for (const auto& Plan:Roles)
    {
        const int32 Count=Counts.FindRef(Plan.Role);
        if (Count<Plan.MinimumParticipants || (Plan.MaximumParticipants>=0 && Count>Plan.MaximumParticipants))
        { Error=TEXT("Actual encounter role counts differ from the source plan."); return false; }
    }
    TArray<int32> Teams; Rosters.GetKeys(Teams); Teams.Sort();
    if (Teams.Num()<2) { Error=TEXT("The actual encounter has fewer than two teams."); return false; }
    for (int32 TeamId:Teams)
    {
        auto& Team=Candidate.Teams.AddDefaulted_GetRef(); Team.TeamIndex=TeamId;
        if (Mode==EOGCombatExecutionMode::Turn)
        {
            if (Rosters.FindChecked(TeamId)>FOGTurnTeamState::MaxRosterSize) { Error=TEXT("Turn roster exceeds its canonical team capacity."); return false; }
            const auto& Formation=Formations.FindChecked(TeamId);
            for (int32 Lane=0;Lane<FOGTurnTeamState::MaxLaneCount;++Lane)
            {
                FOGTurnSuccessionLane Slot; Slot.LaneIndex=Lane; bool Gap=false;
                for (int32 Depth=0;Depth<FOGTurnTeamState::MaxLaneSize;++Depth)
                {
                    const auto* Unit=Formation.Find(Lane+Depth*FOGTurnTeamState::MaxActiveSize);
                    if (!Unit) { Gap=true; continue; }
                    if (Gap) { Error=TEXT("A succession lane has a missing earlier source slot."); return false; }
                    Slot.OrderedUnitIds.Add(*Unit);
                }
                if (!Slot.OrderedUnitIds.IsEmpty()) Team.Lanes.Add(MoveTemp(Slot));
            }
        }
    }
    Out=MoveTemp(Candidate); return true;
}
bool UOGNativeEncounterReceiver::PrepareDefinition_Implementation(UDataAsset* Asset,const FOGDefinitionBindingContext&,FString& Error)
{
    const auto* D=Cast<UOGNativeEncounterDefinition>(Asset);
    if (!D) { Error=TEXT("Encounter receiver requires its shared native plan."); return false; }
    return D->ValidateDefinition(Error);
}
bool UOGNativeEncounterReceiver::StartEncounter(const TArray<FOGResolvedEncounterParticipant>& Participants,FString& Error)
{
    const auto* D=Cast<UOGNativeEncounterDefinition>(BoundDefinition);
    auto* Game=GetWorld()?GetWorld()->GetGameInstance():nullptr;
    auto* Core=Game?Game->GetSubsystem<UOGGameCoreSubsystem>():nullptr;
    const auto* Session=Game?Game->GetSubsystem<UOGGate12RulerSessionSubsystem>():nullptr;
    if (!IsActive() || !D || !Core || !Session || !Session->IsBoundToStoredRuler())
    { Error=TEXT("Encounter trigger requires its actual World and stored Ruler session."); Status=Error; return false; }
    const bool Started=Core->StartOwnedNativeEncounter(*D,Participants,Session->BoundRulerId,{},Error);
    if (!Started) { Status=Error; return false; }
    const auto State=Core->GetOwnedLuminousTurnBattle()->GetState(); LastStartedBattle=State.BattleId;
    TSet<FOGEntityId> Connected;
    for (TActorIterator<AActor> It(GetWorld());It;++It)
    {
        auto* Participant=It->FindComponentByClass<UOGInteractionParticipantComponent>();
        const auto Id=Participant?Participant->GetBoundCanonicalEntityId():FOGEntityId();
        const auto* Unit=State.Units.FindByPredicate([&](const auto& U){return U.UnitEntityId==Id;});
        if (!Unit || !Unit->CanAct()) continue;
        auto* Body=It->FindComponentByClass<UOGCanonicalCombatActorBindingComponent>();
        if (!Body)
        {
            Body=NewObject<UOGCanonicalCombatActorBindingComponent>(*It);
            It->AddInstanceComponent(Body); Body->RegisterComponent();
        }
        FString BindingError;
        if (Body->RefreshCanonicalEncounter(BindingError)) Connected.Add(Id);
    }
    int32 Missing=0;
    for (const auto& Unit:State.Units) if (Unit.CanAct() && !Connected.Contains(Unit.UnitEntityId)) ++Missing;
    Status=Missing
        ? FString::Printf(TEXT("Encounter saved; %d active participant bodies are not connected yet."),Missing)
        : TEXT("Encounter saved; actual participant bodies connected.");
    return true;
}
