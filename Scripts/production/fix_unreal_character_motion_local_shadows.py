"""Repair actor inherited-controller name shadowing and UE_LOG else syntax."""
from pathlib import Path
import re
path=Path(r"D:\UnrealProjects\Gate12_WholeCharacter_Build_20261031\Source\OfflineGame\Private\Characters\OGContentCharacterActors.cpp")
s=path.read_text("utf8")
old='''    auto* Controller=Appearance->GetMotionController();
    if (!Controller || !Controller->EnqueueCombatAction(Semantic,Battle->GetState().BattleId,Action.ActionId))
        UE_LOG(LogTemp, Warning, TEXT("Source combat motion %s is unavailable: %s"), *Semantic.ToString(), *Error);
    else LastPresentedActionId = Action.ActionId;'''
new='''    UOGSharedMotionControllerComponent* ActionMotion = Appearance->GetMotionController();
    if (!ActionMotion || !ActionMotion->EnqueueCombatAction(Semantic,Battle->GetState().BattleId,Action.ActionId))
    {
        UE_LOG(LogTemp, Warning,
            TEXT("Source combat motion %s is unavailable: %s"),
            *Semantic.ToString(), *Error);
    }
    else
    {
        LastPresentedActionId = Action.ActionId;
    }'''
assert s.count(old)==1
s=s.replace(old,new)
start=s.index('void AOGContentPlayableCharacter::Tick(')
end=s.find('\nvoid AOGContentPlayableCharacter::',start+10)
if end<0:
 end=s.find('\nAOGContentPlayableCharacter::',start+10)
assert end>start
segment=s[start:end]
assert 'UOGSharedMotionControllerComponent* Controller = Appearance->GetMotionController();' in segment
segment=re.sub(r'\bController\b','TraversalMotion',segment)
s=s[:start]+segment+s[end:]
path.write_text(s,"utf8")
print("SOURCE_AUTHENTIC_COMBAT_AND_TRAVERSAL_CONTROLLER_CXX_FIXED")
