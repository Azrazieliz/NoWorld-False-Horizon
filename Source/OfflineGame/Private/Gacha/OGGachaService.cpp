#include "Gacha/OGGachaService.h"

#include "Events/OGWorldEvent.h"
#include "Gacha/OGRulerGachaAccessService.h"
#include "Random/OGDeterministicRng.h"

namespace
{
bool SelectWeightedEntry(
    const FOGGachaBannerDefinition& Banner,
    FOGDeterministicRng& Rng,
    int32 PullOrdinalSinceTop,
    bool bTopOnly,
    bool bFeaturedTopOnly,
    bool bUnfeaturedTopOnly,
    bool bSrOnly,
    int32& OutIndex,
    FString& OutError)
{
    TArray<int64> EffectiveWeights;
    EffectiveWeights.Init(0, Banner.Entries.Num());

    // The reviewed soft-pity rule adds percentage POINTS to top-rarity
    // probability, not a multiplier to entry weights.
    const bool bAdditivePity = !bTopOnly && !bFeaturedTopOnly &&
        !bUnfeaturedTopOnly && !bSrOnly && Banner.SoftPityStart > 0 &&
        PullOrdinalSinceTop > Banner.SoftPityStart &&
        Banner.SoftPityBonusPerPullBps > 0;
    bool bSoftTop = false;
    if (bAdditivePity)
    {
        const int32 EffectiveTopBps =
            FOGGachaService::CalculateTopRarityChanceBps(Banner, PullOrdinalSinceTop);
        bSoftTop = Rng.NextRange(0, 10000) < EffectiveTopBps;
    }
    int64 TotalWeight = 0;
    for (int32 Index = 0; Index < Banner.Entries.Num(); ++Index)
    {
        const FOGGachaPoolEntry& Entry = Banner.Entries[Index];
        const bool bTop = Entry.Rarity == Banner.TopRarity;

        if ((bTopOnly && !bTop) ||
            (bFeaturedTopOnly && (!bTop || !Entry.bFeatured)) ||
            (bUnfeaturedTopOnly && (!bTop || Entry.bFeatured)) ||
            (bSrOnly && Entry.Rarity != FName(TEXT("SR"))) ||
            (bAdditivePity && bTop != bSoftTop))
        {
            continue;
        }

        const int64 Weight = Entry.Weight;
        if (Weight <= 0)
        {
            continue;
        }

        if (TotalWeight > static_cast<int64>(MAX_int32) - Weight)
        {
            OutError = TEXT("Effective gacha weight exceeds supported deterministic range.");
            return false;
        }

        EffectiveWeights[Index] = Weight;
        TotalWeight += Weight;
    }

    if (TotalWeight <= 0)
    {
        OutError = TEXT("No eligible gacha entry remains after pity/guarantee filtering.");
        return false;
    }

    const int32 Roll =
        Rng.NextRange(0, static_cast<int32>(TotalWeight));

    int64 Cursor = 0;
    for (int32 Index = 0; Index < EffectiveWeights.Num(); ++Index)
    {
        Cursor += EffectiveWeights[Index];
        if (Roll < Cursor)
        {
            OutIndex = Index;
            return true;
        }
    }

    OutError = TEXT("Gacha weighted selection failed unexpectedly.");
    return false;
}
}

int32 FOGGachaService::CalculateTopRarityChanceBps(
    const FOGGachaBannerDefinition& Banner, int32 PullOrdinalSinceTop)
{
    if (Banner.HardPity > 0 && PullOrdinalSinceTop >= Banner.HardPity)
        return 10000;
    int64 All = 0;
    int64 Top = 0;
    for (const FOGGachaPoolEntry& Entry : Banner.Entries)
    {
        if (Entry.Weight <= 0) continue;
        All += Entry.Weight;
        if (Entry.Rarity == Banner.TopRarity) Top += Entry.Weight;
    }
    if (All <= 0) return 0;
    const int64 BaseBps = (Top * 10000 + All / 2) / All;
    const int64 Bonus = PullOrdinalSinceTop > Banner.SoftPityStart &&
        Banner.SoftPityStart > 0
        ? static_cast<int64>(PullOrdinalSinceTop - Banner.SoftPityStart) *
            static_cast<int64>(Banner.SoftPityBonusPerPullBps)
        : 0;
    return static_cast<int32>(FMath::Clamp<int64>(BaseBps + Bonus, 0, 10000));
}

bool FOGGachaService::ValidateBanner(
    const FOGGachaBannerDefinition& Banner,
    FString& OutError)
{
    OutError.Reset();

    if (!Banner.BannerId.IsValid() ||
        Banner.PityCategory.IsNone() ||
        !Banner.CurrencyId.IsValid() ||
        Banner.PullCost < 0 ||
        Banner.TopRarity.IsNone() ||
        Banner.HardPity <= 0 ||
        Banner.SoftPityStart < 0 ||
        Banner.SoftPityStart >= Banner.HardPity ||
        Banner.SoftPityBonusPerPullBps < 0 ||
        Banner.Entries.IsEmpty())
    {
        OutError = TEXT("Gacha banner has invalid identity, cost, pity, or pool configuration.");
        return false;
    }

    TSet<FOGContentId> SeenTickets;
    for (const FOGContentId& TicketId :
         Banner.CompatibleTicketIds)
    {
        if (!TicketId.IsValid() ||
            TicketId == Banner.CurrencyId ||
            SeenTickets.Contains(TicketId))
        {
            OutError =
                TEXT("Gacha compatible ticket IDs must be valid, unique, and distinct from pull currency.");
            return false;
        }
        SeenTickets.Add(TicketId);
    }
    for (const auto& Ticket:Banner.CompatibleMultiPullTickets)
    {
        if (!Ticket.ResourceId.IsValid() || Ticket.ResourceId==Banner.CurrencyId ||
            SeenTickets.Contains(Ticket.ResourceId) || Ticket.DisplayName.IsEmpty() || Ticket.PullCount<2 || Ticket.PullCount>10)
        { OutError=TEXT("Multi-pull tickets need distinct authored resources and an exact draw count from two to ten."); return false; }
        SeenTickets.Add(Ticket.ResourceId);
    }

    for (const auto& Ticket:Banner.CompatibleGuaranteedTickets)
    {
        if (!Ticket.ResourceId.IsValid() || Ticket.ResourceId==Banner.CurrencyId ||
            SeenTickets.Contains(Ticket.ResourceId) || Ticket.DisplayName.IsEmpty() || Ticket.GuaranteedRarity.IsNone() || Ticket.Entries.IsEmpty())
        { OutError=TEXT("Guaranteed tickets require distinct resources and an original rarity-specific pool."); return false; }
        SeenTickets.Add(Ticket.ResourceId);
        int64 Weight=0;
        TSet<FOGContentId> Versions;
        for (const auto& Entry:Ticket.Entries)
        {
            if (!Entry.IdentityId.IsValid() || !Entry.VersionId.IsValid() || Versions.Contains(Entry.VersionId) ||
                Entry.Rarity!=Ticket.GuaranteedRarity || Entry.Weight<=0 || Weight>MAX_int32-Entry.Weight)
            { OutError=TEXT("Guaranteed pool entries must be distinct, eligible for the exact rarity, and have valid weights."); return false; }
            Weight+=Entry.Weight;
            Versions.Add(Entry.VersionId);
        }
    }

    bool bHasTop = false;
    bool bHasFeaturedTop = false;
    bool bHasUnfeaturedTop = false;
    int64 BaseWeightTotal = 0;

    for (const FOGGachaPoolEntry& Entry : Banner.Entries)
    {
        if (!Entry.IdentityId.IsValid() ||
            !Entry.VersionId.IsValid() ||
            Entry.Rarity.IsNone() ||
            Entry.Weight <= 0)
        {
            OutError = TEXT("Gacha pool entry has invalid content IDs, rarity, or weight.");
            return false;
        }

        BaseWeightTotal += Entry.Weight;
        if (BaseWeightTotal > MAX_int32)
        {
            OutError = TEXT("Gacha base weight total exceeds supported deterministic range.");
            return false;
        }

        if (Entry.Rarity == Banner.TopRarity)
        {
            bHasTop = true;
            bHasFeaturedTop |= Entry.bFeatured;
            bHasUnfeaturedTop |= !Entry.bFeatured;
        }
    }

    if (!bHasTop)
    {
        OutError = TEXT("Gacha banner has no entry matching TopRarity.");
        return false;
    }

    if (Banner.bFeaturedGuaranteeAfterMiss &&
        (!bHasFeaturedTop || !bHasUnfeaturedTop))
    {
        OutError = TEXT("True featured 50/50 requires BOTH featured and standard top-rarity entries.");
        return false;
    }

    return true;
}

bool FOGGachaService::PullBatch(const FOGGachaBannerDefinition& Banner,
    const FOGEntityId& RulerId, int64 WorldTick, int64 Seed, int32 Count,
    TArray<FOGGachaPullResult>& OutResults, FString& OutError)
{ return PullBatchInternal(Banner,RulerId,WorldTick,Seed,Count,OutResults,OutError,true); }

bool FOGGachaService::PullBatchInternal(const FOGGachaBannerDefinition& Banner,
    const FOGEntityId& RulerId,int64 WorldTick,int64 Seed,int32 Count,
    TArray<FOGGachaPullResult>& OutResults,FString& OutError,bool bOwnTransaction,const FOGContentId& BatchTicketId)
{
    OutResults.Reset();
    OutError.Reset();
    if (Count < 1 || Count > 10)
    {
        OutError = TEXT("A batch requires between one and ten pulls.");
        return false;
    }
    if (!ValidateBanner(Banner, OutError) || !RulerId.IsValid())
    {
        if (OutError.IsEmpty()) OutError = TEXT("Gacha requires a valid owning Ruler ID.");
        return false;
    }
    // Gate07 D03 exact approved A: if all ten initial results would be R,
    // replace the LAST with an actual eligible SR. Never manufacture SR
    // content; fail before transaction if a banner cannot honor that floor.
    if (Count == 10 &&
        !Banner.Entries.ContainsByPredicate([](const FOGGachaPoolEntry& Entry)
        {
            return Entry.Weight > 0 && Entry.Rarity == FName(TEXT("SR"));
        }))
    {
        OutError = TEXT("Ten-pull requires an eligible authored SR pool entry.");
        return false;
    }
    if (bOwnTransaction && !Store.BeginTransaction(OutError)) return false;
    auto Fail = [this, &OutResults, &OutError,bOwnTransaction]()
    {
        OutResults.Reset();
        FString RollbackError;
        if (bOwnTransaction) Store.RollbackTransaction(RollbackError);
        if (!RollbackError.IsEmpty()) OutError += TEXT(" | Rollback error: ") + RollbackError;
        return false;
    };
    FOGEntityId PaymentBatchId;
    TOptional<FOGGachaBannerDefinition> GuaranteedPool;
    if (BatchTicketId.IsValid())
    {
        const auto* Ticket=Banner.CompatibleMultiPullTickets.FindByPredicate(
            [&](const auto& Row){return Row.ResourceId==BatchTicketId;});
        const auto* Guaranteed=Banner.CompatibleGuaranteedTickets.FindByPredicate(
            [&](const auto& Row){return Row.ResourceId==BatchTicketId;});
        bool Found=false; int64 Balance=0;
        if (!(Ticket && Ticket->PullCount==Count) && !(Guaranteed && Count==1))
        { OutError=TEXT("The selected ticket cannot pay this batch."); return Fail(); }
        if (Guaranteed)
        {
            GuaranteedPool.Emplace(Banner);
            auto& Draw=GuaranteedPool.GetValue();
            Draw.Entries=Guaranteed->Entries;
            Draw.TopRarity=Guaranteed->GuaranteedRarity;
            Draw.SoftPityStart=0; Draw.SoftPityBonusPerPullBps=0;
            Draw.bFeaturedGuaranteeAfterMiss=false;
            Draw.CompatibleMultiPullTickets.Reset();
            Draw.CompatibleGuaranteedTickets.Reset();
        }
        if (!Store.TryReadResourceBalance(RulerId,BatchTicketId,Found,Balance,OutError)) return Fail();
        if (!Found || Balance<1)
        { OutError=TEXT("The selected multi-pull ticket is unavailable."); return Fail(); }
        if (!Store.SetResourceBalance(RulerId,BatchTicketId,Balance-1,OutError)) return Fail();
        PaymentBatchId=FOGEntityId::NewId();
    }
    const auto& Draw=GuaranteedPool.IsSet()?GuaranteedPool.GetValue():Banner;
    int64 NextSeed = Seed;
    bool bAlreadyRolledSrPlus = false;
    for (int32 Index = 0; Index < Count; ++Index)
    {
        FOGGachaPullResult Result;
        // First nine R allow a potential floor on the final slot, but the
        // final slot is still rolled normally before any conditional upgrade.
        const bool bApplyFloor = Count == 10 && Index == 9 &&
            !bAlreadyRolledSrPlus;
        if (!PullInternal(Draw, RulerId, WorldTick, NextSeed, Result,
                OutError, false, bApplyFloor, BatchTicketId, PaymentBatchId, Index==0, GuaranteedPool.IsSet()))
            return Fail();
        if (Result.Rarity != FName(TEXT("R"))) bAlreadyRolledSrPlus = true;
        NextSeed = static_cast<int64>(static_cast<uint64>(NextSeed) +
            static_cast<uint64>(FMath::Max<int64>(1, Result.RngDrawCount + 1)));
        OutResults.Add(MoveTemp(Result));
    }
    if (bOwnTransaction && !Store.CommitTransaction(OutError)) return Fail();
    return true;
}

bool FOGGachaService::Pull(const FOGGachaBannerDefinition& Banner,
    const FOGEntityId& RulerId, int64 WorldTick, int64 Seed,
    FOGGachaPullResult& OutResult, FString& OutError)
{
    return PullInternal(Banner, RulerId, WorldTick, Seed, OutResult, OutError, true);
}

bool FOGGachaService::PullInternal(
    const FOGGachaBannerDefinition& Banner,
    const FOGEntityId& RulerId,
    int64 WorldTick,
    int64 Seed,
    FOGGachaPullResult& OutResult,
    FString& OutError,
    bool bOwnTransaction,
    bool bUpgradeFinalRToSR,const FOGContentId& BatchTicketId,const FOGEntityId& PaymentBatchId,
    bool bFirstPrepaidPull,bool bPreserveBannerPity)
{
    OutResult = FOGGachaPullResult();
    OutError.Reset();

    if (!ValidateBanner(Banner, OutError))
    {
        return false;
    }

    if (!RulerId.IsValid())
    {
        OutError = TEXT("Gacha pull requires a valid owning Ruler ID.");
        return false;
    }

    if (bOwnTransaction && !Store.BeginTransaction(OutError))
    {
        return false;
    }

    auto Fail = [this, &OutError, bOwnTransaction](const FString& Error)
    {
        OutError = Error;
        if (bOwnTransaction)
        {
            FString RollbackError;
            Store.RollbackTransaction(RollbackError);
            if (!RollbackError.IsEmpty())
                OutError += FString::Printf(TEXT(" | Rollback error: %s"), *RollbackError);
        }
        return false;
    };

    bool bRulerFound = false;
    FName RulerKind = NAME_None;
    FString RulerStateJson;
    int64 RulerRevision = 0;
    FString Error;

    if (!Store.TryReadEntity(
            RulerId,
            bRulerFound,
            RulerKind,
            RulerStateJson,
            RulerRevision,
            Error))
    {
        return Fail(Error);
    }

    if (!bRulerFound)
    {
        return Fail(TEXT("Gacha pull owner does not exist in authoritative state."));
    }

    FOGRulerGachaAccessService AccessService(Store);
    bool bCanUseGacha = false;
    if (!AccessService.CanUseGacha(
            RulerId,
            WorldTick,
            bCanUseGacha,
            Error))
    {
        return Fail(Error);
    }

    if (!bCanUseGacha)
    {
        return Fail(
            TEXT("Gacha is not permanently unlocked for this Ruler."));
    }

    const bool bPrepaid=BatchTicketId.IsValid() && PaymentBatchId.IsValid();
    FOGContentId PaymentResourceId = bPrepaid?BatchTicketId:Banner.CurrencyId;
    bool bUsedTicket = bPrepaid;
    bool bPaymentKnown = false;
    int64 PaymentBalance = 0;

    // Ordinary compatible tickets are additional access instruments, not a
    // second required grind. Consume one automatically before pull currency.
    if (!bPrepaid)
    {
        for (const FOGContentId& TicketId :
             Banner.CompatibleTicketIds)
        {
            bool bTicketKnown = false;
            int64 TicketBalance = 0;
            if (!Store.TryReadResourceBalance(
                    RulerId,
                    TicketId,
                    bTicketKnown,
                    TicketBalance,
                    Error))
            {
                return Fail(Error);
            }

            if (bTicketKnown &&
                TicketBalance > 0)
            {
                PaymentResourceId =
                    TicketId;
                PaymentBalance =
                    TicketBalance;
                bPaymentKnown =
                    true;
                bUsedTicket =
                    true;
                break;
            }
        }
    }

    if (!bUsedTicket &&
        Banner.PullCost > 0)
    {
        if (!Store.TryReadResourceBalance(
                RulerId,
                Banner.CurrencyId,
                bPaymentKnown,
                PaymentBalance,
                Error))
        {
            return Fail(Error);
        }

        if (!bPaymentKnown ||
            PaymentBalance <
                Banner.PullCost)
        {
            return Fail(
                TEXT("Insufficient compatible ticket or gacha currency."));
        }
    }

    bool bStateFound = false;
    FOGGachaStateRecord State;
    if (!Store.TryReadGachaState(
            RulerId,
            Banner.PityCategory,
            bStateFound,
            State,
            Error))
    {
        return Fail(Error);
    }

    if (!bStateFound)
    {
        State.RulerId = RulerId;
        State.PityCategory = Banner.PityCategory;
    }

    const int32 PullOrdinalSinceTop =
        bPreserveBannerPity?1:State.PullsSinceTopRarity+1;
    const bool bHardPity =
        PullOrdinalSinceTop >= Banner.HardPity;

    FOGDeterministicRng Rng(static_cast<uint64>(Seed));
    int32 SelectedIndex = INDEX_NONE;
    if (!SelectWeightedEntry(
            Banner,
            Rng,
            PullOrdinalSinceTop,
            bHardPity,
            false,
            false,
            false,
            SelectedIndex,
            Error))
    {
        return Fail(Error);
    }

    // D03 applies only when the final naturally rolled result is ALSO R,
    // not when the tenth result is already an SR, SSR, UR, LR or MR.
    if (bUpgradeFinalRToSR &&
        Banner.Entries[SelectedIndex].Rarity == FName(TEXT("R")))
    {
        if (!SelectWeightedEntry(Banner, Rng, PullOrdinalSinceTop,
                false, false, false, true, SelectedIndex, Error))
            return Fail(Error);
    }

    const FOGGachaPoolEntry* Selected =
        &Banner.Entries[SelectedIndex];

    bool bTopRarity =
        Selected->Rarity == Banner.TopRarity;

    if (bTopRarity && Banner.bFeaturedGuaranteeAfterMiss)
    {
        // Independent exactly balanced selection AFTER a top-rarity draw.
        const bool bUseFeatured = State.bFeaturedGuarantee ||
            Rng.NextRange(0, 2) == 0;
        if (!SelectWeightedEntry(
                Banner, Rng, PullOrdinalSinceTop,
                true, bUseFeatured, !bUseFeatured,
                false,
                SelectedIndex, Error))
            return Fail(Error);
        Selected = &Banner.Entries[SelectedIndex];
        bTopRarity = true;
    }

    TArray<FOGCharacterManifestationRecord> ExistingManifestations;
    if (!Store.ListCharacterManifestationsByOwnerAndIdentity(
            RulerId,
            Selected->IdentityId,
            ExistingManifestations,
            Error))
    {
        return Fail(Error);
    }

    const bool bDuplicate =
        !ExistingManifestations.IsEmpty();

    int32 AcquisitionOrdinal = 0;
    for (const FOGCharacterManifestationRecord& Existing :
         ExistingManifestations)
    {
        AcquisitionOrdinal =
            FMath::Max(
                AcquisitionOrdinal,
                Existing.AcquisitionOrdinal + 1);
    }

    FOGWorldEvent Event;
    Event.EventId = FOGEntityId::NewId();
    Event.EventType = TEXT("gacha_pull");
    Event.WorldTick = WorldTick;
    Event.PrimaryEntity = RulerId;
    Event.bChronicleEligible = false;

    FOGCharacterManifestationRecord Manifestation;
    Manifestation.ManifestationId = FOGEntityId::NewId();
    Manifestation.OwningRulerId = RulerId;
    Manifestation.IdentityId = Selected->IdentityId;
    Manifestation.ActiveVersionId = Selected->VersionId;
    Manifestation.Level = 1;
    Manifestation.CurrentRarity = Selected->Rarity;
    Manifestation.AcquisitionWorldTick = WorldTick;
    Manifestation.AcquisitionOrdinal = AcquisitionOrdinal;
    Manifestation.OriginPullEventId = Event.EventId;
    Manifestation.LifecycleState = FName(TEXT("active"));

    if (!Store.UpsertCharacterManifestation(
            Manifestation,
            WorldTick,
            Error))
    {
        return Fail(Error);
    }

    FOGManifestationReinforcementRecord Reinforcement;
    Reinforcement.ManifestationId =
        Manifestation.ManifestationId;
    Reinforcement.ReinforcementState =
        FName(TEXT("developing"));
    Reinforcement.bMaxReinforced =
        false;
    Reinforcement.UpdatedWorldTick =
        WorldTick;
    Reinforcement.StateJson =
        TEXT("{\"source\":\"gacha_acquisition\"}");

    if (!Store.UpsertManifestationReinforcement(
            Reinforcement,
            Error))
    {
        return Fail(Error);
    }

    // Acquisition is presentation-visible immediately but never performs the
    // first physical World Mode anchoring step. That explicit action is owned by
    // FOGTerritoryControlService once the Ruler is in effective Territory.

    if (bUsedTicket && !bPrepaid)
    {
        if (!Store.SetResourceBalance(
                RulerId,
                PaymentResourceId,
                PaymentBalance - 1,
                Error))
        {
            return Fail(Error);
        }
    }
    else if (!bPrepaid && Banner.PullCost > 0 &&
             !Store.SetResourceBalance(
                 RulerId,
                 Banner.CurrencyId,
                 PaymentBalance -
                     Banner.PullCost,
                 Error))
    {
        return Fail(Error);
    }

    if (!bPreserveBannerPity)
    {
        ++State.TotalPulls;
        State.UpdatedWorldTick = WorldTick;

        if (bTopRarity)
        {
            State.PullsSinceTopRarity = 0;
            State.bFeaturedGuarantee =
                Banner.bFeaturedGuaranteeAfterMiss &&
                !Selected->bFeatured;
        }
        else
        {
            State.PullsSinceTopRarity = PullOrdinalSinceTop;
        }

        if (!Store.UpsertGachaState(State, Error))
        {
            return Fail(Error);
        }
    }

    const int64 PaymentAmount=bPrepaid?(bFirstPrepaidPull?1:0):(bUsedTicket?1:Banner.PullCost);
    Event.RelatedEntities.Add(Manifestation.ManifestationId);
    Event.PayloadJson = FString::Printf(
        TEXT("{\"banner\":\"%s\",\"identity\":\"%s\",\"version\":\"%s\",")
        TEXT("\"rarity\":\"%s\",\"featured\":%s,\"duplicate\":%s,")
        TEXT("\"payment_resource\":\"%s\",\"used_ticket\":%s,")
        TEXT("\"payment_amount_exact\":\"%lld\",\"payment_batch_id\":\"%s\",\"advances_banner_pity\":%s,")
        TEXT("\"seed\":%lld,\"rng_draws\":%llu}"),
        *Banner.BannerId.ToString(),
        *Selected->IdentityId.ToString(),
        *Selected->VersionId.ToString(),
        *Selected->Rarity.ToString(),
        Selected->bFeatured ? TEXT("true") : TEXT("false"),
        bDuplicate ? TEXT("true") : TEXT("false"),
        *PaymentResourceId.ToString(),
        bUsedTicket ? TEXT("true") : TEXT("false"),
        PaymentAmount,
        *(PaymentBatchId.IsValid()?PaymentBatchId.ToString():FString()),
        bPreserveBannerPity?TEXT("false"):TEXT("true"),
        Seed,
        static_cast<unsigned long long>(Rng.GetDrawCount()));

    if (!Store.AppendWorldEvent(Event, Error))
    {
        return Fail(Error);
    }

    if (bOwnTransaction && !Store.CommitTransaction(OutError))
    {
        FString RollbackError;
        Store.RollbackTransaction(RollbackError);
        return false;
    }

    OutResult.BannerId = Banner.BannerId;
    OutResult.IdentityId = Selected->IdentityId;
    OutResult.VersionId = Selected->VersionId;
    OutResult.Rarity = Selected->Rarity;
    OutResult.ManifestationId = Manifestation.ManifestationId;
    OutResult.bTopRarity = bTopRarity;
    OutResult.bFeatured = Selected->bFeatured;
    OutResult.bDuplicateIdentity = bDuplicate;
    OutResult.PaymentResourceId =
        PaymentResourceId;
    OutResult.bUsedTicket =
        bUsedTicket;
    OutResult.PaymentAmount=PaymentAmount;
    OutResult.PaymentBatchId=PaymentBatchId;
    OutResult.bAdvancedBannerPity=!bPreserveBannerPity;
    OutResult.Seed = Seed;
    OutResult.RngDrawCount =
        static_cast<int64>(Rng.GetDrawCount());
    OutResult.UpdatedState = State;

    return true;
}
