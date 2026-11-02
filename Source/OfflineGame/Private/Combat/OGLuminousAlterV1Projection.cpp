#include "Combat/OGLuminousAlterV1Projection.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Misc/SecureHash.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"

bool FOGLuminousAlterV1Projection::LoadApprovedV1(
    FOGLuminousAlterV1Projection& Out, FString& OutError)
{
    Out=FOGLuminousAlterV1Projection();
    OutError.Reset();
    const FString Path=FPaths::Combine(FPaths::ProjectContentDir(),
        TEXT("NoWorld/Combat/AuthoritativeDesign"),
        TEXT("Luminous_Alter_Offensive_Kit_v1_APPROVED_DESIGN.json"));
    TArray<uint8> Bytes;
    if(!FFileHelper::LoadFileToArray(Bytes,*Path) || Bytes.IsEmpty())
    {
        OutError=TEXT("Exact approved Gate04 Alter v1 source data unavailable.");
        return false;
    }
    FMD5 Md5;
    Md5.Update(Bytes.GetData(),Bytes.Num());
    uint8 Digest[16];
    Md5.Final(Digest);
    if(!BytesToHex(Digest,16).Equals(
        TEXT("5f372226e7b93b63e7b37bb000b68295"),
        ESearchCase::IgnoreCase))
    {
        OutError=TEXT("Gate04 Alter v1 bytes differ from the user-approved source.");
        return false;
    }
    FString Text;
    FFileHelper::BufferToString(Text,Bytes.GetData(),Bytes.Num());
    TSharedPtr<FJsonObject> J;
    if(!FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Text),J)||
       !J.IsValid() ||
       J->GetStringField(TEXT("schema")) !=
        TEXT("gate04.luminous_valentine.alter.ln23.offensive_risk_v1") ||
       J->GetStringField(TEXT("status")) !=
        TEXT("USER_VALIDATED_GATE04_DESIGN_V1_PENDING_GATE11_GATE12_GATE09"))
    {
        OutError=TEXT("Alter v1 schema or approval scope differs from the frozen design.");
        return false;
    }
    const TSharedPtr<FJsonObject>& Authority=J->GetObjectField(TEXT("authority"));
    const TSharedPtr<FJsonObject>& Stats=J->GetObjectField(TEXT("baseline_hero_lv1"));
    const TSharedPtr<FJsonObject>& Resources=J->GetObjectField(TEXT("resources"));
    const TSharedPtr<FJsonObject>& Tithe=Resources->GetObjectField(TEXT("red_tithe"));
    const TSharedPtr<FJsonObject>& Charge=Resources->GetObjectField(TEXT("ultimate_charge"));

    Out.DraftVersionId=FOGContentId(Authority->GetStringField(TEXT("version_id")));
    // Gate11 no longer exists: this user-approved v1 is installed directly
    // under a unique native Alter Version without changing the locked JSON.
    if(Out.DraftVersionId!=FOGContentId(TEXT("draft:character.luminous_valentine.alter")))
    {
        OutError=TEXT("Approved Alter design carries a different immutable source Version.");
        return false;
    }
    Out.NativeVersionId=FOGContentId(TEXT("character:luminous_valentine.version_alter_v1"));
    auto Read=[&](const TCHAR* Id)->int32
    {
        return static_cast<int32>(Stats->GetIntegerField(Id));
    };
    auto& S=Out.HeroLevelOne;
    S.MaxHp=FOGLargeNumber::FromInt64(Read(TEXT("hp")));
    S.Attack=FOGLargeNumber::FromInt64(Read(TEXT("atk")));
    S.Defense=FOGLargeNumber::FromInt64(Read(TEXT("def")));
    S.Speed=Read(TEXT("spd"));
    S.CritRateBps=Read(TEXT("crit_rate"));
    S.CritDamageBonusBps=Read(TEXT("crit_damage"));
    S.CritRateResistanceBps=Read(TEXT("crit_resist"));
    S.CritDamageResistanceBps=Read(TEXT("crit_damage_resist"));
    S.HitBps=Read(TEXT("hit"));
    S.DodgeBps=Read(TEXT("dodge"));
    S.BlockRateBps=Read(TEXT("block"));
    S.BlockReductionBps=Read(TEXT("block_reduction"));
    S.DefensePenetrationBps=Read(TEXT("pen"));
    S.DamageDealtMultiplierBps=Read(TEXT("damage_dealt"));
    S.DamageTakenMultiplierBps=Read(TEXT("damage_taken"));
    S.TrueDamageResistanceBps=Read(TEXT("true_damage_resist"));

    // Exact approved frozen source still uses draft identifiers; convert
    // their namespace one-to-one to stable native executable content IDs.
    // This is a technical identifier projection, NOT an edited ability design.
    const auto NativeAlterId=[](const FString& Source)->FOGContentId
    {
        const FString Prefix(TEXT("draft:luminous_valentine.alter."));
        if(!Source.StartsWith(Prefix,ESearchCase::CaseSensitive) ||
           Source.Len()<=Prefix.Len())return FOGContentId();
        return FOGContentId(TEXT("combat:luminous_valentine.alter.")+
            Source.RightChop(Prefix.Len()));
    };
    for(const TSharedPtr<FJsonValue>& Entry:J->GetArrayField(TEXT("actives")))
        Out.Skills.ActiveSkills.Add(
            NativeAlterId(Entry->AsObject()->GetStringField(TEXT("id"))));
    for(const TSharedPtr<FJsonValue>& Entry:J->GetArrayField(TEXT("passives")))
        Out.Skills.PassiveSkills.Add(
            NativeAlterId(Entry->AsObject()->GetStringField(TEXT("id"))));
    Out.Skills.UltimateSkill=NativeAlterId(
        J->GetObjectField(TEXT("ultimate"))->GetStringField(TEXT("id")));
    Out.WorldFantasmId=NativeAlterId(
        J->GetObjectField(TEXT("world_fantasm"))->GetStringField(TEXT("id")));
    Out.Skills.CurrentForm=Out.NativeVersionId;
    Out.InitialRedTithe=static_cast<int32>(Tithe->GetIntegerField(TEXT("initial")));
    Out.RedTitheMaximum=static_cast<int32>(Tithe->GetIntegerField(TEXT("maximum")));
    Out.UltimateChargeCapPercent=static_cast<int32>(
        Charge->GetIntegerField(TEXT("maximum_percent")));
    if(!Out.DraftVersionId.IsValid() || !Out.NativeVersionId.IsValid() ||
       Out.Skills.ActiveSkills.Num()!=3 ||
       Out.Skills.PassiveSkills.Num()!=6 ||
       !Out.Skills.UltimateSkill.IsValid() ||
       !Out.WorldFantasmId.IsValid() ||
       Out.Skills.HasDuplicateSkills() ||
       Out.InitialRedTithe!=20 ||
       Out.RedTitheMaximum!=100 ||
       Out.UltimateChargeCapPercent!=200 ||
       FOGLargeNumber::Compare(S.MaxHp,FOGLargeNumber())<=0 ||
       S.Speed<=0)
    {
        OutError=TEXT("Alter v1 exact kit shape, stat projection or resource cap invalid.");
        return false;
    }
    Out.AuthoritativeSourceJson=MoveTemp(Text);
    return true;
}
