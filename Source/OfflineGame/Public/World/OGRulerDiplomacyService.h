#pragma once

#include "CoreMinimal.h"
#include "Core/OGEntityId.h"
#include "World/OGDispatchFactionWarRecords.h"

class IOGWorldStore;

enum class EOGRulerDiplomacyAction : uint8
{
    EndAlliance,
    PledgeSupport,
    WithdrawSupport,
    EndSubordination
};

struct OFFLINEGAME_API FOGRulerDiplomacyFaction
{
    FOGEntityId Id;
    FString Name;
};

struct OFFLINEGAME_API FOGRulerDiplomacyLinks
{
    FOGFactionLinkRecord Alliance,OutgoingSupport,IncomingSupport,Subordination;
};

struct OFFLINEGAME_API FOGRulerDiplomacyReview
{
    FOGEntityId RulerId,OwnFactionId,OtherFactionId;
    EOGRulerDiplomacyAction Action=EOGRulerDiplomacyAction::EndAlliance;
    FString OwnName,OtherName,Token,Reason;
    bool bCanApply=false;
};

class OFFLINEGAME_API FOGRulerDiplomacyService
{
public:
    explicit FOGRulerDiplomacyService(IOGWorldStore& InStore):Store(InStore) {}

    bool ReadFactions(const FOGEntityId& Ruler,TArray<FOGRulerDiplomacyFaction>& Led,
        TArray<FOGRulerDiplomacyFaction>& Known,FString& Error) const;
    bool ReadLinks(const FOGEntityId& Ruler,const FOGEntityId& Own,const FOGEntityId& Other,
        FOGRulerDiplomacyLinks& Out,FString& Error) const;
    bool Review(const FOGEntityId& Ruler,const FOGEntityId& Own,const FOGEntityId& Other,
        EOGRulerDiplomacyAction Action,FOGRulerDiplomacyReview& Out,FString& Error) const;
    bool Confirm(const FOGRulerDiplomacyReview& Reviewed,const FOGEntityId& Request,int64 Tick,FString& Error);
    static FString ActionLabel(EOGRulerDiplomacyAction Action);

private:
    IOGWorldStore& Store;
};
