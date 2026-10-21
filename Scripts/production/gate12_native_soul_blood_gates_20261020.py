from pathlib import Path
root=Path(r"D:\UnrealProjects\OfflineGame_Gate12_Koikatsu_20261010\Source\OfflineGame")
h=root/"Public/Combat/OGTurnBattle.h"
b=root/"Private/Combat/OGTurnBattleLuminousBaseV6.cpp"
a=root/"Private/Combat/OGTurnBattleLuminousAlterV1.cpp"
core=root/"Private/Combat/OGTurnBattle.cpp"
def pat(file,pairs):
 s=file.read_text(encoding="utf-8-sig")
 for old,new in pairs:
  n=s.count(old)
  if n!=1:raise ValueError(f"{file.name} expected one found {n}: {old[:100]}")
  s=s.replace(old,new,1)
 file.write_text(s,encoding="utf-8",newline="")
 print("CHANGED",file.name)
pat(h,[
("""using FOGLuminousAcceptedFieldEligibility = TFunction<
    bool(const FOGCombatUnitState&,const FOGContentId&,FString&)>;""",
 """using FOGLuminousAcceptedFieldEligibility = TFunction<
    bool(const FOGCombatUnitState&,const FOGContentId&,FString&)>;

// Registered native source of truth only. Never parse client-provided claims
// of blood anatomy, susceptibility, soul continuity or superior Authority.
using FOGLuminousNativeBloodEligibility = TFunction<bool(
    const FOGCombatUnitState& Attacker,
    const FOGCombatUnitState& Target,
    const FOGContentId& RegisteredStatusFamily)>;
using FOGLuminousNativeRevivalEligibility = TFunction<bool(
    const FOGCombatUnitState& RestoringManifestation,
    const FOGCombatUnitState& IncapacitatedTarget,
    bool bAllyCovenant)>;
"""),
("""    // Gate 00 Registry/current MR / effect and field ownership gate, provided""",
 """    void BindLuminousAuthoritativeStatusGates(
        FOGLuminousNativeBloodEligibility InBlood,
        FOGLuminousNativeRevivalEligibility InRevival)
    {
        LuminousBloodEligibility=MoveTemp(InBlood);
        LuminousRevivalEligibility=MoveTemp(InRevival);
    }

    // Gate 00 Registry/current MR / effect and field ownership gate, provided"""),
("""    FOGLuminousAcceptedFieldEligibility LuminousFieldEligibility;
    bool TryActivateLuminousAuthoredField(""",
 """    FOGLuminousAcceptedFieldEligibility LuminousFieldEligibility;
    FOGLuminousNativeBloodEligibility LuminousBloodEligibility;
    FOGLuminousNativeRevivalEligibility LuminousRevivalEligibility;
    bool IsLuminousBloodStatusEligible(
        const FOGEntityId& AttackerId,
        const FOGEntityId& TargetId,
        const FOGContentId& StatusFamily) const
    {
        if(!LuminousBloodEligibility)return false;
        const FOGCombatUnitState* Attacker=FindUnit(AttackerId);
        const FOGCombatUnitState* Target=FindUnit(TargetId);
        return Attacker && Target &&
            LuminousBloodEligibility(*Attacker,*Target,StatusFamily);
    }
    bool IsLuminousRevivalEligible(
        const FOGEntityId& RestoringId,
        const FOGEntityId& TargetId,
        bool bAlly) const
    {
        if(!LuminousRevivalEligibility)return false;
        const FOGCombatUnitState* Restoring=FindUnit(RestoringId);
        const FOGCombatUnitState* Target=FindUnit(TargetId);
        return Restoring && Target &&
            LuminousRevivalEligibility(*Restoring,*Target,bAlly);
    }
    bool TryActivateLuminousAuthoredField("""),
("""        bool bSelfSoulContinuityVerified = false;
        bool bNoSuperiorRevivalNegationVerified = false;
""", ""),
("""        bool bCovenantSoulVerified = false;
        bool bCovenantNegationClear = false;
""", ""),
("""        bool bSoulContinuityVerified = false;
        bool bNoSuperiorRevivalNegationVerified = false;
""", "")
])
pat(core,[
("""    LuminousFieldEligibility=FOGLuminousAcceptedFieldEligibility();
    IdentityContext =""",
 """    LuminousFieldEligibility=FOGLuminousAcceptedFieldEligibility();
    LuminousBloodEligibility=FOGLuminousNativeBloodEligibility();
    LuminousRevivalEligibility=FOGLuminousNativeRevivalEligibility();
    IdentityContext =""")
])
pat(b,[
("""    bool bPreparedSoulVerified=false,bPreparedNegationClear=false;
    if(bA3Covenant)
    {
        if(!Context->TryGetBoolField(TEXT("verified_target_soul_continuity"),
                                     bPreparedSoulVerified) ||
           !bPreparedSoulVerified ||
           !Context->TryGetBoolField(TEXT("verified_no_superior_revival_negation"),
                                     bPreparedNegationClear) ||
           !bPreparedNegationClear)
        {
            OutError=TEXT("A3 Covenant requires explicit lawful soul and higher-order Authority verification.");
            return false;
        }
    }""",
 """    if(bA3Covenant &&
       !IsLuminousRevivalEligible(Source.UnitEntityId,
                                 Action.TargetUnitIds[0],true))
    {
        OutError=TEXT("A3 Covenant preparation denied by authoritative native soul and superior Authority rules.");
        return false;
    }"""),
("""        StateOfOwner.bCovenantSoulVerified=bPreparedSoulVerified;
        StateOfOwner.bCovenantNegationClear=bPreparedNegationClear;
""",""),
("""    bool bSoulVerified=false,bUnnegated=false;
    if(Context->TryGetBoolField(TEXT("verified_soul_continuity"),bSoulVerified) &&
       Context->TryGetBoolField(TEXT("verified_no_superior_revival_negation"),bUnnegated))
    {
        StateOfOwner.bSelfSoulContinuityVerified=bSoulVerified;
        StateOfOwner.bNoSuperiorRevivalNegationVerified=bUnnegated;
    }

""",""),
("""    TSet<FOGEntityId> BloodEligible;
    const TArray<TSharedPtr<FJsonValue>>* BloodIds=nullptr;
    if(Context->TryGetArrayField(TEXT("verified_blood_eligible_targets"),BloodIds))
        for(const TSharedPtr<FJsonValue>& V:*BloodIds)
        {
            FGuid G;
            if(FGuid::Parse(V->AsString(),G))
                BloodEligible.Add(FOGEntityId(G));
        }""",
 """    TSet<FOGEntityId> BloodEligible;
    for(const FOGEntityId& TargetId:Action.TargetUnitIds)
        if(IsLuminousBloodStatusEligible(Source.UnitEntityId,TargetId,
            FOGContentId(TEXT("combat:bleed"))))
            BloodEligible.Add(TargetId);"""),
("""       !Runtime->bSelfSoulContinuityVerified ||
       !Runtime->bNoSuperiorRevivalNegationVerified ||
       Unit->Presence!=EOGCombatPresence::Defeated ||""",
 """       !IsLuminousRevivalEligible(UnitId,UnitId,false) ||
       Unit->Presence!=EOGCombatPresence::Defeated ||"""),
("""           S->bAllyRevivalSpent || !S->bCovenantSoulVerified ||
           !S->bCovenantNegationClear ||
           S->PreparedCovenantTarget!=TargetUnitId ||""",
 """           S->bAllyRevivalSpent ||
           !IsLuminousRevivalEligible(OwnerId,TargetUnitId,true) ||
           S->PreparedCovenantTarget!=TargetUnitId ||""")
])
pat(a,[
("""    bool bSoul=false,bUnnegated=false;
    if(Context->TryGetBoolField(TEXT("verified_soul_continuity"),bSoul) &&
       Context->TryGetBoolField(TEXT("verified_no_superior_revival_negation"),bUnnegated))
    {
        Own.bSoulContinuityVerified=bSoul;
        Own.bNoSuperiorRevivalNegationVerified=bUnnegated;
    }

""",""),
("""    TSet<FOGEntityId> Eligible;
    const TArray<TSharedPtr<FJsonValue>>* Blood=nullptr;
    if(Context->TryGetArrayField(TEXT("verified_blood_eligible_targets"),Blood))
        for(const TSharedPtr<FJsonValue>& V:*Blood)
        {
            FGuid G;
            if(FGuid::Parse(V->AsString(),G))Eligible.Add(FOGEntityId(G));
        }""",
 """    TSet<FOGEntityId> Eligible;
    for(const FOGEntityId& TargetId:Action.TargetUnitIds)
        if(IsLuminousBloodStatusEligible(Source.UnitEntityId,TargetId,
            FOGContentId(TEXT("combat:bleed"))))
            Eligible.Add(TargetId);"""),
("""       !R->bSoulContinuityVerified ||
       !R->bNoSuperiorRevivalNegationVerified ||
       U->Presence!=EOGCombatPresence::Defeated ||""",
 """       !IsLuminousRevivalEligible(UnitId,UnitId,false) ||
       U->Presence!=EOGCombatPresence::Defeated ||""")
])
print("AUTHORITATIVE_TYPED_STATUS_REVIVAL_GATES_INTEGRATED")
