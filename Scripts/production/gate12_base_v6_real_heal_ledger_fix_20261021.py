from pathlib import Path
p=Path(r"D:\UnrealProjects\OfflineGame_Gate12_Koikatsu_20261010\Source\OfflineGame\Private\Combat\OGTurnBattleLuminousBaseV6.cpp")
s=p.read_text(encoding="utf-8-sig")
a="""        const bool bDayUltimate=bUlt;
        for(int32 I=0;I<Action.TargetUnitIds.Num();++I)"""
b="""        const bool bDayUltimate=bUlt;
        // Benediction and Day Ultimate heal from actual caster ATK, including
        // only currently active, approved Dawn attack blessings on the caster.
        int32 CasterAtkBps=0,CasterSpeedBps=0,CasterHealBps=0,CasterMitigationBps=0;
        GetApprovedDawnBlessingBps(Source.UnitEntityId,CasterAtkBps,
            CasterSpeedBps,CasterHealBps,CasterMitigationBps);
        const FOGLargeNumber HealerAttack=CasterAtkBps>0
            ?FOGLargeNumber::ScaleByBasisPoints(
                Source.Stats.Attack,10000+CasterAtkBps)
            :Source.Stats.Attack;
        for(int32 I=0;I<Action.TargetUnitIds.Num();++I)"""
assert s.count(a)==1
s=s.replace(a,b,1)
a="""            const FOGLargeNumber Missing=PositiveMissing(*T);
            const int32 Scale=bDayUltimate?(I==0?22000:9500):
                                          (I==0?6500:2500);
            FOGLargeNumber Heal=FOGLargeNumber::ScaleByBasisPoints(
                Source.Stats.Attack,Scale);
            Heal=FOGLargeNumber::ScaleByBasisPoints(Heal,12400);
            const bool bConsumeDawnOath=bDayUltimate &&
                StateOfOwner.DawnOathByBeneficiary.FindRef(Id)>=CurrentOwnerAction;
            if(bConsumeDawnOath)
                Heal=FOGLargeNumber::ScaleByBasisPoints(Heal,12000);
            if(StateOfOwner.bAttunementActive && StateOfOwner.bDayAttunement)
                Heal=FOGLargeNumber::ScaleByBasisPoints(Heal,11200);
            const FOGLargeNumber UncappedHeal=Heal;
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
            StateOfOwner.BlessingCategory.Add(Id,bDayUltimate?2:1);
            if(bA3 && !Heal.IsZero())
                StateOfOwner.DawnOathByBeneficiary.Add(Id,CurrentOwnerAction+2);
            if(bConsumeDawnOath && !Heal.IsZero())
                StateOfOwner.DawnOathByBeneficiary.Remove(Id);
            if(!Heal.IsZero() && !bGrantedOnHpDamage)"""
b="""            const int32 Scale=bDayUltimate?(I==0?22000:9500):
                                          (I==0?6500:2500);
            FOGLargeNumber Heal=FOGLargeNumber::ScaleByBasisPoints(
                HealerAttack,Scale);
            Heal=FOGLargeNumber::ScaleByBasisPoints(Heal,12400);
            const int32* OathExpiry=
                StateOfOwner.DawnOathByBeneficiary.Find(Id);
            const bool bConsumeDawnOath=bDayUltimate && OathExpiry &&
                *OathExpiry>=CurrentOwnerAction;
            if(bConsumeDawnOath)
                Heal=FOGLargeNumber::ScaleByBasisPoints(Heal,12000);
            if(StateOfOwner.bAttunementActive && StateOfOwner.bDayAttunement)
                Heal=FOGLargeNumber::ScaleByBasisPoints(Heal,11200);
            // The actual shared healer applies target HealingReceived AFTER
            // this skill, and clamps at MaxHp. Never cap this amount first:
            // doing so double counts or loses overheal when a blessing is up.
            int32 RecipientAtk=0,RecipientSpeed=0;
            int32 RecipientHeal=0,RecipientReduction=0;
            GetApprovedDawnBlessingBps(Id,RecipientAtk,RecipientSpeed,
                RecipientHeal,RecipientReduction);
            const FOGLargeNumber GrossHealing=RecipientHeal>0
                ?FOGLargeNumber::ScaleByBasisPoints(
                    Heal,10000+RecipientHeal):Heal;
            const FOGLargeNumber BeforeHealing=T->CurrentHp;
            if(!Heal.IsZero() &&
               !ApplyHealing(Source.UnitEntityId,Id,Heal,Action.SkillId,OutError))
                return false;
            const FOGLargeNumber RealRestored=FOGLargeNumber::Add(
                T->CurrentHp,FOGLargeNumber(
                    -BeforeHealing.Significand,BeforeHealing.Exponent10));
            // P5 Dawn Sanctification converts 40% of genuinely unused
            // computed healing into its separately capped 8%-MaxHP barrier.
            const FOGLargeNumber UnusedHealing=FOGLargeNumber::Add(
                GrossHealing,FOGLargeNumber(
                    -RealRestored.Significand,RealRestored.Exponent10));
            if(FOGLargeNumber::Compare(UnusedHealing,FOGLargeNumber())>0)
                GrantAuthoredDawnOverhealBarrier(
                    Source.UnitEntityId,Id,UnusedHealing);
            StateOfOwner.BlessingExpiry.Add(Id,CurrentOwnerAction+2);
            StateOfOwner.BlessingCategory.Add(Id,bDayUltimate?2:1);
            if(bA3 && FOGLargeNumber::Compare(
                RealRestored,FOGLargeNumber())>0)
                StateOfOwner.DawnOathByBeneficiary.Add(
                    Id,CurrentOwnerAction+2);
            if(bConsumeDawnOath && FOGLargeNumber::Compare(
                RealRestored,FOGLargeNumber())>0)
                StateOfOwner.DawnOathByBeneficiary.Remove(Id);
            if(FOGLargeNumber::Compare(
                RealRestored,FOGLargeNumber())>0 && !bGrantedOnHpDamage)"""
assert s.count(a)==1, s.count(a)
s=s.replace(a,b,1)
p.write_text(s,encoding="utf8",newline="")
print("CHECKPOINT_1: canonical recipient healing resolved once; actual restoration drives Dawn Oath, reserve, overheal barrier")
