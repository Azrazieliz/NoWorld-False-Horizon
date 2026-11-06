"""Repair evidenced Unreal 5.8 compiler errors while preserving all game content."""
from pathlib import Path
import re
root=Path(r"D:\UnrealProjects\Gate12_WholeCharacter_Build_20261031\Source\OfflineGame\Private")
pending={}
def patch(name,old,new,n=1):
 p=root/(name+".cpp");s=pending.get(p,p.read_text("utf8"))
 if s.count(old)!=n:
  raise RuntimeError(f"{name}: expected {n} of {old!r}, actual {s.count(old)}")
 pending[p]=s.replace(old,new)
def include(name,inc,after):
 p=root/(name+".cpp");s=pending.get(p,p.read_text("utf8"))
 if inc in s:return
 if after not in s:raise RuntimeError(f"missing anchor {after} in {name}")
 pending[p]=s.replace(after,after+"\n"+inc,1)
def replace_if(name,old,new):
 p=root/(name+".cpp");s=pending.get(p,p.read_text("utf8"))
 if old in s:pending[p]=s.replace(old,new)
# Engine TSharedRef must not call ToSharedRef again.
for name,values in [
 ("Content/OGDefinitionLibrary",["Empty"]),
 ("World/OGOwnedEquipmentService",["Receipt"]),
 ("Combat/OGCombatActionJournal",["Payload","Receipt","Json","Event"]),
 ("Combat/OGCombatRewardDefinition",["Payload"]),
 ("Combat/OGTurnBattleNativeEffects",["Json"])]:
 for key in values:replace_if(name,key+".ToSharedRef()",key)
patch("Content/OGDefinitionLibrary",'TEXT("Unknown catalog root field: ")+Pair.Key','FString(TEXT("Unknown catalog root field: "))+FString(Pair.Key.ToString())')
include("Content/OGDefinitionLibrary",'#include "AssetRegistry/AssetData.h"','#include "AssetRegistry/AssetRegistryModule.h"')
patch("World/OGEquipmentCatalog",'TEXT("unknown field ") + Field.Key','FString(TEXT("unknown field "))+FString(Field.Key.ToString())')
patch("World/OGEquipmentCatalog",'Pair.Key != Pair.Key.ToLower()','FString(Pair.Key.ToString()) != FString(Pair.Key.ToString()).ToLower()')
patch("Content/OGSystemBindingComponent",'for (auto* Receiver:ActiveReceivers)','for (const auto& Receiver:ActiveReceivers)')
patch("Animation/OGSharedMotionControllerComponent",'for (auto* Part:PlayingParts)','for (const auto& Part:PlayingParts)')
patch("Animation/OGSharedMotionControllerComponent",'Part->GetLeaderPoseComponent()','Part->LeaderPoseComponent.Get()')
patch("Combat/OGNativeCombatCheckpoint",'Candidate.TriggerRuntime.ConditionEvaluator=MoveTemp(Bindings.Conditions);','Candidate.TriggerRuntime.SetConditionEvaluator(MoveTemp(Bindings.Conditions));')
patch("Combat/OGCombatVitalityService",'#include "Misc/LexFromString.h"\n','')
patch("UI/OGGate12NativeRulerInventory",'#include "Misc/GuardValue.h"\n','')
patch("UI/OGGate12NativeRulerGacha",'Shield->InitIsFocusable(false);','')
patch("UI/OGNativeBattleWidget",'Button->SetIsFocusable(true); ','')
patch("UI/OGNativeBattleWidget",'if (auto* Slot=Cast<UButtonSlot>(Button->AddChild(Text)))\n    { Slot->SetPadding(FMargin(12,10)); Slot->SetVerticalAlignment(VAlign_Center); }','if (auto* NativeButtonSlot=Cast<UButtonSlot>(Button->AddChild(Text)))\n    { NativeButtonSlot->SetPadding(FMargin(12,10)); NativeButtonSlot->SetVerticalAlignment(VAlign_Center); }')
include("UI/OGNativeInteractionWidget",'#include "Components/Overlay.h"','#include "Components/Border.h"')
patch("UI/OGNativeInteractionWidget",'auto* Slot = Canvas->AddChildToCanvas(FOGNativeUiStyle::Surface(*WidgetTree,Panel));','auto* RootCanvasSlot = Canvas->AddChildToCanvas(FOGNativeUiStyle::Surface(*WidgetTree,Panel));')
patch("UI/OGNativeInteractionWidget",'Slot->SetAnchors(FAnchors(.04f, .08f, .60f, .92f)); Slot->SetOffsets(FMargin(0));','RootCanvasSlot->SetAnchors(FAnchors(.04f, .08f, .60f, .92f)); RootCanvasSlot->SetOffsets(FMargin(0));')
patch("UI/OGNativeInteractionWidget",'if (auto* Slot = Cast<UCanvasPanelSlot>(Canvas->GetChildAt(0)->Slot))\n                { const auto Size = Geometry.GetLocalSize(); Slot->SetAnchors(','if (auto* ActiveCanvasSlot = Cast<UCanvasPanelSlot>(Canvas->GetChildAt(0)->Slot))\n                { const auto Size = Geometry.GetLocalSize(); ActiveCanvasSlot->SetAnchors(')
patch("UI/OGGate12GothicChromeWidget",'const bool Navigation=Kind==EOGNativeSurface::Navigation;','const bool bNavigationSurface=Kind==EOGNativeSurface::Navigation;')
# Only the plain standalone Navigation identifier; do not change enum literals.
key=root/"UI/OGGate12GothicChromeWidget.cpp"
s=pending[key];s=re.sub(r'(?<!::)\bNavigation\b','bNavigationSurface',s)
s=s.replace('EOGNativeSurface::bNavigationSurface','EOGNativeSurface::Navigation')
pending[key]=s
patch("UI/OGGate12NativeRulerGacha",'const FLinearColor Main(.92f,.91f,.94f,1),Muted(.59f,.59f,.65f,1),Gold(.72f,.62f,.44f,1);','const FLinearColor Main(.92f,.91f,.94f,1),GachaMuted(.59f,.59f,.65f,1),Gold(.72f,.62f,.44f,1);')
key=root/"UI/OGGate12NativeRulerGacha.cpp"
s=pending[key];start=s.index('const FLinearColor Main(.92f,.91f,.94f,1),GachaMuted')
end=s.find('\nvoid ',start)
if end<0:end=len(s)
block=s[start:end].replace(',Muted)',',GachaMuted)').replace(',Muted,',',GachaMuted,')
block=block.replace('auto* Padding=WidgetTree->ConstructWidget<UBorder>();','auto* ModalPadding=WidgetTree->ConstructWidget<UBorder>();')
block=block.replace('Padding->','ModalPadding->').replace('Surface(*WidgetTree,Padding,','Surface(*WidgetTree,ModalPadding,')
pending[key]=s[:start]+block+s[end:]
patch("Koikatsu/OGLuminousV3PlayableCharacter",'const USkeletalMesh* Mesh = Part ? Part->GetSkeletalMeshAsset() : nullptr;\n        if (Mesh && Mesh->GetSkeleton() == Sequence->GetSkeleton())','const USkeletalMesh* SourceRigMesh = Part ? Part->GetSkeletalMeshAsset() : nullptr;\n        if (SourceRigMesh && SourceRigMesh->GetSkeleton() == Sequence->GetSkeleton())')
patch("Animation/OGSharedReferenceMotion",'FString Layer,Name;','FString Layer,MorphName;')
patch("Animation/OGSharedReferenceMotion",'TryGetStringField(TEXT("morph"),Name) || Name.IsEmpty()','TryGetStringField(TEXT("morph"),MorphName) || MorphName.IsEmpty()')
patch("Animation/OGSharedReferenceMotion",'Track.Morph=FName(*Name);','Track.Morph=FName(*MorphName);')
patch("Animation/OGSharedAnimationRoutes",'FindRoute(FName System,FName Event) const\n{ return Routes.FindByPredicate([&](const auto& Route){return Route.SourceSystem==System && Route.Event==Event;}); }','FindRoute(FName SourceSystem,FName Event) const\n{ return Routes.FindByPredicate([&](const auto& Route){return Route.SourceSystem==SourceSystem && Route.Event==Event;}); }')
patch("World/OGGate12OwnedLuminousCombatSurface",'const float Top=H-Edge-Rows*(ButtonH+Gap);','const float CommandGridTop=H-Edge-Rows*(ButtonH+Gap);')
key=root/"World/OGGate12OwnedLuminousCombatSurface.cpp"
s=pending[key];start=s.index('const float CommandGridTop');end=s.index('// Error messages are actual canonical preflight errors',start)
pending[key]=s[:start]+s[start:end].replace('Top-','CommandGridTop-')+s[end:]
patch("World/OGGate12OwnedLuminousCombatSurface",'Bottom-35.f*S,18.f*S,Accent,W-2.f*Edge','H-Edge-35.f*S,18.f*S,Accent,W-2.f*Edge')
patch("World/OGGate02R1RealSourceRuntime",'Terrain->SetMaterial(CurrentSection,TerrainMaterial);','Terrain->SetMaterial(CurrentSection,TerrainMaterial.Get());')
patch("UI/OGReactiveUiSubsystem",'Json->Values.Find(Pair.Key.ToString())','Json->TryGetField(Pair.Key.ToString())')
patch("UI/OGReactiveUiSubsystem",'auto* Pawn=Player?Player->GetPawn():nullptr;','APawn* Pawn=Player?Player->GetPawn().Get():nullptr;')
patch("UI/OGReactiveUiSubsystem",'if (auto* Binding=Pawn->FindComponentByClass<UOGSystemBindingComponent>();','if (auto Binding=Pawn->FindComponentByClass<UOGSystemBindingComponent>();')
patch("UI/OGReactiveUiSubsystem",'else if (auto* P=Pawn->FindComponentByClass<UOGCanonicalCharacterPresentation>();','else if (auto P=Pawn->FindComponentByClass<UOGCanonicalCharacterPresentation>();')
include("Tests/OGObjectActionRuntimeTests",'#include "UObject/Package.h"','#include "Misc/Paths.h"')
include("Animation/OGSharedMotionLibrary",'#include "UObject/Package.h"','#include "Animation/OGSharedMotionLibrary.h"')
patch("Tests/OGGate12CharacterVisualCatalogTests",'NewObject<UOGGate12CharacterVisualCatalog>(GetTransientPackage())','NewObject<UOGGate12CharacterVisualCatalog>(GetTransientPackageAsObject())')
patch("Animation/OGSharedMotionLibrary",'NewObject<UOGSharedMotionLibrary>(GetTransientPackage())','NewObject<UOGSharedMotionLibrary>(GetTransientPackageAsObject())',2)
patch("Tests/OGReactiveUiTests",'Title->Slot.Get()==Slot','Title->Slot.Get()==static_cast<UPanelSlot*>(Slot)')
# Repaired UPackage lifetime and complete UObject hierarchy.
patch("Tests/OGObjectActionRuntimeTests",'auto* Package=CreatePackage(','UPackage* Package=CreatePackage(')
for path,s in pending.items():
 if s!=path.read_text("utf8"):
  path.write_text(s,"utf8")
  print("COMPILER_FIX",path.relative_to(root))
print("TOTAL_PATCHED_FILE_COUNT",len(pending))
