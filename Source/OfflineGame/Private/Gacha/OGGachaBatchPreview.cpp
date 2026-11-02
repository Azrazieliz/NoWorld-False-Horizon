#include "Gacha/OGGachaService.h"
#include "Gacha/OGRulerGachaAccessService.h"
#include "Misc/SecureHash.h"
#include "UObject/Class.h"

bool FOGGachaService::PreviewBatch(const FOGGachaBannerDefinition& Banner,const FOGEntityId& Ruler,
    int64 Tick,int32 Count,FOGGachaBatchPreview& Out,FString& Error) const
{
    Out=FOGGachaBatchPreview(); Error.Reset();
    if (Store.IsWriteTransactionActive())
    { Error=TEXT("Review committed acquisition state outside another write transaction."); return false; }
    return BuildBatchPreview(Banner,Ruler,Tick,Count,Out,Error);
}
bool FOGGachaService::BuildBatchPreview(const FOGGachaBannerDefinition& Banner,const FOGEntityId& Ruler,
    int64 Tick,int32 Count,FOGGachaBatchPreview& Out,FString& Error) const
{
    Out=FOGGachaBatchPreview(); Error.Reset();
    if (!Store.IsOpen() || !Ruler.IsValid() || Tick<0 || Count<1 || Count>10 || !ValidateBanner(Banner,Error))
    { if (Error.IsEmpty()) Error=TEXT("A valid acquisition owner, banner, clock and batch size are required."); return false; }
    bool Found=false; FName Kind; FString State; int64 Revision=0;
    if (!Store.TryReadEntity(Ruler,Found,Kind,State,Revision,Error)) return false;
    if (!Found || Kind!=TEXT("ruler"))
    { Error=TEXT("Acquisition review requires the actual canonical Ruler."); return false; }
    Out.RulerId=Ruler; Out.BannerId=Banner.BannerId; Out.Count=Count;
    if (!FOGRulerGachaAccessService(Store).CanUseGacha(Ruler,Tick,Out.bAccessUnlocked,Error)) return false;
    if (!Out.bAccessUnlocked) Out.MissingRequirements.Add(TEXT("Acquisition access is not unlocked."));
    if (Count==10 && !Banner.Entries.ContainsByPredicate([](const auto& E){return E.Weight>0 && E.Rarity==TEXT("SR");}))
        Out.MissingRequirements.Add(TEXT("This banner cannot provide its authored ten-draw SR floor."));
    int32 Remaining=Count;
    for (const auto& Ticket:Banner.CompatibleTicketIds)
    {
        FOGGachaPaymentPreview Line; Line.ResourceId=Ticket; Line.bTicket=true;
        bool Known=false;
        if (!Store.TryReadResourceBalance(Ruler,Ticket,Known,Line.Balance,Error)) return false;
        if (!Known) Line.Balance=0;
        Line.Spend=FMath::Min<int64>(FMath::Max<int64>(0,Line.Balance),Remaining);
        Remaining-=static_cast<int32>(Line.Spend); Out.Payments.Add(Line);
    }
    FOGGachaPaymentPreview Currency; Currency.ResourceId=Banner.CurrencyId;
    bool CurrencyKnown=false;
    if (!Store.TryReadResourceBalance(Ruler,Banner.CurrencyId,CurrencyKnown,Currency.Balance,Error)) return false;
    if (!CurrencyKnown) Currency.Balance=0;
    if (Remaining>0 && Banner.PullCost>MAX_int64/Remaining)
    { Error=TEXT("The authored batch payment exceeds the account amount range."); return false; }
    Currency.Spend=Banner.PullCost*Remaining; Out.Payments.Add(Currency);
    if (Currency.Spend>FMath::Max<int64>(0,Currency.Balance))
        Out.MissingRequirements.Add(TEXT("Insufficient compatible tickets or draw currency."));
    bool PityFound=false;
    if (!Store.TryReadGachaState(Ruler,Banner.PityCategory,PityFound,Out.Pity,Error)) return false;
    if (!PityFound) { Out.Pity.RulerId=Ruler; Out.Pity.PityCategory=Banner.PityCategory; }
    if (Out.Pity.RulerId!=Ruler || Out.Pity.PityCategory!=Banner.PityCategory ||
        Out.Pity.PullsSinceTopRarity<0 || Out.Pity.PullsSinceTopRarity==MAX_int32 || Out.Pity.TotalPulls<0 ||
        Out.Pity.TotalPulls>MAX_int64-Count)
    { Error=TEXT("The saved acquisition counter cannot be safely reviewed; preserve the existing record."); return false; }
    Out.NextTopRarityChanceBps=CalculateTopRarityChanceBps(Banner,Out.Pity.PullsSinceTopRarity+1);
    Out.bCanPull=Out.MissingRequirements.IsEmpty();
    FString BannerText;
    FOGGachaBannerDefinition::StaticStruct()->ExportText(BannerText,&Banner,nullptr,nullptr,PPF_None,nullptr);
    TArray<FString> Parts={BannerText,Ruler.ToString(),State,LexToString(Revision),LexToString(Count),
        LexToString(Out.bAccessUnlocked),LexToString(PityFound),LexToString(Out.Pity.PullsSinceTopRarity),
        LexToString(Out.Pity.bFeaturedGuarantee),LexToString(Out.Pity.TotalPulls),LexToString(Out.Pity.UpdatedWorldTick)};
    for (const auto& Line:Out.Payments) { Parts.Add(LexToString(Line.Balance)); Parts.Add(LexToString(Line.Spend)); }
    FString Source;
    for (const auto& Part:Parts) Source+=LexToString(Part.Len())+TEXT(":")+Part;
    FTCHARToUTF8 Bytes(*Source); uint8 Digest[20];
    FSHA1::HashBuffer(Bytes.Get(),Bytes.Length(),Digest);
    if (Out.bCanPull) Out.ConfirmationToken=BytesToHex(Digest,UE_ARRAY_COUNT(Digest));
    return true;
}
bool FOGGachaService::PullBatchReviewed(const FOGGachaBannerDefinition& Banner,const FOGEntityId& Ruler,
    int64 Tick,int64 Seed,int32 Count,const FString& Token,TArray<FOGGachaPullResult>& Out,FString& Error)
{
    Out.Reset(); Error.Reset();
    if (Token.IsEmpty()) { Error=TEXT("Review the current acquisition payment before confirming."); return false; }
    if (Store.IsWriteTransactionActive())
    { Error=TEXT("Reviewed acquisition must own its complete root transaction."); return false; }
    if (!Store.BeginTransaction(Error)) return false;
    auto Fail=[&]()
    {
        FString Rollback; Store.RollbackTransaction(Rollback); Out.Reset();
        if (!Rollback.IsEmpty()) Error+=TEXT(" | Rollback: ")+Rollback;
        return false;
    };
    FOGGachaBatchPreview Current;
    if (!BuildBatchPreview(Banner,Ruler,Tick,Count,Current,Error)) return Fail();
    if (!Current.bCanPull || Current.ConfirmationToken!=Token)
    { Error=TEXT("Acquisition payment, pity, access or banner details changed. Review the current offer again."); return Fail(); }
    if (!PullBatchInternal(Banner,Ruler,Tick,Seed,Count,Out,Error,false)) return Fail();
    if (!Store.CommitTransaction(Error)) return Fail();
    return true;
}
