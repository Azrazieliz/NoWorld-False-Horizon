#if WITH_DEV_AUTOMATION_TESTS
#include "Combat/OGNativeCombatAbility.h"
#include "Combat/OGTurnBattle.h"
#include "Misc/AutomationTest.h"

namespace
{
FOGCombatUnitState NativeTestUnit(int32 Team,const TCHAR* Identity,int64 Hp=1000)
{
    FOGCombatUnitState U; U.UnitEntityId=FOGEntityId::NewId(); U.IdentityId=FOGContentId(Identity);
    U.SkillSet.CurrentForm=FOGContentId(TEXT("test:native.version")); U.TeamIndex=Team;
    U.Presence=EOGCombatPresence::Active; U.CurrentHp=FOGLargeNumber::FromInt64(Hp);
    U.Stats.MaxHp=FOGLargeNumber::FromInt64(1000); U.Stats.Attack=FOGLargeNumber::FromInt64(100);
    U.Stats.HitBps=10000; U.Stats.CritRateBps=0; U.Stats.Defense=FOGLargeNumber();
    U.NextActionValue=Team?10000:0; U.DefaultActionDelay=100;
    return U;
}
FOGTurnTeamState NativeTestTeam(int32 Index,const TArray<FOGEntityId>& Ids)
{
    FOGTurnTeamState T; T.TeamIndex=Index;
    for (int32 I=0;I<Ids.Num();++I) { FOGTurnSuccessionLane L; L.LaneIndex=I; L.OrderedUnitIds.Add(Ids[I]); T.Lanes.Add(L); }
    return T;
}
FOGNativeAbilityDefinition NativeStrike(const FOGCombatUnitState& Source,EOGNativeAbilityTargets Targets=EOGNativeAbilityTargets::SingleEnemy)
{
    FOGNativeAbilityDefinition D; D.IdentityId=Source.IdentityId; D.VersionId=Source.SkillSet.CurrentForm;
    D.bNoResourceCost=true;
    D.SkillId=Source.SkillSet.ActiveSkills[0]; D.OriginalSourceReference=TEXT("test-only exact authored fixture"); D.Targets=Targets;
    FOGNativeAbilityPacket P; P.DefenseReference=EOGNativeDefenseReference::Attack; P.Repeats=2;
    FOGNativeAbilityScale S; S.Stat=EOGNativeScaleStat::Attack; S.MultiplierBps=10000; P.Scaling.Add(S); D.Packets.Add(P);
    return D;
}
FOGTurnBattleState NativeInitial(const FOGCombatUnitState& Source,const TArray<FOGCombatUnitState>& Enemies)
{
    FOGTurnBattleState S; S.BattleId=FOGEntityId::NewId(); S.Units.Add(Source); S.Units.Append(Enemies);
    TArray<FOGEntityId> Ids; for (const auto& U:Enemies) Ids.Add(U.UnitEntityId);
    S.Teams={NativeTestTeam(0,{Source.UnitEntityId}),NativeTestTeam(1,Ids)}; return S;
}
FOGResolvedCombatAction NativeAction(const FOGCombatUnitState& Source,const FOGEntityId& Target)
{
    FOGResolvedCombatAction A; A.ActionId=FOGEntityId::NewId(); A.SourceUnitId=Source.UnitEntityId;
    A.SkillId=Source.SkillSet.ActiveSkills[0]; A.TargetUnitIds={Target}; A.ActionDelay=Source.DefaultActionDelay; return A;
}
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FOGNativeAbilityExecutionTest,
    "OfflineGame.Combat.NativeAbility.RealDamageAndMissingExecutor",
    EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::EngineFilter)
bool FOGNativeAbilityExecutionTest::RunTest(const FString& Parameters)
{
    auto Source=NativeTestUnit(0,TEXT("test:native.actor"));
    Source.SkillSet.ActiveSkills.Add(FOGContentId(TEXT("test:native.strike")));
    const auto Enemy=NativeTestUnit(1,TEXT("test:native.enemy"));
    FOGTurnBattle Battle; FString Error;
    if (!TestTrue(TEXT("Initializes actual combat state"),Battle.Initialize(NativeInitial(Source,{Enemy}),Error))) return false;
    Battle.RequireNativeActionExecutors(true);
    const auto Sequence=Battle.GetLog().GetNextSequence();
    const auto Action=NativeAction(Source,Enemy.UnitEntityId);
    TestFalse(TEXT("Unimplemented ability is never accepted as an empty action"),Battle.ApplyResolvedAction(Action,Error));
    TestEqual(TEXT("Missing executor consumes no log or turn"),Battle.GetLog().GetNextSequence(),Sequence);
    TestEqual(TEXT("Missing executor preserves timeline"),Battle.GetState().CurrentActionValue,int64(0));
    auto Definition=NativeStrike(Source);
    if (!TestTrue(TEXT("Registers exact complete typed ability"),Battle.RegisterNativeAbility(Definition,Error))) return false;
    const auto Revision=Battle.GetNativeAbilityRevision();
    TestTrue(TEXT("Identical admission is idempotent"),Battle.RegisterNativeAbility(Definition,Error));
    TestEqual(TEXT("Idempotent admission preserves revision"),Battle.GetNativeAbilityRevision(),Revision);
    auto Conflict=Definition; Conflict.Packets[0].Scaling[0].MultiplierBps=20000;
    TestFalse(TEXT("Mid-encounter content cannot overwrite an admitted ability"),Battle.RegisterNativeAbility(Conflict,Error));
    if (!TestTrue(TEXT("Accepted attack resolves actual HP"),Battle.ApplyResolvedAction(Action,Error))) return false;
    TestTrue(TEXT("Two independently resolved 100-ATK hits remove 200 HP"),
        FOGLargeNumber::Compare(Battle.GetState().Units[1].CurrentHp,FOGLargeNumber::FromInt64(800))==0);
    return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FOGNativeAbilityAreaRollbackTest,
    "OfflineGame.Combat.NativeAbility.AllTargetsAndAtomicRollback",
    EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::EngineFilter)
bool FOGNativeAbilityAreaRollbackTest::RunTest(const FString& Parameters)
{
    auto Source=NativeTestUnit(0,TEXT("test:native.actor")); Source.SkillSet.ActiveSkills.Add(FOGContentId(TEXT("test:native.area")));
    const auto First=NativeTestUnit(1,TEXT("test:native.enemy_a")),Second=NativeTestUnit(1,TEXT("test:native.enemy_b"));
    FOGTurnBattle Battle; FString Error;
    if (!Battle.Initialize(NativeInitial(Source,{First,Second}),Error)) { AddError(Error); return false; }
    Battle.RequireNativeActionExecutors(true);
    if (!Battle.RegisterNativeAbility(NativeStrike(Source,EOGNativeAbilityTargets::AllEnemies),Error)) return false;
    auto Action=NativeAction(Source,First.UnitEntityId); FOGResolvedCombatAction Expanded; bool Area=false;
    if (!TestTrue(TEXT("Field expands"),Battle.ExpandApprovedFieldTargets(Action,Expanded,Area,Error))) return false;
    TestTrue(TEXT("Manual subset cannot omit the second enemy"),Area && Expanded.TargetUnitIds.Num()==2 && Expanded.TargetUnitIds.Contains(Second.UnitEntityId));
    if (!TestTrue(TEXT("Field damages the full roster"),Battle.ApplyResolvedAction(Action,Error))) return false;
    for (int32 I=1;I<3;++I) TestTrue(TEXT("Every eligible enemy loses genuine HP"),
        FOGLargeNumber::Compare(Battle.GetState().Units[I].CurrentHp,FOGLargeNumber::FromInt64(800))==0);

    Source.RankProjection.RankId=FOGContentId(TEXT("test:rank.source"));
    auto Ranked=Second; Ranked.RankProjection.RankId=FOGContentId(TEXT("test:rank.target"));
    FOGTurnBattle Rollback; if (!Rollback.Initialize(NativeInitial(Source,{First,Ranked}),Error)) return false;
    Rollback.RequireNativeActionExecutors(true);
    if (!Rollback.RegisterNativeAbility(NativeStrike(Source,EOGNativeAbilityTargets::AllEnemies),Error)) return false;
    const auto BeforeSequence=Rollback.GetLog().GetNextSequence();
    TestFalse(TEXT("Missing real Rank resolver rejects the later packet target"),Rollback.ApplyResolvedAction(NativeAction(Source,First.UnitEntityId),Error));
    TestEqual(TEXT("Rejected multi-target action leaves no events"),Rollback.GetLog().GetNextSequence(),BeforeSequence);
    for (int32 I=1;I<3;++I) TestTrue(TEXT("Earlier target damage rolls back with the failed action"),
        FOGLargeNumber::Compare(Rollback.GetState().Units[I].CurrentHp,FOGLargeNumber::FromInt64(1000))==0);
    return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FOGNativeAbilityHealingTest,
    "OfflineGame.Combat.NativeAbility.HealingAndDefinitionBoundaries",
    EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::EngineFilter)
bool FOGNativeAbilityHealingTest::RunTest(const FString& Parameters)
{
    auto Source=NativeTestUnit(0,TEXT("test:native.healer"),100);
    Source.SkillSet.ActiveSkills.Add(FOGContentId(TEXT("test:native.restore")));
    const auto Enemy=NativeTestUnit(1,TEXT("test:native.enemy"));
    FOGTurnBattle Battle; FString Error;
    if (!Battle.Initialize(NativeInitial(Source,{Enemy}),Error)) return false;
    Battle.RequireNativeActionExecutors(true);
    auto D=NativeStrike(Source,EOGNativeAbilityTargets::Self);
    D.Packets[0].Effect=EOGNativeAbilityEffect::Healing; D.Packets[0].DefenseReference=EOGNativeDefenseReference::Unspecified;
    D.Packets[0].Repeats=1; D.Packets[0].Scaling[0].MultiplierBps=5000;
    if (!TestTrue(TEXT("Self healing admits its actual packet"),Battle.RegisterNativeAbility(D,Error))) return false;
    auto Action=NativeAction(Source,Enemy.UnitEntityId); Action.ResolutionJson=TEXT("{\"amount\":999999}");
    TestFalse(TEXT("Command JSON cannot replace authored numbers"),Battle.ApplyResolvedAction(Action,Error));
    Action.ResolutionJson=TEXT("{}");
    if (!TestTrue(TEXT("Self target expansion ignores stale opponent focus"),Battle.ApplyResolvedAction(Action,Error))) return false;
    TestTrue(TEXT("Healing changes genuine caster HP"),
        FOGLargeNumber::Compare(Battle.GetState().Units[0].CurrentHp,FOGLargeNumber::FromInt64(150))==0);
    auto Paid=D; Paid.bNoResourceCost=false;
    TestFalse(TEXT("Missing resource executor cannot silently make a skill free"),Paid.Validate(Error));
    auto Invalid=NativeStrike(Source); Invalid.Packets[0].DefenseReference=EOGNativeDefenseReference::Unspecified;
    TestFalse(TEXT("Physical damage never gets an invented default DEF reference"),Invalid.Validate(Error));
    Invalid=D; Invalid.IdentityId=FOGContentId(TEXT("character:luminous_valentine.identity"));
    TestFalse(TEXT("Ordinary definitions cannot masquerade as complete Luminous kits"),Invalid.Validate(Error));
    auto* Transient=NewObject<UOGNativeCombatAbility>(); Transient->Definition=D;
    TestFalse(TEXT("Transient command objects cannot enter production admission"),Transient->ValidateInstalled(Error));
    return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FOGNativeAbilityCooldownTest,
    "OfflineGame.Combat.NativeAbility.PerSourceCooldown",
    EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::EngineFilter)
bool FOGNativeAbilityCooldownTest::RunTest(const FString& Parameters)
{
    auto Source=NativeTestUnit(0,TEXT("test:native.actor"));
    Source.SkillSet.ActiveSkills={FOGContentId(TEXT("test:native.primary")),FOGContentId(TEXT("test:native.other"))};
    const auto Enemy=NativeTestUnit(1,TEXT("test:native.enemy"));
    FOGTurnBattle Battle; FString Error;
    if (!Battle.Initialize(NativeInitial(Source,{Enemy}),Error)) return false;
    Battle.RequireNativeActionExecutors(true);
    auto First=NativeStrike(Source); First.CooldownOwnerActions=1;
    auto Other=NativeStrike(Source); Other.SkillId=Source.SkillSet.ActiveSkills[1];
    if (!Battle.RegisterNativeAbility(First,Error) || !Battle.RegisterNativeAbility(Other,Error)) return false;
    auto Action=NativeAction(Source,Enemy.UnitEntityId);
    if (!TestTrue(TEXT("First ability executes"),Battle.ApplyResolvedAction(Action,Error))) return false;
    const int64 Sequence=Battle.GetLog().GetNextSequence();
    TestFalse(TEXT("One intervening own action is required"),Battle.ApplyResolvedAction(Action,Error));
    TestEqual(TEXT("Cooldown rejection consumes no turn"),Battle.GetLog().GetNextSequence(),Sequence);
    Action.SkillId=Other.SkillId;
    if (!TestTrue(TEXT("A distinct owned ability advances this source's action count"),Battle.ApplyResolvedAction(Action,Error))) return false;
    Action.SkillId=First.SkillId;
    TestTrue(TEXT("Primary is ready after the exact intervening own action"),Battle.ApplyResolvedAction(Action,Error));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FOGSharedNativeAbilityTest,
    "OfflineGame.Combat.NativeAbilities.SharedSkillConnectsDifferentSources",
    EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FOGSharedNativeAbilityTest::RunTest(const FString&)
{
    auto Source=NativeTestUnit(0,TEXT("test:shared.source"));
    auto Enemy=NativeTestUnit(1,TEXT("test:shared.other"));
    const FOGContentId SharedSkill(TEXT("test:shared.skill"));
    Source.SkillSet.ActiveSkills={SharedSkill}; Enemy.SkillSet.ActiveSkills={SharedSkill};
    Enemy.SkillSet.CurrentForm=FOGContentId(TEXT("test:shared.other_version"));
    FOGTurnBattle Battle; FString Error;
    if (!Battle.Initialize(NativeInitial(Source,{Enemy}),Error)) return false;
    Battle.RequireNativeActionExecutors(true);
    auto Definition=NativeStrike(Source); Definition.IdentityId=FOGContentId(); Definition.VersionId=FOGContentId();
    if (!TestTrue(TEXT("One global skill admission"),Battle.RegisterNativeAbility(Definition,Error))) return false;
    TestTrue(TEXT("Original source owns registered executor"),Battle.HasNativeActionExecutor(Source,SharedSkill,Error));
    TestTrue(TEXT("Different Identity/Version connects without a duplicate definition"),Battle.HasNativeActionExecutor(Enemy,SharedSkill,Error));
    auto Override=Definition; Override.IdentityId=Source.IdentityId;
    TestFalse(TEXT("Later narrower scope cannot replace admitted shared mechanics"),Battle.RegisterNativeAbility(Override,Error));
    TestTrue(TEXT("Shared ability executes real packets"),Battle.ApplyResolvedAction(NativeAction(Source,Enemy.UnitEntityId),Error));
    const auto* Actual=Battle.GetState().Units.FindByPredicate([&](const auto& U){return U.UnitEntityId==Enemy.UnitEntityId;});
    TestTrue(TEXT("Shared executor changes actual battle HP"),Actual && FOGLargeNumber::Compare(Actual->CurrentHp,Enemy.CurrentHp)<0);
    return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FOGNativeResourceCostTest,
    "OfflineGame.Combat.NativeAbilities.ResourceCostsAreAtomicAndPerUnit",
    EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FOGNativeResourceCostTest::RunTest(const FString&)
{
    auto Source=NativeTestUnit(0,TEXT("test:resource.source"));
    auto Enemy=NativeTestUnit(1,TEXT("test:resource.enemy"),10000);
    Source.SkillSet.ActiveSkills={FOGContentId(TEXT("test:resource.skill"))};
    FOGTurnBattle Battle; FString Error;
    if (!Battle.Initialize(NativeInitial(Source,{Enemy}),Error)) return false;
    Battle.RequireNativeActionExecutors(true);
    FOGCombatResourceSpecification Resource;
    Resource.ResourceId=FOGContentId(TEXT("test:resource.meter")); Resource.DisplayName=FText::FromString(TEXT("Test meter"));
    Resource.Maximum=10; Resource.OpeningValue=3;
    if (!Battle.ConfigureNativeResource(Source.UnitEntityId,Resource,Error) ||
        !Battle.ConfigureNativeResource(Enemy.UnitEntityId,Resource,Error)) return false;
    auto Definition=NativeStrike(Source); Definition.bNoResourceCost=false;
    FOGNativeAbilityResourceAmount Cost; Cost.ResourceId=Resource.ResourceId; Cost.Amount=2;
    FOGNativeAbilityResourceAmount Gain=Cost; Gain.Amount=1;
    Definition.ResourceCosts={Cost}; Definition.ResourceChanges={Gain};
    if (!Battle.RegisterNativeAbility(Definition,Error)) return false;
    auto Action=NativeAction(Source,Enemy.UnitEntityId);
    TestTrue(TEXT("Paid skill executes"),Battle.ApplyResolvedAction(Action,Error));
    const auto Read=[&](const FOGEntityId& Id)
    { return Battle.GetState().Units.FindByPredicate([&](const auto& U){return U.UnitEntityId==Id;})->BattleResources[0].Value; };
    TestEqual(TEXT("Exact cost then gain once per skill, not once per hit"),Read(Source.UnitEntityId),int64(2));
    TestEqual(TEXT("Other unit's same named meter is independent"),Read(Enemy.UnitEntityId),int64(3));
    TestTrue(TEXT("Second paid skill executes"),Battle.ApplyResolvedAction(Action,Error));
    const auto Sequence=Battle.GetLog().GetNextSequence();
    const auto Before=Battle.GetState().Units[1].CurrentHp;
    TestFalse(TEXT("Unaffordable skill is rejected"),Battle.ApplyResolvedAction(Action,Error));
    TestEqual(TEXT("Rejected cost consumes no timeline/log step"),Battle.GetLog().GetNextSequence(),Sequence);
    TestTrue(TEXT("Rejected cost causes no damage"),Battle.GetState().Units[1].CurrentHp==Before);
    TestEqual(TEXT("Rejected cost leaves the meter unchanged"),Read(Source.UnitEntityId),int64(1));
    TestTrue(TEXT("Identical re-admission is idempotent"),Battle.ConfigureNativeResource(Source.UnitEntityId,Resource,Error));
    TestEqual(TEXT("Re-admission cannot refill a spent meter"),Read(Source.UnitEntityId),int64(1));
    return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FOGNativeResourceSaturationTest,
    "OfflineGame.Combat.NativeAbilities.ResourceOnlySkillSaturatesWithoutOverflow",
    EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FOGNativeResourceSaturationTest::RunTest(const FString&)
{
    auto Source=NativeTestUnit(0,TEXT("test:capacity.source"));
    const auto Enemy=NativeTestUnit(1,TEXT("test:capacity.enemy"));
    Source.SkillSet.ActiveSkills={FOGContentId(TEXT("test:capacity.skill"))};
    FOGTurnBattle Battle; FString Error;
    if (!Battle.Initialize(NativeInitial(Source,{Enemy}),Error)) return false;
    Battle.RequireNativeActionExecutors(true);
    FOGCombatResourceSpecification Resource;
    Resource.ResourceId=FOGContentId(TEXT("test:capacity.meter")); Resource.DisplayName=FText::FromString(TEXT("Capacity"));
    Resource.Maximum=MAX_int64; Resource.OpeningValue=MAX_int64-1;
    if (!Battle.ConfigureNativeResource(Source.UnitEntityId,Resource,Error)) return false;
    auto Definition=NativeStrike(Source); Definition.Packets.Reset(); Definition.Targets=EOGNativeAbilityTargets::Self;
    FOGNativeAbilityResourceAmount Gain; Gain.ResourceId=Resource.ResourceId; Gain.Amount=MAX_int64;
    Definition.ResourceChanges={Gain};
    if (!Battle.RegisterNativeAbility(Definition,Error)) return false;
    TestTrue(TEXT("Resource-only skill resolves through the real executor"),Battle.ApplyResolvedAction(NativeAction(Source,Source.UnitEntityId),Error));
    TestEqual(TEXT("Near-int64 resource gain saturates safely"),Battle.GetState().Units[0].BattleResources[0].Value,int64(MAX_int64));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FOGNativeTurnCheckpointTest,
    "OfflineGame.Combat.Recovery.ExactTurnTimelineAndNativeCooldown",
    EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FOGNativeTurnCheckpointTest::RunTest(const FString&)
{
    auto Source=NativeTestUnit(0,TEXT("test:recovery.source"));
    Source.SkillSet.ActiveSkills={FOGContentId(TEXT("test:recovery.strike"))};
    auto Enemy=NativeTestUnit(1,TEXT("test:recovery.enemy"));
    const int64 Exact=9007199254740993LL;
    Source.NextActionValue=Exact; Enemy.NextActionValue=Exact+10000;
    auto Initial=NativeInitial(Source,{Enemy}); Initial.CurrentActionValue=Exact;
    FOGTurnBattle Battle; FString Error;
    if (!Battle.Initialize(Initial,Error)) return false;
    Battle.RequireNativeActionExecutors(true);
    auto Ability=NativeStrike(Source); Ability.CooldownOwnerActions=3;
    if (!Battle.RegisterNativeAbility(Ability,Error) || !Battle.ApplyResolvedAction(NativeAction(Source,Enemy.UnitEntityId),Error)) return false;
    TArray<uint8> Bytes; if (!Battle.ExportNativeCheckpoint(Bytes,Error)) { AddError(Error); return false; }
    FOGTurnBattle Restored; if (!Restored.RestoreNativeCheckpoint(Bytes,{},Error)) { AddError(Error); return false; }
    TestEqual(TEXT("Timeline beyond floating-point integer precision remains exact"),
        Restored.GetState().CurrentActionValue,Battle.GetState().CurrentActionValue);
    TestEqual(TEXT("Per-unit next action is restored exactly"),Restored.GetState().Units[0].NextActionValue,Battle.GetState().Units[0].NextActionValue);
    TestTrue(TEXT("Actual HP is restored without replaying damage"),Restored.GetState().Units[1].CurrentHp==FOGLargeNumber::FromInt64(800));
    FOGResolvedCombatAction Expanded;
    TestFalse(TEXT("Restoring does not reset the native owner-action cooldown"),
        Restored.ValidateResolvedAction(NativeAction(Source,Enemy.UnitEntityId),Expanded,Error));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FOGNativeCallbackRecoveryTest,
    "OfflineGame.Combat.Recovery.CallbacksRebindWithoutSerializedPermission",
    EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FOGNativeCallbackRecoveryTest::RunTest(const FString&)
{
    auto Source=NativeTestUnit(0,TEXT("test:callback.identity"));
    Source.SkillSet.ActiveSkills={FOGContentId(TEXT("test:callback.strike"))};
    auto Enemy=NativeTestUnit(1,TEXT("test:callback.identity"));
    FOGIdentityExclusivityContext OriginalIdentity;
    OriginalIdentity.Override=[](const auto&,const auto&){return EOGIdentityExclusivityOverrideDecision::AllowDuplicate;};
    FOGCombatConditionEvaluator OriginalConditions=[](const auto&,const auto&,const auto&){return true;};
    FOGTurnBattle Battle; FString Error;
    if (!Battle.Initialize(NativeInitial(Source,{Enemy}),{},OriginalConditions,OriginalIdentity,{},Error) ||
        !Battle.RegisterNativeAbility(NativeStrike(Source),Error) ||
        !Battle.ApplyResolvedAction(NativeAction(Source,Enemy.UnitEntityId),Error)) return false;
    TArray<uint8> Bytes; if (!Battle.ExportNativeCheckpoint(Bytes,Error)) {AddError(Error);return false;}
    FOGTurnBattle Restored;
    TestFalse(TEXT("A saved callback requirement cannot grant its own authority"),Restored.RestoreNativeCheckpoint(Bytes,{},Error));
    TestFalse(TEXT("Rejected callback restore publishes no partial battle"),Restored.GetState().BattleId.IsValid());
    int32 FreshCalls=0; bool Allow=false;
    auto Fresh=[&]()
    {
        FOGCombatRecoveryBindings B;
        B.Identity.Override=[&](const auto&,const auto&)
        {++FreshCalls;return Allow?EOGIdentityExclusivityOverrideDecision::AllowDuplicate:EOGIdentityExclusivityOverrideDecision::RejectDuplicate;};
        B.Conditions=[](const auto&,const auto&,const auto&){return true;}; return B;
    };
    TestFalse(TEXT("Fresh native authority can reject what the old session allowed"),
        Restored.RestoreNativeCheckpointWithBindings(Bytes,Fresh(),Error));
    TestTrue(TEXT("Restore evaluated the new native callback"),FreshCalls>0);
    Allow=true;
    if (!TestTrue(TEXT("Complete installed callback bindings restore atomically"),
        Restored.RestoreNativeCheckpointWithBindings(Bytes,Fresh(),Error))) {AddError(Error);return false;}
    TestTrue(TEXT("Restoration keeps actual already-resolved damage"),
        Restored.GetState().Units[1].CurrentHp==FOGLargeNumber::FromInt64(800));
    TestEqual(TEXT("Restoration does not replay the action log"),Restored.GetLog().GetNextSequence(),Battle.GetLog().GetNextSequence());
    return true;
}
#endif
