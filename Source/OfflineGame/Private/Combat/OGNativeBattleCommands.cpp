#include "Combat/OGNativeBattleCommands.h"
#include "Characters/OGCharacterGameplayDefinition.h"
#include "Combat/OGNativeCombatAbility.h"
#include "Combat/OGApprovedLuminousSourceKit.h"
#include "Combat/OGLuminousAlterV1Projection.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"

namespace
{
bool ReadNames(const FString& Text, bool Base, TArray<FString>& Actives, FString& Ultimate, FString& Field, FString& Error)
{
    TSharedPtr<FJsonObject> Root;
    if (!FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Text),Root) || !Root)
    { Error=TEXT("Approved combat labels are unavailable."); return false; }
    const TArray<TSharedPtr<FJsonValue>>* Rows=nullptr;
    if (!Root->TryGetArrayField(Base?TEXT("core_skills"):TEXT("actives"),Rows)) return false;
    for (const auto& Value:*Rows)
    {
        const auto Row=Value.IsValid()?Value->AsObject():nullptr; FString Name,Slot;
        if (!Row || !Row->TryGetStringField(TEXT("name"),Name) || Name.IsEmpty()) return false;
        if (Base && (!Row->TryGetStringField(TEXT("slot_type"),Slot) || Slot!=TEXT("active"))) continue;
        Actives.Add(Name);
    }
    const TSharedPtr<FJsonObject>* U=nullptr; const TSharedPtr<FJsonObject>* F=nullptr;
    if (Actives.Num()!=3 || !Root->TryGetObjectField(TEXT("ultimate"),U) ||
        !Root->TryGetObjectField(TEXT("world_fantasm"),F) ||
        !(*U)->TryGetStringField(TEXT("name"),Ultimate) ||
        !(*F)->TryGetStringField(TEXT("name"),Field)) return false;
    return true;
}
bool SameKit(const FOGResolvedSkillSet& A,const FOGResolvedSkillSet& B)
{
    if (A.CurrentForm!=B.CurrentForm || A.ActiveSkills.Num()!=B.ActiveSkills.Num() ||
        A.PassiveSkills.Num()!=B.PassiveSkills.Num() || !A.HasSameUltimates(B) ||
        A.HasDuplicateSkills()) return false;
    for (const auto& Id:B.ActiveSkills) if (!A.ActiveSkills.Contains(Id)) return false;
    for (const auto& Id:B.PassiveSkills) if (!A.PassiveSkills.Contains(Id)) return false;
    return true;
}
}
void FOGNativeBattleCommand::BuildAction(const FOGCombatUnitState& Source,
    const TArray<FOGEntityId>& Selected,FOGResolvedCombatAction& Out) const
{
    Out=FOGResolvedCombatAction();
    Out.SourceUnitId=Source.UnitEntityId; Out.SkillId=SkillId;
    Out.ActionDelay=FMath::Max<int64>(1,Source.DefaultActionDelay);
    Out.TargetUnitIds=bSelfTarget?TArray<FOGEntityId>{Source.UnitEntityId}:Selected;
    auto Context=MakeShared<FJsonObject>();
    if (!Branch.IsNone()) Context->SetStringField(TEXT("branch"),Branch.ToString().ToLower());
    if (!Mode.IsNone()) Context->SetStringField(TEXT("mode"),Mode.ToString().ToLower());
    Out.ResolutionJson.Reset();
    FJsonSerializer::Serialize(Context,TJsonWriterFactory<>::Create(&Out.ResolutionJson));
}
FName FOGNativeBattleCommand::MotionSemantic() const
{
    if (!AuthoredMotion.IsNone()) return AuthoredMotion;
    FString Key=bFantasm?TEXT("Combat.Fantasm."):ActiveSlot==INDEX_NONE?TEXT("Combat.Ultimate."):TEXT("Combat.Skill.");
    Key+=SkillId.ToString();
    if (!Branch.IsNone()) Key+=TEXT(".")+Branch.ToString().ToLower();
    if (!Mode.IsNone()) Key+=TEXT(".")+Mode.ToString().ToLower();
    return FName(*Key);
}
bool FOGNativeBattleCommands::BuildFor(const FOGCombatUnitState& Source,FOGNativeBattleCommandSet& Out,FString& Error)
{
    Out=FOGNativeBattleCommandSet(); Error.Reset();
    if (Source.IdentityId!=FOGContentId(TEXT("character:luminous_valentine.identity")))
    {
        const auto* Definition=UOGCharacterGameplayDefinition::Find(Source.IdentityId,Source.SkillSet.CurrentForm,Error);
        if (!Definition || !Definition->MatchesKit(Source.SkillSet))
        { if (Error.IsEmpty()) Error=TEXT("The current character does not carry its exact registered kit."); return false; }
        auto* Library=UOGDefinitionLibrary::Get();
        Out.ApprovedSkills=Definition->Skills;
        TArray<FOGContentId> Skills=Source.SkillSet.ActiveSkills;
        Skills.Append(Source.SkillSet.GetUltimateSkills());
        for (int32 Index=0;Index<Skills.Num();++Index)
        {
            const auto* Ability=Cast<UOGNativeCombatAbility>(Library->Resolve(TEXT("ability"),Skills[Index]));
            if (!Ability || !Ability->ValidateInstalled(Error) ||
                !Ability->Definition.MatchesSource(Source.IdentityId,Source.SkillSet.CurrentForm))
            { if (Error.IsEmpty()) Error=TEXT("A kit command has no compatible registered skill definition."); Out=FOGNativeBattleCommandSet(); return false; }
            FOGNativeBattleCommand C; C.Id=FName(*Skills[Index].ToString()); C.SkillId=Skills[Index];
            C.Label=Ability->DisplayName.IsEmpty()?Skills[Index].ToString():Ability->DisplayName.ToString();
            C.Hint=Ability->Description.ToString(); C.AuthoredMotion=Ability->Definition.MotionSemantic;
            C.bSelfTarget=Ability->Definition.Targets==EOGNativeAbilityTargets::Self;
            C.ActiveSlot=Index<Source.SkillSet.ActiveSkills.Num()?Index:INDEX_NONE;
            Out.Commands.Add(MoveTemp(C));
        }
        return true;
    }
    const FString Form=Source.SkillSet.CurrentForm.ToString();
    const bool Base=Form==TEXT("character:luminous_valentine.version_ln_base");
    const bool Alter=Form==TEXT("character:luminous_valentine.version_alter_v1");
    if (!Base && !Alter)
    { Error=TEXT("This form's exact native combat executor is not yet admitted."); return false; }
    static TMap<FOGContentId,FOGNativeBattleCommandSet> Cached;
    if (!Cached.Contains(Source.SkillSet.CurrentForm))
    {
        FOGNativeBattleCommandSet Set; FOGContentId Field; FString Original;
        if (Base)
        {
            FOGApprovedLuminousSourceKitView Kit;
            if (!FOGApprovedLuminousSourceKit::LoadBaseV6(Kit,Error)) return false;
            Set.ApprovedSkills=Kit.CoreSkills; Field=Kit.WorldFantasmId;
            Set.ResourceLabel=TEXT("Scarlet Reserve"); Set.ResourceCap=Kit.ScarletReserveMax;
            Set.ChargeCap=Kit.UltimateChargeCapPercent; Original=MoveTemp(Kit.AuthoritativeSourceJson);
        }
        else
        {
            FOGLuminousAlterV1Projection Kit;
            if (!FOGLuminousAlterV1Projection::LoadApprovedV1(Kit,Error)) return false;
            Set.ApprovedSkills=Kit.Skills; Field=Kit.WorldFantasmId;
            Set.ResourceLabel=TEXT("Red Tithe"); Set.ResourceCap=Kit.RedTitheMaximum;
            Set.ChargeCap=Kit.UltimateChargeCapPercent; Original=MoveTemp(Kit.AuthoritativeSourceJson);
        }
        TArray<FString> Names; FString Ultimate,Fantasm;
        if (!ReadNames(Original,Base,Names,Ultimate,Fantasm,Error))
        { if (Error.IsEmpty()) Error=TEXT("Exact approved ability names are unavailable."); return false; }
        auto Add=[&Set](FName Id,const FString& Label,const FString& Hint,int32 Slot,
            FOGContentId Skill,FName Branch=NAME_None,FName Mode=NAME_None,bool Field=false,bool Self=false)
        {
            FOGNativeBattleCommand C; C.Id=Id; C.Label=Label; C.Hint=Hint;
            C.ActiveSlot=Slot; C.SkillId=Skill; C.Branch=Branch; C.Mode=Mode;
            C.bFantasm=Field; C.bSelfTarget=Self; Set.Commands.Add(MoveTemp(C));
        };
        for (int32 I=0;I<3;++I)
            Add(FName(*FString::Printf(TEXT("active_%d"),I+1)),Names[I],
                Alter&&I==2?TEXT("Applies to this Manifestation."):TEXT("Select a primary target; field-wide effects expand automatically."),
                I,Set.ApprovedSkills.ActiveSkills[I],NAME_None,NAME_None,false,Alter&&I==2);
        if (Base)
        {
            Add(TEXT("covenant"),Names[2]+TEXT(" / Covenant"),TEXT("Prepare the approved covenant on one living ally."),
                2,Set.ApprovedSkills.ActiveSkills[2],NAME_None,TEXT("covenant"));
            Add(TEXT("ultimate_day"),Ultimate+TEXT(" / Day"),TEXT("The approved allied field restoration."),
                INDEX_NONE,Set.ApprovedSkills.UltimateSkill,TEXT("day"));
            Add(TEXT("revival_prepare"),Ultimate+TEXT(" / Prepare Revival"),TEXT("Prepare revival on one living ally before defeat."),
                INDEX_NONE,Set.ApprovedSkills.UltimateSkill,TEXT("day"),TEXT("revival_prepare"));
            Add(TEXT("ultimate_night"),Ultimate+TEXT(" / Night"),TEXT("The approved hostile field branch."),
                INDEX_NONE,Set.ApprovedSkills.UltimateSkill,TEXT("night"));
        }
        else
        {
            Add(TEXT("ultimate_ruin"),Ultimate+TEXT(" / Ruin"),TEXT("The approved hostile field branch."),
                INDEX_NONE,Set.ApprovedSkills.UltimateSkill,TEXT("ruin"));
            Add(TEXT("ultimate_harvest"),Ultimate+TEXT(" / Hollow Crown"),TEXT("Select one living enemy."),
                INDEX_NONE,Set.ApprovedSkills.UltimateSkill,TEXT("harvest"));
        }
        Add(TEXT("fantasm"),Fantasm,TEXT("Activate the authored World field with its actual resource and Authority requirements."),
            INDEX_NONE,Field,NAME_None,NAME_None,true,true);
        Cached.Add(Source.SkillSet.CurrentForm,MoveTemp(Set));
    }
    const auto& Set=Cached[Source.SkillSet.CurrentForm];
    if (!SameKit(Source.SkillSet,Set.ApprovedSkills))
    { Error=TEXT("The current Manifestation does not carry this exact approved combat kit."); return false; }
    Out=Set; return true;
}
