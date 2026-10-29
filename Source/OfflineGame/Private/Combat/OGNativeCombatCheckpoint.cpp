#include "Combat/OGTurnBattle.h"
#include "Serialization/MemoryReader.h"
#include "Serialization/MemoryWriter.h"
#include "Serialization/ObjectAndNameAsStringProxyArchive.h"
#include "Misc/Crc.h"
#include "UObject/Class.h"

namespace
{
constexpr int32 CheckpointVersion=2;
enum ECallbackRequirement : uint32 { IdentityCallback=1,ConditionCallback=2,RankCallback=4,BloodCallback=8,
    RevivalCallback=16,FieldCallback=32,ContestCallback=64 };
constexpr int32 MaximumCheckpointBytes=16*1024*1024;
constexpr int32 MaximumRows=100000;
class FCheckpointFields
{
public:
    explicit FCheckpointFields(FArchive& In):Ar(In) {}
    FArchive& Ar;
    template<class T> auto Field(T& V)->decltype(T::StaticStruct(),void())
    { T::StaticStruct()->SerializeItem(Ar,&V,nullptr); }
    void Field(FString& V) { Ar<<V; }
    void Field(FName& V) { Ar<<V; }
    void Field(int32& V) { Ar<<V; }
    void Field(uint32& V) { Ar<<V; }
    void Field(uint8& V) { Ar<<V; }
    void Field(int64& V) { Ar<<V; }
    void Field(uint64& V) { Ar<<V; }
    void Field(bool& V) { Ar<<V; }
    int32 Count(int32 Saving)
    {
        int32 N=Saving; Ar<<N;
        if (N<0 || N>MaximumRows) { Ar.SetError(); return 0; }
        return N;
    }
    template<class T,class F> void ArrayWith(TArray<T>& Values,F SerializeValue)
    {
        const int32 N=Count(Values.Num());
        if (Ar.IsLoading()) Values.SetNum(N);
        for (auto& Value:Values) { if (Ar.IsError()) break; SerializeValue(Value); }
    }
    template<class T> void Array(TArray<T>& Values)
    {
        const int32 N=Count(Values.Num());
        if (Ar.IsLoading()) Values.SetNum(N);
        for (auto& Value:Values) { if (Ar.IsError()) break; Field(Value); }
    }
    template<class T> void Set(TSet<T>& Values)
    {
        const int32 N=Count(Values.Num());
        if (Ar.IsLoading())
        {
            Values.Reset();
            for (int32 I=0;I<N && !Ar.IsError();++I)
            { T V{}; Field(V); if (Values.Contains(V)) Ar.SetError(); else Values.Add(MoveTemp(V)); }
        }
        else for (auto& V:Values) Field(V);
    }
    template<class K,class V,class F> void MapWith(TMap<K,V>& Values,F SerializeValue)
    {
        const int32 N=Count(Values.Num());
        if (Ar.IsLoading())
        {
            Values.Reset();
            for (int32 I=0;I<N && !Ar.IsError();++I)
            {
                K Key{}; V Value{}; Field(Key); SerializeValue(Value);
                if (Values.Contains(Key)) Ar.SetError(); else Values.Add(MoveTemp(Key),MoveTemp(Value));
            }
        }
        else for (auto& Pair:Values) { Field(Pair.Key); SerializeValue(Pair.Value); }
    }
    template<class K,class V> void Map(TMap<K,V>& Values)
    { MapWith(Values,[&](V& Value){Field(Value);}); }
};
}

// Friend access is limited to this exact checkpoint codec. No gameplay callbacks
// or executable permissions are read from a saved World record.
class FOGNativeCombatCheckpointCodec
{
public:
    static void TrimDiagnostics(FOGTurnBattle& B)
    {
        if (B.Log.Events.Num()>2048) B.Log.Events.RemoveAt(0,B.Log.Events.Num()-2048,EAllowShrinking::No);
    }
    static uint32 Requirements(const FOGTurnBattle& B)
    {
        uint32 Mask=0;
        if (B.IdentityContext.Override) Mask|=IdentityCallback;
        if (B.NativeFallbackConditions || B.TriggerRuntime.ConditionEvaluator) Mask|=ConditionCallback;
        if (B.RankSuppressionResolver) Mask|=RankCallback;
        if (B.LuminousBloodEligibility) Mask|=BloodCallback;
        if (B.LuminousRevivalEligibility) Mask|=RevivalCallback;
        if (B.LuminousFieldEligibility) Mask|=FieldCallback;
        if (B.LuminousRegisteredContestEvidence) Mask|=ContestCallback;
        return Mask;
    }
    static bool Supported(const FOGTurnBattle& B,FString& Error)
    {
        if (B.NativeReactionProof.IsSet() || B.WorldExecutionProof.IsSet() || B.bApprovedRevivalHealInProgress ||
            !B.PendingTriggeredActions.IsEmpty() || !B.OutstandingTriggeredActionSequences.IsEmpty() ||
            !B.PendingDefeatedUnitIds.IsEmpty() || !B.TriggerRuntime.QueuedActions.IsEmpty())
        { Error=TEXT("Encounter checkpoint requires a fully settled action boundary."); return false; }
        return true;
    }
    static void Serialize(FOGTurnBattle& B,FArchive& Archive,int32 Version,uint32& Required)
    {
        FCheckpointFields F(Archive);
        F.Field(B.State); F.Field(B.bRequireNativeExecutors); F.Field(B.NativeAbilityRevision);
        F.Map(B.NativeAbilities); F.Map(B.NativeResourceSpecifications); F.Map(B.NativeEffectDefinitions);
        F.Map(B.WorldAbilities); F.Map(B.NativeOwnerActions); F.Map(B.NativeAbilityReadyActions);
        F.Map(B.NativeOwnerActionEpochs); F.Map(B.WorldAbilityReadyAt);
        F.MapWith(B.NativeOwnerActivationCounts,[&](auto& V){F.Field(V.Epoch); F.Field(V.Count);});
        F.MapWith(B.NativePassiveRules,[&](auto& V)
        { F.Field(V.OwnerUnitId); F.Field(V.PassiveSkillId); F.Field(V.Rule); });
        F.Set(B.NativePassiveActionOwners); F.Array(B.LastNativeReactions);
        F.MapWith(B.WorldCasts,[&](auto& V)
        {
            F.Field(V.Root); F.Field(V.Specification); F.Field(V.StartedAt); F.Field(V.Seed);
            F.Set(V.CompletedSteps); F.Map(V.Activations); F.Field(V.bCostsPaid); F.Field(V.bHadContact);
            F.Set(V.BeforeEffectTargets); F.Array(V.ContactTargets);
        });
        F.Map(B.LastEffectiveHpDamage);
        F.Array(B.Log.Events); F.Field(B.Log.NextSequence);
        F.Array(B.TriggerRuntime.Bindings); F.Field(B.TriggerRuntime.NextQueueSequence);
        if (Version<2) return; // Original generic images retain their exact prefix.
        F.Field(Required); F.Field(B.AcceptedBleedFamilyId);
        F.MapWith(B.LuminousFields,[&](auto& V)
        {
            F.Field(V.OwnerFantasmId); F.Field(V.ActivatedOwnerAction); F.Field(V.ExpiresBeforeOwnerAction);
            F.Field(V.BaseNocturneSuccessfulTargetCount); F.Set(V.NocturneConsumedTargets); F.Set(V.RekindledAllies);
            F.Field(V.bLastRoseUsed); F.Field(V.AlterBloodReversalSuccessCount); F.Set(V.AlterThornConsumedTargets);
            F.Field(V.bAlterMirrorDebtUsed); F.Set(V.AlterNegationRefundedCategories);
        });
        F.MapWith(B.LuminousBaseStates,[&](auto& V)
        {
            F.Field(V.ScarletReserve); F.Field(V.UltimateChargePercent); F.Field(V.bRevivalUsed);
            F.Field(V.RevivalAfterglowExpiresOwnerAction); F.Field(V.MercifulEdgeExpiresOwnerAction);
            F.Field(V.LastNightExecutionChargeOwnerAction); F.Field(V.bDayAttunement); F.Field(V.bAttunementActive);
            F.Field(V.AttunementExpiryAction); F.Field(V.NextA2ReadyOwnerAction); F.Field(V.NextA3ReadyOwnerAction);
            F.Field(V.DiscountNextA2Reserve); F.Field(V.bNextScarletCadenceDelayReduction); F.Field(V.LastBleedReserveOwnerAction);
            F.Set(V.PendingFourMarkTargetBonus); F.Set(V.EclipseClaimTargets); F.Map(V.EclipseClaimExpiryOwnerAction);
            F.Field(V.bAllyRevivalSpent); F.Field(V.PreparedCovenantTarget); F.Field(V.CovenantExpiryOwnerAction);
            F.Field(V.PreparedDayUltimateRevivalTarget); F.Field(V.DayUltimateRevivalExpiryOwnerAction); F.Field(V.OwnerActions);
            F.Map(V.HemalMarks); F.Map(V.HemalMarkExpiryOwnerAction); F.Map(V.HemorrhageStacks); F.Map(V.HemorrhageEndOwnerAction);
            F.Map(V.BlessingExpiry); F.Map(V.BlessingCategory); F.Field(V.DawnOathExpiry); F.Map(V.DawnOathByBeneficiary);
            F.Field(V.NextFantasmReadyOwnerAction); F.Array(V.ObservedEnemyContentIds);
        });
        F.MapWith(B.LuminousAlterStates,[&](auto& V)
        {
            F.Field(V.RedTithe); F.Field(V.UltimateChargePercent); F.Field(V.OwnerActions); F.Field(V.MartyrMomentum);
            F.Field(V.MomentumExpiresOwnerAction); F.Field(V.MartyrStanceExpires); F.Field(V.bMartyrNextFinisherPending);
            F.Field(V.DeathPriceExpiresOwnerAction); F.Field(V.EucharistNextAvailableAction); F.Field(V.CommunionNextAvailableAction);
            F.Field(V.bSelfRevived); F.Field(V.LastPursuitOwnerAction); F.Field(V.bPursuitDelayPending);
            F.Array(V.ObservedEnemyContentIds); F.Map(V.RuptureSeals); F.Map(V.SealExpiryOwnerAction);
            F.Map(V.SeveringWounds); F.Map(V.WoundExpiryOwnerAction);
        });
        F.ArrayWith(B.LuminousDawnBarriers,[&](auto& V)
        { F.Field(V.GrantingManifestation); F.Field(V.Beneficiary); F.Field(V.Remaining); F.Field(V.ExpiresAfterOwnerAction); });
    }
    static bool Validate(const FOGTurnBattle& B,FString& Error)
    {
        const auto& S=B.State;
        auto Bad=[&](){Error=TEXT("Encounter checkpoint contains inconsistent native state."); return false;};
        if (!S.BattleId.IsValid() || S.Teams.Num()<2 || S.Units.IsEmpty() ||
            (S.Status!=EOGTurnBattleStatus::Running && S.Status!=EOGTurnBattleStatus::Completed) ||
            (S.ExecutionMode!=EOGCombatExecutionMode::Turn && S.ExecutionMode!=EOGCombatExecutionMode::World) ||
            S.WorldElapsedMilliseconds<0 || S.CurrentActionValue<0 ||
            B.Log.NextSequence<0 || B.TriggerRuntime.NextQueueSequence<0) return Bad();
        TSet<int32> Teams; TSet<FOGEntityId> Units; TSet<FString> Occupied;
        for (const auto& T:S.Teams) { if (Teams.Contains(T.TeamIndex)) return Bad(); Teams.Add(T.TeamIndex); }
        for (const auto& U:S.Units)
        {
            if (!U.UnitEntityId.IsValid() || !U.IdentityId.IsValid() || !U.SkillSet.CurrentForm.IsValid() ||
                Units.Contains(U.UnitEntityId) || !Teams.Contains(U.TeamIndex) || U.Stats.MaxHp.GetSign()<=0 ||
                U.CurrentHp.GetSign()<0 || FOGLargeNumber::Compare(U.CurrentHp,U.Stats.MaxHp)>0 ||
                U.NextActionValue<0 || U.DefaultActionDelay<=0 ||
                static_cast<uint8>(U.Presence)>static_cast<uint8>(EOGCombatPresence::Removed)) return Bad();
            Units.Add(U.UnitEntityId);
            if (S.ExecutionMode==EOGCombatExecutionMode::Turn && U.CanAct())
            {
                const FString Slot=LexToString(U.TeamIndex)+TEXT("|")+LexToString(U.OccupiedLaneIndex);
                if (U.OccupiedLaneIndex<0 || U.OccupiedLaneIndex>=FOGTurnTeamState::MaxLaneCount ||
                    Occupied.Contains(Slot)) return Bad();
                Occupied.Add(Slot);
            }
            TSet<FOGContentId> Resources;
            for (const auto& R:U.BattleResources)
            {
                if (!R.ResourceId.IsValid() || Resources.Contains(R.ResourceId) || R.Value<0 || R.Maximum<0 || R.Value>R.Maximum) return Bad();
                Resources.Add(R.ResourceId);
            }
            for (const auto& E:U.NativeEffects)
                if (!E.InstanceId.IsValid() || !B.NativeEffectDefinitions.Contains(E.EffectId) || !E.SourceUnitId.IsValid()) return Bad();
        }
        TSet<FOGEntityId> Formed;
        for (const auto& Team:S.Teams)
        {
            if (Team.GetRosterSize()>FOGTurnTeamState::MaxRosterSize || Team.Lanes.Num()>FOGTurnTeamState::MaxLaneCount) return Bad();
            TSet<int32> Lanes;
            for (const auto& Lane:Team.Lanes)
            {
                if (Lane.LaneIndex<0 || Lane.LaneIndex>=FOGTurnTeamState::MaxLaneCount || Lanes.Contains(Lane.LaneIndex) ||
                    Lane.OrderedUnitIds.IsEmpty() || Lane.OrderedUnitIds.Num()>FOGTurnTeamState::MaxLaneSize) return Bad();
                Lanes.Add(Lane.LaneIndex);
                for (const auto& Id:Lane.OrderedUnitIds)
                {
                    const auto* Unit=B.FindUnit(Id);
                    if (!Unit || Unit->TeamIndex!=Team.TeamIndex || Formed.Contains(Id)) return Bad();
                    Formed.Add(Id);
                }
            }
        }
        if (S.ExecutionMode==EOGCombatExecutionMode::Turn && Formed.Num()!=S.Units.Num()) return Bad();
        for (const auto& Pair:B.NativeAbilities) if (!Pair.Value.Validate(Error)) return false;
        for (const auto& Pair:B.NativeEffectDefinitions) if (!Pair.Value.Validate(Error)) return false;
        for (const auto& Pair:B.WorldAbilities) if (!Pair.Value.Validate(Error)) return false;
        TSet<FOGEntityId> Casters;
        for (const auto& Pair:B.WorldCasts)
        {
            const auto& C=Pair.Value; const auto* Owner=B.FindUnit(C.Root.SourceUnitId);
            if (S.ExecutionMode!=EOGCombatExecutionMode::World || !Pair.Key.IsValid() || Pair.Key!=C.Root.ActionId ||
                !Owner || Casters.Contains(Owner->UnitEntityId) || !C.Specification.Matches(*Owner) ||
                C.Root.SkillId!=C.Specification.SkillId || C.StartedAt<0 || C.StartedAt>S.WorldElapsedMilliseconds ||
                C.Specification.DurationMilliseconds>MAX_int64-C.StartedAt || !C.Specification.Validate(Error)) return Bad();
            Casters.Add(Owner->UnitEntityId);
            for (int32 Step:C.CompletedSteps) if (!C.Specification.Steps.IsValidIndex(Step)) return Bad();
        }
        int64 Previous=-1;
        for (const auto& E:B.Log.Events)
        { if (E.Sequence<=Previous || E.Sequence>=B.Log.NextSequence) return Bad(); Previous=E.Sequence; }
        return ValidateLocalIdentityExclusivity(S.Units,B.IdentityContext,Error);
    }
};

bool FOGTurnBattle::ExportNativeCheckpoint(TArray<uint8>& Out,FString& Error) const
{
    Out.Reset(); Error.Reset();
    if (!FOGNativeCombatCheckpointCodec::Supported(*this,Error) ||
        !FOGNativeCombatCheckpointCodec::Validate(*this,Error)) return false;
    FOGTurnBattle Snapshot=*this;
    // Canonical history remains in SQLite. Keep bounded diagnostic history in the runtime image.
    FOGNativeCombatCheckpointCodec::TrimDiagnostics(Snapshot);
    TArray<uint8> Payload;
    FMemoryWriter Memory(Payload,true); FObjectAndNameAsStringProxyArchive Archive(Memory,false);
    uint32 Required=FOGNativeCombatCheckpointCodec::Requirements(Snapshot);
    FOGNativeCombatCheckpointCodec::Serialize(Snapshot,Archive,CheckpointVersion,Required);
    if (Archive.IsError() || Payload.IsEmpty() || Payload.Num()>MaximumCheckpointBytes)
    { Error=TEXT("Cannot serialize the native encounter checkpoint."); return false; }
    FMemoryWriter Envelope(Out,true);
    int32 Version=CheckpointVersion; uint32 Crc=FCrc::MemCrc32(Payload.GetData(),Payload.Num());
    Envelope<<Version; Envelope<<Crc; Envelope.Serialize(Payload.GetData(),Payload.Num());
    return !Envelope.IsError();
}
bool FOGTurnBattle::RestoreNativeCheckpoint(const TArray<uint8>& Bytes,FOGRankSuppressionResolver Rank,FString& Error)
{
    FOGCombatRecoveryBindings Bindings; Bindings.Rank=MoveTemp(Rank);
    return RestoreNativeCheckpointWithBindings(Bytes,MoveTemp(Bindings),Error);
}
bool FOGTurnBattle::RestoreNativeCheckpointWithBindings(const TArray<uint8>& Bytes,FOGCombatRecoveryBindings Bindings,FString& Error)
{
    Error.Reset();
    if (Bytes.Num()<9 || Bytes.Num()>MaximumCheckpointBytes+8)
    { Error=TEXT("Native encounter checkpoint size is invalid."); return false; }
    FMemoryReader Envelope(Bytes,true); int32 Version=0; uint32 Crc=0;
    Envelope<<Version; Envelope<<Crc;
    if ((Version!=1 && Version!=CheckpointVersion) || Crc!=FCrc::MemCrc32(Bytes.GetData()+8,Bytes.Num()-8))
    { Error=TEXT("Native encounter checkpoint version or integrity differs; saved bytes preserved."); return false; }
    TArray<uint8> Payload; Payload.Append(Bytes.GetData()+8,Bytes.Num()-8);
    FMemoryReader Memory(Payload,true); FObjectAndNameAsStringProxyArchive Archive(Memory,false);
    FOGTurnBattle Candidate; uint32 Required=0;
    FOGNativeCombatCheckpointCodec::Serialize(Candidate,Archive,Version,Required);
    if (Archive.IsError() || Memory.Tell()!=Memory.TotalSize() || (Required&~uint32(127))!=0)
    { Error=TEXT("Native encounter checkpoint could not be decoded atomically."); return false; }
    const bool SpecializedState=!Candidate.LuminousBaseStates.IsEmpty() || !Candidate.LuminousAlterStates.IsEmpty() ||
        !Candidate.LuminousFields.IsEmpty() || !Candidate.LuminousDawnBarriers.IsEmpty() ||
        Candidate.State.Units.ContainsByPredicate([](const auto& U){return U.IdentityId==FOGContentId(TEXT("character:luminous_valentine.identity"));});
    if ((SpecializedState && !Bindings.RebindSpecialized) ||
        ((Required&IdentityCallback) && !Bindings.Identity.Override) ||
        ((Required&ConditionCallback) && !Bindings.Conditions) ||
        ((Required&RankCallback) && !Bindings.Rank) ||
        ((Required&(BloodCallback|RevivalCallback|FieldCallback|ContestCallback)) && !Bindings.RebindSpecialized))
    { Error=TEXT("Installed native recovery provider has not rebuilt all required callback authority."); return false; }
    Candidate.IdentityContext=MoveTemp(Bindings.Identity);
    Candidate.NativeFallbackConditions=Bindings.Conditions;
    Candidate.TriggerRuntime.ConditionEvaluator=MoveTemp(Bindings.Conditions);
    Candidate.RankSuppressionResolver=MoveTemp(Bindings.Rank);
    const auto ExpectedBleed=Candidate.AcceptedBleedFamilyId;
    if (Bindings.RebindSpecialized && !Bindings.RebindSpecialized(Candidate,Error)) return false;
    if ((FOGNativeCombatCheckpointCodec::Requirements(Candidate)&Required)!=Required ||
        (ExpectedBleed.IsValid() && Candidate.AcceptedBleedFamilyId!=ExpectedBleed))
    { Error=TEXT("Restored callback authority differs from the captured native contract."); return false; }
    if (!FOGNativeCombatCheckpointCodec::Validate(Candidate,Error))
    { if (Error.IsEmpty()) Error=TEXT("Native encounter checkpoint validation failed."); return false; }
    // No Initialize(), opening triggers, costs, resource grants or rewards run on restore.
    *this=MoveTemp(Candidate); return true;
}
bool FOGTurnBattle::ReadWorldCastRecovery(const FOGEntityId& Cast,FOGResolvedCombatAction& Root,
    FOGWorldAbilitySpecification& Specification,TSet<int32>& Completed,int64& Started) const
{
    const auto* Found=WorldCasts.Find(Cast); if (!Found) return false;
    Root=Found->Root; Specification=Found->Specification; Completed=Found->CompletedSteps; Started=Found->StartedAt;
    return true;
}
