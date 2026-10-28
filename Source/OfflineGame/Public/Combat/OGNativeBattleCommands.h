#pragma once
#include "CoreMinimal.h"
#include "Combat/OGCombatTypes.h"

/** Presentation choices for existing approved abilities; never a second executor. */
struct OFFLINEGAME_API FOGNativeBattleCommand
{
    FName Id;
    FString Label;
    FString Hint;
    FOGContentId SkillId;
    int32 ActiveSlot = INDEX_NONE;
    FName Branch;
    FName Mode;
    FName AuthoredMotion;
    bool bFantasm = false;
    bool bSelfTarget = false;

    void BuildAction(const FOGCombatUnitState& Source,
        const TArray<FOGEntityId>& Selected, FOGResolvedCombatAction& Out) const;
    FName MotionSemantic() const;
};
struct OFFLINEGAME_API FOGNativeBattleCommandSet
{
    FOGResolvedSkillSet ApprovedSkills;
    TArray<FOGNativeBattleCommand> Commands;
    FString ResourceLabel;
    int32 ResourceCap = 0;
    int32 ChargeCap = 0;
};
class OFFLINEGAME_API FOGNativeBattleCommands
{
public:
    /** Exact frozen kit identity and full owned skill set are verified before exposure. */
    static bool BuildFor(const FOGCombatUnitState& Source, FOGNativeBattleCommandSet& Out, FString& Error);
};
