#pragma once

#include "CoreMinimal.h"
#include "Gacha/OGGachaDefinitions.h"
#include "Persistence/OGWorldStore.h"

/** Exact existing account inputs; tickets retain authored priority. */
struct OFFLINEGAME_API FOGGachaPaymentPreview
{
    FOGContentId ResourceId; int64 Balance=0,Spend=0; bool bTicket=false;
};
struct OFFLINEGAME_API FOGGachaBatchPreview
{
    FOGEntityId RulerId; FOGContentId BannerId; int32 Count=0;
    TArray<FOGGachaPaymentPreview> Payments;
    FOGContentId BatchTicketId;
    FName GuaranteedTicketRarity;
    FOGGachaStateRecord Pity;
    int32 NextTopRarityChanceBps=0;
    bool bAccessUnlocked=false,bCanPull=false;
    TArray<FString> MissingRequirements;
    FString ConfirmationToken;
};

/**
 * Deterministic, persistent acquisition service.
 *
 * Banner content is immutable definition data. Pity/guarantee state and owned
 * Manifestations live in the authoritative world store.
 */
class OFFLINEGAME_API FOGGachaService
{
public:
    explicit FOGGachaService(IOGWorldStore& InStore)
        : Store(InStore)
    {
    }

    bool Pull(
        const FOGGachaBannerDefinition& Banner,
        const FOGEntityId& RulerId,
        int64 WorldTick,
        int64 Seed,
        FOGGachaPullResult& OutResult,
        FString& OutError);

    /** All acquisitions, payment, pity and history succeed or roll back together. */
    bool PullBatch(const FOGGachaBannerDefinition& Banner, const FOGEntityId& RulerId,
        int64 WorldTick, int64 Seed, int32 Count,
        TArray<FOGGachaPullResult>& OutResults, FString& OutError);

    /** Read-only exact payment/pity quote; no RNG, ownership or persistence changes. */
    bool PreviewBatch(const FOGGachaBannerDefinition&,const FOGEntityId&,int64,int32,
        FOGGachaBatchPreview&,FString&,const FOGContentId& BatchTicketId=FOGContentId()) const;
    /** Validate the reviewed quote in the same root transaction as the acquisition. */
    bool PullBatchReviewed(const FOGGachaBannerDefinition&,const FOGEntityId&,int64,int64,int32,
        const FString&,TArray<FOGGachaPullResult>&,FString&,const FOGContentId& BatchTicketId=FOGContentId());

    static bool ValidateBanner(
        const FOGGachaBannerDefinition& Banner,
        FString& OutError);

    /** Read-only exact D02 additive soft-pity quote in basis points. */
    static int32 CalculateTopRarityChanceBps(
        const FOGGachaBannerDefinition& Banner,
        int32 PullOrdinalSinceTop);

private:
    bool BuildBatchPreview(const FOGGachaBannerDefinition&,const FOGEntityId&,int64,int32,
        FOGGachaBatchPreview&,FString&,const FOGContentId& BatchTicketId) const;
    bool PullBatchInternal(const FOGGachaBannerDefinition&,const FOGEntityId&,int64,int64,int32,
        TArray<FOGGachaPullResult>&,FString&,bool bOwnTransaction,const FOGContentId& BatchTicketId=FOGContentId());
    bool PullInternal(const FOGGachaBannerDefinition& Banner, const FOGEntityId& RulerId,
        int64 WorldTick, int64 Seed, FOGGachaPullResult& OutResult,
        FString& OutError, bool bOwnTransaction,
        bool bUpgradeFinalRToSR=false,const FOGContentId& BatchTicketId=FOGContentId(),
        const FOGEntityId& PaymentBatchId=FOGEntityId(),bool bFirstPrepaidPull=false,bool bPreserveBannerPity=false);
    IOGWorldStore& Store;
};
