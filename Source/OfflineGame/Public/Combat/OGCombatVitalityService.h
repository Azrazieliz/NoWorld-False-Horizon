#pragma once
#include "CoreMinimal.h"
#include "Combat/OGCombatTypes.h"
#include "Persistence/OGWorldStore.h"

struct FOGTurnBattleState;
/** Durable HP belongs to the actual entity/life, never an identity-wide or UI cache.
 * Deterministic combat_vitality state rows leave original entity metadata untouched. */
struct OFFLINEGAME_API FOGStoredCombatVitality
{
    FOGEntityId EntityId,LifeId,BattleId,ReceiptId;
    FOGContentId IdentityId,VersionId;
    FOGLargeNumber CurrentHp,MaximumHpAtCommit;
    int64 WorldTick=0;
};
class OFFLINEGAME_API FOGCombatVitalityService
{
public:
    explicit FOGCombatVitalityService(IOGWorldStore& InStore):Store(InStore) {}
    bool Read(const FOGEntityId& Entity,bool& Found,FOGStoredCombatVitality& Out,FString& Error) const;
    /** Project exact stored HP, clamped down to the current resolved maximum. Never grants healing. */
    bool Project(FOGCombatUnitState& Unit,FString& Error) const;
    /** Caller owns the action transaction and rolls back on failure. Requires its already appended receipt. */
    bool Commit(const FOGTurnBattleState& Before,const FOGTurnBattleState& After,
        const FOGEntityId& Ruler,const FOGWorldEvent& Receipt,FString& Error);
private:
    IOGWorldStore& Store;
};
