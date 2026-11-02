#pragma once

#include "CoreMinimal.h"
#include "Characters/OGCharacterDefinitions.h"
#include "Combat/OGTurnBattle.h"

class IOGWorldStore;

/**
 * Inputs MUST be resolved by installed, validated Gate 11 Registry and the
 * native World/effect/soul services. Draft JSON and UI state are not authority.
 * The owner GameInstance must release its battle before closing WorldStore.
 */
struct OFFLINEGAME_API FOGLuminousCanonicalSources
{
    TFunction<bool(const FOGContentId&, FOGCharacterIdentityDefinition&, FString&)>
        ReadInstalledIdentity;
    // Exact registered version -> approved Fantasm, not a name-prefix guess.
    TMap<FOGContentId, FOGContentId> FantasmByRegisteredVersion;
    FOGContentId WorldNegationGradeId;
    FOGContentId RegisteredBleedFamilyId;
    FOGLuminousNativeBloodEligibility ReadNativeBloodEligibility;
    FOGLuminousNativeRevivalEligibility ReadNativeSoulContinuity;
    FOGLuminousRegisteredContestEvidence ValidateRegisteredEffectContest;
    // Must validate live World ownership / active field authority independently.
    TFunction<bool(const FOGEntityId&, const FOGContentId&, FString&)>
        CheckRegisteredWorldFieldOwnership;
};

/** Strict integration into FOGTurnBattle. No alternate damage/turn executor. */
class OFFLINEGAME_API FOGLuminousCanonicalAuthorityBinding
{
public:
    static bool Bind(
        FOGTurnBattle& Battle,
        IOGWorldStore& Store,
        const FOGEntityId& OwningRulerId,
        FOGLuminousCanonicalSources Sources,
        FString& OutError);
};
