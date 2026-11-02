from pathlib import Path
r=Path(r"D:\UnrealProjects\OfflineGame_Gate12_Koikatsu_20261010\Source\OfflineGame")
h=r/"Public/Combat/OGTurnBattle.h"
text=h.read_text(encoding="utf-8-sig")
def sub(a,b):
 global text
 n=text.count(a)
 if n!=1:raise RuntimeError(f"expected unique {a[:100]}, found {n}")
 text=text.replace(a,b,1)
sub('#include "Combat/OGCombatTypes.h"', '#include "Combat/OGCombatTypes.h"\n#include "Rules/OGRulePriority.h"')
sub('''class OFFLINEGAME_API FOGTurnBattle
{''','''/**
 * An effect instance already validated by the registered mechanic engine.
 * Source-provenance, authoritative potency and spatial checks are performed
 * by the engine supplying this event, NEVER by a user-controlled JSON flag.
 */
enum class EOGLuminousFieldRule : uint8
{
    BaseNocturneSeverance, BaseRekindlingEdict, BaseLastRose,
    BaseBoundaryCollision, AlterBloodReversal, AlterThornUnbinding,
    AlterMirrorDebt, AlterBoundaryDecree
};
struct FOGLuminousRegisteredEffectContest
{
    EOGLuminousFieldRule Rule = EOGLuminousFieldRule::BaseNocturneSeverance;
    FOGEntityId OwnerManifestationId;
    FOGEntityId TargetUnitId;
    FOGContentId RegisteredEffectId;
    FOGEntityId EffectSourceId;
    FOGRulePriority AttackingPriority;
    FOGRulePriority DefendingPriority;
    // These statements must originate from registered typed content/state;
    // UI JSON must never be accepted as a substitute.
    bool bRegisteredTemporaryEffect = false;
    bool bSpecificEffectFamilyEligible = false;
    bool bPerceptuallyObserved = false;
    bool bInAuthorizedFieldFootprint = false;
    bool bProjectionCopyNotOriginal = false;
    bool bPreviouslyRecordedSameContentEffect = false;
    bool bCooldownElapsed = false; // actual World ms deadline (not inferred)
    bool bBoundaryOverlapSegment = false;
};
using FOGLuminousAcceptedFieldEligibility = TFunction<
    bool(const FOGCombatUnitState&,const FOGContentId&,FString&)>;
using FOGLuminousRegisteredEffectRemoval = TFunction<
    bool(const FOGLuminousRegisteredEffectContest&,FString&)>;

class OFFLINEGAME_API FOGTurnBattle
{''')
sub('''    int32 GetResolvedLuminousUltimateCharge(
        const FOGEntityId& ManifestationId) const;

    int32 SelectNextActingUnitIndex() const;''','''    int32 GetResolvedLuminousUltimateCharge(
        const FOGEntityId& ManifestationId) const;

    // Gate 00 Registry/current MR / effect and field ownership gate, provided
    // by authoritative native runtime. Absent gate means activation DENIED.
    void BindApprovedLuminousFieldEligibility(
        FOGLuminousAcceptedFieldEligibility InGate)
    {
        LuminousFieldEligibility=MoveTemp(InGate);
    }

    // Registered effect engine supplies one concrete effect instance and a
    // lawful on-success removal callback; no global immunity suppression.
    bool TryResolveLuminousRegisteredFieldContest(
        const FOGLuminousRegisteredEffectContest& Contest,
        const FOGLuminousRegisteredEffectRemoval& RemoveOneEligibleEffect,
        bool& bOutWonSpecificContest,
        FString& OutError);

    int32 SelectNextActingUnitIndex() const;''')
sub('''private:
    struct FLuminousBaseBattleRuntime''','''private:
    struct FLuminousFieldState
    {
        FOGContentId OwnerFantasmId;
        int32 ActivatedOwnerAction = 0;
        int32 ExpiresBeforeOwnerAction = INDEX_NONE;
        int32 BaseNocturneSuccessfulTargetCount = 0;
        TSet<FOGEntityId> NocturneConsumedTargets;
        TSet<FOGEntityId> RekindledAllies;
        bool bLastRoseUsed = false;
        int32 AlterBloodReversalSuccessCount = 0;
        TSet<FOGEntityId> AlterThornConsumedTargets;
        bool bAlterMirrorDebtUsed = false;
    };
    TMap<FOGEntityId,FLuminousFieldState> LuminousFields;
    FOGLuminousAcceptedFieldEligibility LuminousFieldEligibility;
    bool TryActivateLuminousAuthoredField(
        const FOGCombatUnitState& Unit,
        const FOGContentId& ExactFantasmId,
        FString& OutError);

    struct FLuminousBaseBattleRuntime''')
sub('''        int32 DawnOathExpiry = 0;
    };''','''        int32 DawnOathExpiry = 0;
        TMap<FOGEntityId,int32> DawnOathByBeneficiary;
        int32 NextFantasmReadyOwnerAction = 0;
    };''')
h.write_text(text,encoding="utf8",newline="")
print("ADDED_AUTHENTIC_FIELD_ACTIVATION_CONTEST_PUBLIC_API")
