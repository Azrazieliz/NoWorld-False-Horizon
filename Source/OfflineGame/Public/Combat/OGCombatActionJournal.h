#pragma once

#include "CoreMinimal.h"
#include "Combat/OGCombatTypes.h"
#include "Combat/OGTurnBattle.h"
#include "Events/OGWorldEvent.h"

/**
 * Builds the canonical causal record for an already accepted turn action.
 *
 * This is deliberately separate from FOGTurnBattle execution: callers provide
 * the detached before/after snapshots produced by the authoritative executor,
 * so the journal cannot invent damage, targets or outcomes from UI input.
 */
class OFFLINEGAME_API FOGCombatActionJournal
{
public:
    static bool BuildWorldRuntimeEvent(const FOGTurnBattleState& Before,const FOGTurnBattleState& After,
        const TArray<FOGNativeReactionRecord>& Reactions,const FOGEntityId& Ruler,int64 Tick,FName Operation,
        const FOGResolvedCombatAction* Cause,int32 StepIndex,const TArray<FOGWorldAbilityCastView>& ActiveCasts,
        FOGWorldEvent& Out,FString& Error);
    static bool BuildEncounterStartedEvent(const FOGTurnBattleState& Before,const FOGTurnBattleState& After,
        const TArray<FOGNativeReactionRecord>& Reactions,const FOGEntityId& RulerId,int64 WorldTick,
        FOGWorldEvent& Out,FString& Error);
    static bool BuildResolvedActionEvent(const FOGTurnBattleState& Before,const FOGTurnBattleState& After,
        const FOGResolvedCombatAction& Action,const TArray<FOGNativeReactionRecord>& Reactions,
        const FOGEntityId& RulerId,int64 WorldTick,FOGWorldEvent& Out,FString& Error);
    static bool BuildResolvedActionEvent(
        const FOGTurnBattleState& Before,
        const FOGTurnBattleState& After,
        const FOGResolvedCombatAction& Action,
        const FOGEntityId& OwningRulerId,
        int64 WorldTick,
        FOGWorldEvent& OutEvent,
        FString& OutError);
};
