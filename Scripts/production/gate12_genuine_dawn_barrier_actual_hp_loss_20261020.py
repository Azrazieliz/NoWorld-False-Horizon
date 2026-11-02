from pathlib import Path
r=Path(r"D:\UnrealProjects\OfflineGame_Gate12_Koikatsu_20261010\Source\OfflineGame")
h=r/"Public/Combat/OGTurnBattle.h"
core=r/"Private/Combat/OGTurnBattle.cpp"
base=r/"Private/Combat/OGTurnBattleLuminousBaseV6.cpp"
alter=r/"Private/Combat/OGTurnBattleLuminousAlterV1.cpp"
def edit(path,changes):
 s=path.read_text(encoding="utf-8-sig")
 for old,new in changes:
  n=s.count(old)
  if n!=1:raise RuntimeError(f"{path.name}: found {n}, expected one: {old[:120]!r}")
  s=s.replace(old,new,1)
 path.write_text(s,encoding="utf8",newline="")
 print("UPDATED",path.name)
edit(h,[
("""    // Per-Manifestation, per-battle state. Never a shared identity/global meter.
    TMap<FOGEntityId,FLuminousBaseBattleRuntime> LuminousBaseStates;""",
 """    // Per-Manifestation, per-battle state. Never a shared identity/global meter.
    TMap<FOGEntityId,FLuminousBaseBattleRuntime> LuminousBaseStates;

    struct FLuminousDawnBarrier
    {
        FOGEntityId GrantingManifestation;
        FOGEntityId Beneficiary;
        FOGLargeNumber Remaining;
        int32 ExpiresAfterOwnerAction = INDEX_NONE;
    };
    TArray<FLuminousDawnBarrier> LuminousDawnBarriers;
    void GrantAuthoredDawnOverhealBarrier(
        const FOGEntityId& OwnerId,
        const FOGEntityId& BeneficiaryId,
        const FOGLargeNumber& ExcessHealing);""")
])
edit(core,[
("""    LuminousBaseStates.Reset();
    LuminousAlterStates.Reset();""",
 """    LuminousBaseStates.Reset();
    LuminousDawnBarriers.Reset();
    LuminousAlterStates.Reset();"""),
("""    FOGLargeNumber NegativeAmount(
        -Amount.Significand,
        Amount.Exponent10);

    Target->CurrentHp =
        FOGLargeNumber::Add(
            Target->CurrentHp,
            NegativeAmount);""",
 """    // Dawn Sanctification P5 is a specific, source-owned 40% overheal
    // barrier capped at 8% target MaxHP, NOT a generic shield added to all
    // actors. Consume only live, unexpired layers; sorted per source GUID for
    // deterministic replay when two distinct Base Manifestations buff one ally.
    FOGLargeNumber EffectiveAmount=Amount;
    FOGLargeNumber Absorbed;
    TArray<int32> BarrierIndices;
    for(int32 I=0;I<LuminousDawnBarriers.Num();++I)
        if(LuminousDawnBarriers[I].Beneficiary==TargetUnitId)
            BarrierIndices.Add(I);
    BarrierIndices.Sort([this](int32 A,int32 B)
    {
        return LuminousDawnBarriers[A].GrantingManifestation.ToString() <
               LuminousDawnBarriers[B].GrantingManifestation.ToString();
    });
    for(const int32 I:BarrierIndices)
    {
        FLuminousDawnBarrier& Barrier=LuminousDawnBarriers[I];
        const FLuminousBaseBattleRuntime* OwnerState=
            LuminousBaseStates.Find(Barrier.GrantingManifestation);
        if(!OwnerState || OwnerState->OwnerActions>Barrier.ExpiresAfterOwnerAction ||
           FOGLargeNumber::Compare(Barrier.Remaining,FOGLargeNumber())<=0 ||
           EffectiveAmount.IsZero())
            continue;
        const FOGLargeNumber Used=
            FOGLargeNumber::Compare(EffectiveAmount,Barrier.Remaining)<0
                ?EffectiveAmount:Barrier.Remaining;
        Barrier.Remaining=FOGLargeNumber::Add(
            Barrier.Remaining,FOGLargeNumber(-Used.Significand,Used.Exponent10));
        EffectiveAmount=FOGLargeNumber::Add(
            EffectiveAmount,FOGLargeNumber(-Used.Significand,Used.Exponent10));
        Absorbed=FOGLargeNumber::Add(Absorbed,Used);
    }
    FOGLargeNumber NegativeAmount(
        -EffectiveAmount.Significand,
        EffectiveAmount.Exponent10);

    Target->CurrentHp =
        FOGLargeNumber::Add(
            Target->CurrentHp,
            NegativeAmount);"""),
("""    Damage.PayloadJson = FString::Printf(
        TEXT("{\\"amount\\":\\"%s\\"}"),
        *Amount.ToDebugString());""",
 """    Damage.PayloadJson = Absorbed.IsZero()
        ? FString::Printf(TEXT("{\\"amount\\":\\"%s\\"}"),
              *EffectiveAmount.ToDebugString())
        : FString::Printf(TEXT("{\\"amount\\":\\"%s\\",\\"dawn_barrier_absorbed\\":\\"%s\\",\\"nominal\\":\\"%s\\"}"),
              *EffectiveAmount.ToDebugString(),*Absorbed.ToDebugString(),
              *Amount.ToDebugString());"""),
])
edit(base,[
("""    else if(bA3 ||
            (bUlt && UltimateBranch==TEXT("day")))""",
 """    else if((bA3 && !bA3Covenant) ||
            (bUlt && UltimateBranch==TEXT("day")))"""),
("""        const FOGLargeNumber Actual=MinNumber(Before,Bleed);""",
 """        const FOGLargeNumber Actual=FOGLargeNumber::Add(
            Before,FOGLargeNumber(-T->CurrentHp.Significand,
                                   T->CurrentHp.Exponent10));"""),
("""        const FOGLargeNumber Actual=MinNumber(Before,Damage.TotalDamage);""",
 """        const FOGLargeNumber Actual=FOGLargeNumber::Add(
            Before,FOGLargeNumber(-Target->CurrentHp.Significand,
                                   Target->CurrentHp.Exponent10));"""),
("""            const FOGLargeNumber Missing=PositiveMissing(*T);
            if(Missing.IsZero())continue;
            const int32 Scale=bDayUltimate?""",
 """            const FOGLargeNumber Missing=PositiveMissing(*T);
            const int32 Scale=bDayUltimate?"""),
("""            Heal=MinNumber(Heal,Missing);
            if(!Heal.IsZero() &&
               !ApplyHealing(Source.UnitEntityId,Id,Heal,Action.SkillId,OutError))
                return false;
            StateOfOwner.BlessingExpiry.Add(Id,CurrentOwnerAction+2);
            if(bA3)
                StateOfOwner.DawnOathByBeneficiary.Add(Id,CurrentOwnerAction+2);
            if(!bGrantedOnHpDamage)""",
 """            const FOGLargeNumber UncappedHeal=Heal;
            Heal=MinNumber(Heal,Missing);
            if(!Heal.IsZero() &&
               !ApplyHealing(Source.UnitEntityId,Id,Heal,Action.SkillId,OutError))
                return false;
            const FOGLargeNumber Overheal=FOGLargeNumber::Add(
                UncappedHeal,
                FOGLargeNumber(-Heal.Significand,Heal.Exponent10));
            if(!Overheal.IsZero())
                GrantAuthoredDawnOverhealBarrier(Source.UnitEntityId,Id,Overheal);
            StateOfOwner.BlessingExpiry.Add(Id,CurrentOwnerAction+2);
            if(bA3 && !Heal.IsZero())
                StateOfOwner.DawnOathByBeneficiary.Add(Id,CurrentOwnerAction+2);
            if(!Heal.IsZero() && !bGrantedOnHpDamage)"""),
])
edit(alter,[
("""        const FOGLargeNumber Actual=AlterMin(T->CurrentHp,Tick);
            if(!ApplyDamage""",
 """        const FOGLargeNumber Before=T->CurrentHp;
            if(!ApplyDamage"""),
("""                Tick,Action.SkillId,OutError))return false;
            Own.RedTithe=FMath::Min(100,Own.RedTithe+5);""",
 """                Tick,Action.SkillId,OutError))return false;
            const FOGLargeNumber Actual=FOGLargeNumber::Add(
                Before,FOGLargeNumber(-T->CurrentHp.Significand,
                                      T->CurrentHp.Exponent10));
            if(Actual.IsZero())continue;
            Own.RedTithe=FMath::Min(100,Own.RedTithe+5);"""),
("""        const FOGLargeNumber Actual=AlterMin(T->CurrentHp,D.TotalDamage);
        if(!ApplyDamage(Source.UnitEntityId,Id,D.TotalDamage,
                        Action.SkillId,OutError))return false;
        if(!Actual.IsZero()""",
 """        const FOGLargeNumber Before=T->CurrentHp;
        if(!ApplyDamage(Source.UnitEntityId,Id,D.TotalDamage,
                        Action.SkillId,OutError))return false;
        const FOGLargeNumber Actual=FOGLargeNumber::Add(
            Before,FOGLargeNumber(-T->CurrentHp.Significand,
                                   T->CurrentHp.Exponent10));
        if(!Actual.IsZero()"""),
])
extra='''

void FOGTurnBattle::GrantAuthoredDawnOverhealBarrier(
    const FOGEntityId& OwnerId,const FOGEntityId& BeneficiaryId,
    const FOGLargeNumber& ExcessHealing)
{
    FLuminousBaseBattleRuntime* OwnerState=LuminousBaseStates.Find(OwnerId);
    const FOGCombatUnitState* Target=FindUnit(BeneficiaryId);
    if(!OwnerState || !Target || !Target->IsAlive() ||
       FOGLargeNumber::Compare(ExcessHealing,FOGLargeNumber())<=0)
        return;
    const FOGLargeNumber MaxBarrier=FOGLargeNumber::ScaleByBasisPoints(
        Target->Stats.MaxHp,800);
    const FOGLargeNumber Gift=FOGLargeNumber::ScaleByBasisPoints(
        ExcessHealing,4000);
    for(FLuminousDawnBarrier& Barrier:LuminousDawnBarriers)
        if(Barrier.GrantingManifestation==OwnerId &&
           Barrier.Beneficiary==BeneficiaryId)
        {
            if(OwnerState->OwnerActions>Barrier.ExpiresAfterOwnerAction)
                Barrier.Remaining=FOGLargeNumber();
            const FOGLargeNumber Candidate=FOGLargeNumber::Add(
                Barrier.Remaining,Gift);
            Barrier.Remaining=FOGLargeNumber::Compare(Candidate,MaxBarrier)<0
                ?Candidate:MaxBarrier;
            Barrier.ExpiresAfterOwnerAction=OwnerState->OwnerActions+2;
            return;
        }
    FLuminousDawnBarrier Barrier;
    Barrier.GrantingManifestation=OwnerId;
    Barrier.Beneficiary=BeneficiaryId;
    Barrier.Remaining=FOGLargeNumber::Compare(Gift,MaxBarrier)<0
        ?Gift:MaxBarrier;
    Barrier.ExpiresAfterOwnerAction=OwnerState->OwnerActions+2;
    LuminousDawnBarriers.Add(MoveTemp(Barrier));
}
'''
with base.open("a",encoding="utf8",newline="") as fp:fp.write(extra)
print("BARRIER_ORIGIN_PROVENANCE_AND_ACTUAL_HP_DAMAGE_DISPATCH_BOUND_NO_BUILD")
