from pathlib import Path
r=Path(r"D:\UnrealProjects\OfflineGame_Gate12_Koikatsu_20261010\Source\OfflineGame")
h=r/"Public/Combat/OGTurnBattle.h"
c=r/"Private/Combat/OGTurnBattleLuminousBaseV6.cpp"
def patch(f,pairs):
 s=f.read_text(encoding="utf-8-sig")
 for a,b in pairs:
  n=s.count(a)
  if n!=1:raise RuntimeError(f"{f.name}: expected one found {n}: {a[:100]}")
  s=s.replace(a,b,1)
 f.write_text(s,encoding="utf8",newline="")
 print("PATCHED",f.name)
patch(h,[
 ("""        TSet<FOGEntityId> EclipseClaimTargets;
        bool bAllyRevivalSpent = false;""",
 """        TSet<FOGEntityId> EclipseClaimTargets;
        TMap<FOGEntityId,int32> EclipseClaimExpiryOwnerAction;
        bool bAllyRevivalSpent = false;"""),
 ("""        TMap<FOGEntityId,int32> HemalMarks;
        TMap<FOGEntityId,int32> HemorrhageStacks;""",
 """        TMap<FOGEntityId,int32> HemalMarks;
        TMap<FOGEntityId,int32> HemalMarkExpiryOwnerAction;
        TMap<FOGEntityId,int32> HemorrhageStacks;"""),
])
patch(c,[
 ("""    // Status effects belong to this Manifestation, never all characters who
    // share the Luminous Identity. Resolve actual HP damage before any siphon.
    TArray<FOGEntityId> Expired;""",
 """    // Source-authoritative Base v6 status lifetimes: Hemal Marks 3 owner
    // turns, Eclipse Claim 2, Dawn Oath 2. Never retain a hidden expired
    // mark after an enemy switches lane or the owner exits Day/Night.
    for(auto It=StateOfOwner.HemalMarks.CreateIterator();It;++It)
        if(StateOfOwner.HemalMarkExpiryOwnerAction.FindRef(It.Key())<
           CurrentOwnerAction)
        {
            StateOfOwner.HemalMarkExpiryOwnerAction.Remove(It.Key());
            StateOfOwner.PendingFourMarkTargetBonus.Remove(It.Key());
            It.RemoveCurrent();
        }
    for(auto It=StateOfOwner.EclipseClaimTargets.CreateIterator();It;++It)
        if(StateOfOwner.EclipseClaimExpiryOwnerAction.FindRef(*It)<
           CurrentOwnerAction)
        {
            StateOfOwner.EclipseClaimExpiryOwnerAction.Remove(*It);
            It.RemoveCurrent();
        }
    for(auto It=StateOfOwner.DawnOathByBeneficiary.CreateIterator();It;++It)
        if(It.Value()<CurrentOwnerAction)
            It.RemoveCurrent();

    // Status effects belong to this Manifestation, never all characters who
    // share the Luminous Identity. Resolve actual HP damage before any siphon.
    TArray<FOGEntityId> Expired;"""),
 ("""                StateOfOwner.HemalMarks.Add(TargetId,AfterMarks);
                if(BeforeMarks<4 && AfterMarks==4)""",
 """                StateOfOwner.HemalMarks.Add(TargetId,AfterMarks);
                StateOfOwner.HemalMarkExpiryOwnerAction.Add(
                    TargetId,CurrentOwnerAction+3);
                if(BeforeMarks<4 && AfterMarks==4)"""),
 ("""                StateOfOwner.HemorrhageEndOwnerAction.Add(TargetId,CurrentOwnerAction+3);""",
 """                StateOfOwner.HemorrhageEndOwnerAction.Add(TargetId,CurrentOwnerAction+2);"""),
 ("""                StateOfOwner.EclipseClaimTargets.Add(Id);
                if(bFourMarkBonus)""",
 """                StateOfOwner.EclipseClaimTargets.Add(Id);
                StateOfOwner.EclipseClaimExpiryOwnerAction.Add(
                    Id,CurrentOwnerAction+2);
                if(bFourMarkBonus)"""),
 ("""        if(StateOfOwner.bAttunementActive &&
           !StateOfOwner.bDayAttunement)
        {
            EffectiveAttacker.Speed=EffectiveAttacker.Speed*10800/10000;
            if(StateOfOwner.HemorrhageStacks.FindRef(TargetId)>0)""",
 """        if(StateOfOwner.bAttunementActive &&
           !StateOfOwner.bDayAttunement)
        {
            EffectiveAttacker.Speed=EffectiveAttacker.Speed*10800/10000;
            if(StateOfOwner.HemorrhageStacks.FindRef(TargetId)>0)"""),
 ("""        const FOGDamageResolution Damage=FOGCombatMath::ResolveDamage(
            Request,EffectiveAttacker,Target->Stats,Rng);""",
 """        // Night Ultimate's +20% penetration is confined to its own four
        // independently rolled strokes. No global passive is fabricated.
        if(bUlt && UltimateBranch==TEXT("night"))
            EffectiveAttacker.DefensePenetrationBps+=2000;
        const FOGDamageResolution Damage=FOGCombatMath::ResolveDamage(
            Request,EffectiveAttacker,Target->Stats,Rng);"""),
 ("""            Heal=FOGLargeNumber::ScaleByBasisPoints(Heal,12400);
            if(StateOfOwner.bAttunementActive && StateOfOwner.bDayAttunement)""",
 """            Heal=FOGLargeNumber::ScaleByBasisPoints(Heal,12400);
            const bool bConsumeDawnOath=bDayUltimate &&
                StateOfOwner.DawnOathByBeneficiary.FindRef(Id)>=CurrentOwnerAction;
            if(bConsumeDawnOath)
                Heal=FOGLargeNumber::ScaleByBasisPoints(Heal,12000);
            if(StateOfOwner.bAttunementActive && StateOfOwner.bDayAttunement)"""),
 ("""            if(bA3 && !Heal.IsZero())
                StateOfOwner.DawnOathByBeneficiary.Add(Id,CurrentOwnerAction+2);
            if(!Heal.IsZero() && !bGrantedOnHpDamage)""",
 """            if(bA3 && !Heal.IsZero())
                StateOfOwner.DawnOathByBeneficiary.Add(Id,CurrentOwnerAction+2);
            if(bConsumeDawnOath && !Heal.IsZero())
                StateOfOwner.DawnOathByBeneficiary.Remove(Id);
            if(!Heal.IsZero() && !bGrantedOnHpDamage)"""),
])
print("BASE_V6_CANONICAL_STATUS_DURATION_PENETRATION_AND_DAWN_OATH_CONSUMPTION_IMPLEMENTED")
