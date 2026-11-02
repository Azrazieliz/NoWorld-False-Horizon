#include "Combat/OGApprovedLuminousSourceKit.h"

#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Misc/SecureHash.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"

namespace
{
FString SourceFile(const TCHAR* File)
{
    return FPaths::Combine(FPaths::ProjectContentDir(),
        TEXT("NoWorld/Combat/AuthoritativeDesign"),File);
}

bool ReadLockedDocument(const TCHAR* File, const TCHAR* ExpectedMd5,
                        FString& Text, TSharedPtr<FJsonObject>& Json,
                        FString& Error)
{
    TArray<uint8> Bytes;
    if (!FFileHelper::LoadFileToArray(Bytes,*SourceFile(File)) ||
        Bytes.IsEmpty())
    {
        Error=FString::Printf(TEXT("Approved Luminous source missing: %s"),File);
        return false;
    }
    FMD5 Hash;
    Hash.Update(Bytes.GetData(),Bytes.Num());
    uint8 Digest[16];
    Hash.Final(Digest);
    if (!BytesToHex(Digest,16).Equals(ExpectedMd5,
                                     ESearchCase::IgnoreCase))
    {
        Error=TEXT("Approved Luminous source bytes differ from Gate11-verified original.");
        return false;
    }
    FFileHelper::BufferToString(Text,Bytes.GetData(),Bytes.Num());
    if (!FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Text),Json) ||
        !Json.IsValid())
    {
        Error=TEXT("Approved Luminous JSON is unreadable.");
        return false;
    }
    return true;
}

int64 ReadExactInteger(const TSharedPtr<FJsonObject>& Obj,const TCHAR* Name)
{
    return Obj->GetIntegerField(Name);
}
}

bool FOGApprovedLuminousSourceKit::LoadBaseV6(
    FOGApprovedLuminousSourceKitView& Out,FString& OutError)
{
    Out=FOGApprovedLuminousSourceKitView();
    OutError.Reset();
    FString Design,Registry;
    TSharedPtr<FJsonObject> Kit,Ids;
    if (!ReadLockedDocument(TEXT("Luminous_Base_v6_USER_APPROVED.json"),
            TEXT("4e14ea3284afb8061c0727d485ccde1a"),
            Design,Kit,OutError) ||
        !ReadLockedDocument(TEXT("Luminous_Base_v6_DESIGN_IDS.json"),
            TEXT("7c4f3370b3c05f2c46c7977fec303b36"),
            Registry,Ids,OutError))
        return false;

    Out.IdentityId=FOGContentId(
        Ids->GetStringField(TEXT("intended_identity_content_id")));
    Out.VersionId=FOGContentId(
        Ids->GetStringField(TEXT("intended_base_version_content_id")));
    if (!Out.IdentityId.IsValid() || !Out.VersionId.IsValid() ||
        Kit->GetStringField(TEXT("design_version")) != TEXT("2026-10-19.v6"))
    {
        OutError=TEXT("Original Luminous Base source/Registry identity or version mismatch.");
        return false;
    }

    const TSharedPtr<FJsonObject>& Stats=
        Kit->GetObjectField(TEXT("stats"))->GetObjectField(TEXT("FOGCombatStats"));
    auto& S=Out.RankOneHeroStats;
    S.MaxHp=FOGLargeNumber::FromInt64(ReadExactInteger(Stats,TEXT("max_hp")));
    S.Attack=FOGLargeNumber::FromInt64(ReadExactInteger(Stats,TEXT("attack")));
    S.Defense=FOGLargeNumber::FromInt64(ReadExactInteger(Stats,TEXT("defense")));
    S.Speed=static_cast<int32>(ReadExactInteger(Stats,TEXT("speed")));
    S.CritRateBps=static_cast<int32>(ReadExactInteger(Stats,TEXT("crit_rate_bps")));
    S.CritDamageBonusBps=static_cast<int32>(ReadExactInteger(Stats,TEXT("crit_damage_bonus_bps")));
    S.CritRateResistanceBps=static_cast<int32>(ReadExactInteger(Stats,TEXT("crit_rate_resistance_bps")));
    S.CritDamageResistanceBps=static_cast<int32>(ReadExactInteger(Stats,TEXT("crit_damage_resistance_bps")));
    S.HitBps=static_cast<int32>(ReadExactInteger(Stats,TEXT("hit_bps")));
    S.DodgeBps=static_cast<int32>(ReadExactInteger(Stats,TEXT("dodge_bps")));
    S.BlockRateBps=static_cast<int32>(ReadExactInteger(Stats,TEXT("block_rate_bps")));
    S.BlockReductionBps=static_cast<int32>(ReadExactInteger(Stats,TEXT("block_reduction_bps")));
    S.DefensePenetrationBps=static_cast<int32>(ReadExactInteger(Stats,TEXT("defense_penetration_bps")));
    S.DamageDealtMultiplierBps=static_cast<int32>(ReadExactInteger(Stats,TEXT("damage_dealt_multiplier_bps")));
    S.DamageTakenMultiplierBps=static_cast<int32>(ReadExactInteger(Stats,TEXT("damage_taken_multiplier_bps")));
    S.TrueDamageResistanceBps=static_cast<int32>(ReadExactInteger(Stats,TEXT("true_damage_resistance_bps")));
    if (FOGLargeNumber::Compare(S.MaxHp,FOGLargeNumber())<=0 ||
        FOGLargeNumber::Compare(S.Attack,FOGLargeNumber())<=0 ||
        S.Speed<=0)
    {
        OutError=TEXT("Original Base v6 authored stat projection invalid.");
        return false;
    }

    for (const TSharedPtr<FJsonValue>& Value:
         Ids->GetArrayField(TEXT("ability_content_ids")))
    {
        const TSharedPtr<FJsonObject>& Row=Value->AsObject();
        if (!Row.IsValid()) {OutError=TEXT("Malformed Base skill Registry row.");return false;}
        const FOGContentId Id(Row->GetStringField(TEXT("content_id")));
        const FString Slot=Row->GetStringField(TEXT("slot_type"));
        if (!Id.IsValid()) {OutError=TEXT("Invalid approved design skill ID.");return false;}
        if (Slot==TEXT("active"))Out.CoreSkills.ActiveSkills.Add(Id);
        else if (Slot==TEXT("passive"))Out.CoreSkills.PassiveSkills.Add(Id);
        else if (Slot==TEXT("ultimate"))Out.CoreSkills.UltimateSkill=Id;
        else if (Slot==TEXT("fantasm"))Out.WorldFantasmId=Id;
    }
    if (Out.CoreSkills.ActiveSkills.Num()!=3 ||
        Out.CoreSkills.PassiveSkills.Num()!=6 ||
        !Out.CoreSkills.UltimateSkill.IsValid() ||
        !Out.WorldFantasmId.IsValid() ||
        Out.CoreSkills.HasDuplicateSkills())
    {
        OutError=TEXT("Exact Base v6 3-active/6-passive/ultimate/fantasm contract failed.");
        return false;
    }
    const TSharedPtr<FJsonObject>& Resources=Kit->GetObjectField(TEXT("resources"));
    const TSharedPtr<FJsonObject>& Scarlet=
        Resources->GetObjectField(TEXT("scarlet_reserve"));
    const TSharedPtr<FJsonObject>& Charge=
        Resources->GetObjectField(TEXT("ultimate_charge"));
    Out.InitialScarletReserve=static_cast<int32>(
        ReadExactInteger(Scarlet,TEXT("initial")));
    Out.ScarletReserveMax=static_cast<int32>(
        ReadExactInteger(Scarlet,TEXT("max")));
    Out.UltimateChargeCapPercent=static_cast<int32>(
        ReadExactInteger(Charge,TEXT("max_percent")));
    Out.DayUltimateCostPercent=static_cast<int32>(
        ReadExactInteger(Charge,TEXT("day_cost_percent")));
    Out.NightUltimateCostPercent=static_cast<int32>(
        ReadExactInteger(Charge,TEXT("night_cost_percent")));
    if (Out.InitialScarletReserve<0 || Out.ScarletReserveMax!=100 ||
        Out.InitialScarletReserve>100 ||
        Out.UltimateChargeCapPercent!=200 ||
        Out.DayUltimateCostPercent!=100 ||
        Out.NightUltimateCostPercent!=150)
    {
        OutError=TEXT("Base v6 source resource schedule differs from approved contract.");
        return false;
    }
    // The exact design Version is also the resolved Base form. Never
    // produce a form-less combat snapshot that can inherit Alter skills.
    Out.CoreSkills.CurrentForm=Out.VersionId;
    // All per-action mechanics, triggers, restoration/revival, World
    // Negation and target rules stay available as the EXACT original JSON,
    // not summarized into a fictitious generic skill handler.
    Out.AuthoritativeSourceJson=MoveTemp(Design);
    return true;
}
