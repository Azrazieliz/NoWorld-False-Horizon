#pragma once

#include "CoreMinimal.h"
#include "Combat/OGCombatTypes.h"
#include "Persistence/OGWorldStore.h"

/** An authored cost may describe currency or an aggregated material. Both use
 * the existing canonical resource account; no second wallet is introduced. */
struct OFFLINEGAME_API FOGDevelopmentCost
{
    FOGContentId ResourceId;
    FString DisplayName;
    int64 Amount = 0;
};

struct OFFLINEGAME_API FOGDevelopmentRouteRequirement
{
    FOGContentId RouteId;
    FOGContentId NodeId;
    FName State;
};

struct OFFLINEGAME_API FOGDevelopmentFactRequirement
{
    FName FactKey;
    FString DisplayName;
    bool bSubjectIsRuler = false;
};

/** One explicit development choice. Tiers, costs, maxima and stat grants are
 * supplied by character content, never generated from a universal curve. */
struct OFFLINEGAME_API FOGCharacterDevelopmentDefinition
{
    FOGContentId DevelopmentId;
    FOGContentId IdentityId;
    FOGContentId VersionId;
    FString DisplayName;
    FString Description;
    int32 MinimumLevel = 1;
    FName RequiredRarity;
    FName RequiredReinforcementState;
    TArray<FOGContentId> PrerequisiteDevelopmentIds;
    TArray<FOGContentId> ExcludedDevelopmentIds;
    TArray<FOGDevelopmentRouteRequirement> RequiredRoutes;
    TArray<FOGDevelopmentFactRequirement> RequiredFacts;
    TArray<FOGDevelopmentCost> Costs;
    int32 ResultLevel = 0; // zero preserves current level
    FName ResultRarity; // None preserves current rarity
    FName ResultReinforcementState;
    bool bCompletesReinforcement = false;
    TMap<FName, FOGLargeNumber> StatGrants;
    TArray<FOGDevelopmentRouteRequirement> GrantedRoutes;
    TArray<FOGContentId> GrantedForms;
};

struct OFFLINEGAME_API FOGDevelopmentCostPreview
{
    FOGDevelopmentCost Cost;
    int64 Balance = 0;
};

struct OFFLINEGAME_API FOGCharacterDevelopmentPreview
{
    FOGContentId DevelopmentId;
    FOGEntityId ManifestationId;
    FString DisplayName;
    FString Description;
    FOGCharacterManifestationRecord Current;
    FOGCharacterManifestationRecord Result;
    FName CurrentReinforcementState;
    FName ResultReinforcementState;
    bool bCurrentlyMaxReinforced = false;
    bool bResultMaxReinforced = false;
    bool bAlreadyCompleted = false;
    bool bCanApply = false;
    TArray<FOGDevelopmentCostPreview> Costs;
    TMap<FName, FOGLargeNumber> StatGrants;
    TMap<FName, FOGLargeNumber> CurrentEarnedStats;
    TMap<FName, FOGLargeNumber> ResultEarnedStats;
    TArray<FString> MissingRequirements;
    /** Fingerprint of definition + relevant canonical state, rechecked under
     * the write transaction. UI confirmation cannot apply a stale quote. */
    FString ConfirmationToken;
};

class OFFLINEGAME_API FOGCharacterDevelopmentCatalog
{
public:
    static bool LoadInstalled(TArray<FOGCharacterDevelopmentDefinition>& Out,
        FString& Error);
    static bool Parse(const FString& Json,
        TArray<FOGCharacterDevelopmentDefinition>& Out, FString& Error);
    static bool Validate(const TArray<FOGCharacterDevelopmentDefinition>& Definitions,
        FString& Error);
    static bool ValidateDefinition(const FOGCharacterDevelopmentDefinition& Definition,
        FString& Error);
};

/** Owned development commands share SQLite transactions, world events and
 * progression records with combat, roster and Grand Convergence. */
class OFFLINEGAME_API FOGCharacterDevelopmentService
{
public:
    explicit FOGCharacterDevelopmentService(IOGWorldStore& InStore) : Store(InStore) {}

    bool Preview(const FOGEntityId& RulerId, const FOGEntityId& ManifestationId,
        const FOGCharacterDevelopmentDefinition& Definition,
        FOGCharacterDevelopmentPreview& Out, FString& Error) const;
    bool Apply(const FOGEntityId& RulerId, const FOGEntityId& ManifestationId,
        const FOGCharacterDevelopmentDefinition& Definition,
        const FString& ConfirmationToken, const FOGEntityId& RequestId,
        int64 WorldTick, bool& bOutAlreadyApplied, FString& Error);
    /** Add earned development grants to fresh content-resolved base stats.
     * Call once when constructing a battle snapshot, never once per hit. */
    bool ApplyEarnedStats(const FOGEntityId& ManifestationId,
        FOGCombatStats& InOutStats, FString& Error) const;
    /** Idempotent bridge for both action-party and turn-battle snapshots. */
    bool ProjectCombatUnit(FOGCombatUnitState& InOutUnit, FString& Error) const;

private:
    IOGWorldStore& Store;
};
