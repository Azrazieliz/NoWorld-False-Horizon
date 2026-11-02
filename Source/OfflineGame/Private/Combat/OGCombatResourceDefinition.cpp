#include "Combat/OGCombatResourceDefinition.h"
bool FOGCombatResourceSpecification::Validate(FString& Error) const
{
    Error.Reset();
    if (!ResourceId.IsValid() || DisplayName.IsEmpty() || Maximum<1 || OpeningValue<0 || OpeningValue>Maximum)
    { Error=TEXT("Combat resource requires an exact ID, label and valid authored opening value/capacity."); return false; }
    return true;
}
bool FOGCombatResourceSpecification::operator==(const FOGCombatResourceSpecification& Other) const
{ return ResourceId==Other.ResourceId && DisplayName.EqualTo(Other.DisplayName) && Maximum==Other.Maximum && OpeningValue==Other.OpeningValue; }
bool FOGCombatResourceValue::operator==(const FOGCombatResourceValue& Other) const
{ return ResourceId==Other.ResourceId && DisplayName.EqualTo(Other.DisplayName) && Maximum==Other.Maximum && Value==Other.Value; }
UOGCombatResourceDefinition::UOGCombatResourceDefinition() { System=TEXT("combat_resource"); }
bool UOGCombatResourceDefinition::ValidateDefinition_Implementation(FString& Error) const
{
    if (!Super::ValidateDefinition_Implementation(Error)) return false;
    if (System!=TEXT("combat_resource") || DefinitionId!=Specification.ResourceId)
    { Error=TEXT("Combat resource definition key mismatch."); return false; }
    return Specification.Validate(Error);
}
