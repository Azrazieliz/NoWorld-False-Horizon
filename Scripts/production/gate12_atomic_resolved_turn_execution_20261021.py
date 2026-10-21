from pathlib import Path
r=Path(r"D:\UnrealProjects\OfflineGame_Gate12_Koikatsu_20261010\Source\OfflineGame")
h=r/"Public/Combat/OGTurnBattle.h"
c=r/"Private/Combat/OGTurnBattle.cpp"
s=h.read_text(encoding="utf-8-sig")
find="""private:
    struct FLuminousFieldState
"""
replace="""private:
    // Atomic action transaction: the currently committed Battle must never
    // retain partial HP, resource, trigger or log changes from a rejected
    // authored multi-hit or revival action.
    bool ApplyResolvedActionMutating(
        const FOGResolvedCombatAction& Action,
        FString& OutError);

    struct FLuminousFieldState
"""
if s.count(find)!=1:raise RuntimeError("Expected one private battle section")
h.write_text(s.replace(find,replace,1),encoding="utf8",newline="")
s=c.read_text(encoding="utf-8-sig")
find="""bool FOGTurnBattle::ApplyResolvedAction(
    const FOGResolvedCombatAction& Action,
    FString& OutError)
{
    OutError.Reset();"""
replace="""bool FOGTurnBattle::ApplyResolvedAction(
    const FOGResolvedCombatAction& Action,
    FString& OutError)
{
    // Existing FOGTurnBattle copy semantics are used by the deterministic
    // diagnostic encounter and replay path. Execute on a detached snapshot,
    // then move only successful results into the real combat state.
    // Critical for multi-strike costs, Dawn barriers, OnDefeat reactions,
    // field contests, event queues and action-value timeline consistency.
    FOGTurnBattle Candidate=*this;
    FString CandidateError;
    if(!Candidate.ApplyResolvedActionMutating(Action,CandidateError))
    {
        OutError=CandidateError.IsEmpty()
            ?TEXT("Resolved action rejected; no battle state was committed.")
            :MoveTemp(CandidateError);
        return false;
    }
    *this=MoveTemp(Candidate);
    OutError.Reset();
    return true;
}

bool FOGTurnBattle::ApplyResolvedActionMutating(
    const FOGResolvedCombatAction& Action,
    FString& OutError)
{
    OutError.Reset();"""
if s.count(find)!=1:raise RuntimeError("Expected one real battle action method")
c.write_text(s.replace(find,replace,1),encoding="utf8",newline="")
print("GATE12_ACTUAL_CORE_BATTLE_FULL_ACTION_TRANSACTION_APPLIED")
