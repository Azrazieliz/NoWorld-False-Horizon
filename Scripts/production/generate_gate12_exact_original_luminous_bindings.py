"""Generate exact native source path registry from immutable verified approved-card UE receipts.
No candidate/model guessing and no per-form animation catalog. Regenerate only if receipts change."""
import json,pathlib,hashlib
R=pathlib.Path(__file__).resolve().parents[2]
P=R/"docs/production/gate12/evidence/verified-original-koikatsu-native/G12_Awakened_Berserker_ExactOriginal_NativeUEImportAccepted.json"
proof=json.loads(P.read_text("utf8"))
assert proof["status"]=="PASS_AWAKENED_AND_BERSERKER_TRUE_ORIGINAL_COMPLETE_NATIVE_UE_SKINNED_SOURCES_NOT_DEVICE_ACCEPTED"
forms={}
for form in ("Awakened","Berserker"):
 data=proof["forms"][form]
 assert data["complete_native_3d_character_source"] and data["native_skeletal_mesh_count"]==9
 assert data["complete_source_body_coverage"] and len(data["installed_native_parts"])==9
 assert data["rig_bones_source"] in (1073,882)
 parts=sorted(data["installed_native_parts"],key=lambda x:x["source_id"])
 unique=set()
 for part in parts:
  assert part["native_skeletal_mesh_saved_and_built"]
  mesh=part["skeletal_mesh"]
  assert mesh and mesh.startswith("/Game/NoWorld/Characters/Luminous/"+form+"/ExactOriginalClothed/")
  assert mesh not in unique;unique.add(mesh)
  file=R/"Content"/pathlib.Path(mesh.removeprefix("/Game/")+".uasset")
  assert file.is_file(),f"Native imported package missing: {file}"
 forms[form]=data
H=R/"Source/OfflineGame/Public/Koikatsu/OGExactOriginalLuminousParts.h"
C=R/"Source/OfflineGame/Private/Koikatsu/OGExactOriginalLuminousParts.cpp"
H.write_text('''#pragma once
#include "CoreMinimal.h"
#include "UObject/SoftObjectPath.h"

// Exact source-card imported and verified from Koikatsu -> Blender -> UE 5.8.
// No placeholder, no substitution, no fifth ordinary lineage.
// These entries affect rendered parts, NOT the shared animation library.
struct OFFLINEGAME_API FOGExactOriginalLuminousParts
{
    static bool GetClothed(const FString& VersionId,
        TArray<FSoftObjectPath>& OutPaths, int32& OutSourceBones);
    static bool IsInstalled(const FString& VersionId);
};
''',"utf8")
lines=['#include "Koikatsu/OGExactOriginalLuminousParts.h"',
       '#include "Engine/SkeletalMesh.h"', '#include "Animation/Skeleton.h"',
       '#include "UObject/UObjectGlobals.h"', '', 'namespace {',
       'struct FExactForm { const TCHAR* Version; int32 SourceBones; const TCHAR* const* Parts; int32 Count; };']
for form,data in forms.items():
 lines+=['static const TCHAR* const G'+form+'Parts[] = {']
 for part in sorted(data["installed_native_parts"],key=lambda x:x["source_id"]):
  mesh=part["skeletal_mesh"];asset=mesh.rsplit("/",1)[-1]
  lines.append('    TEXT("'+mesh+'.'+asset+'"),')
 lines+=['};','']
lines+=['static const FExactForm GForms[] = {',
    ' { TEXT("character:luminous_valentine.version_awakened_v4"), 1073, GAwakenedParts, UE_ARRAY_COUNT(GAwakenedParts) },',
    ' { TEXT("character:luminous_valentine.version_berserker_v4"), 882, GBerserkerParts, UE_ARRAY_COUNT(GBerserkerParts) },',
    '};','}', '',
    'bool FOGExactOriginalLuminousParts::GetClothed(const FString& VersionId,',
    '    TArray<FSoftObjectPath>& OutPaths, int32& OutSourceBones)',
    '{',
    '    OutPaths.Reset(); OutSourceBones = 0;',
    '    for (const FExactForm& Form : GForms)',
    '        if (VersionId == Form.Version)',
    '        {',
    '            for (int32 I = 0; I < Form.Count; ++I)',
    '                OutPaths.Add(FSoftObjectPath(Form.Parts[I]));',
    '            OutSourceBones = Form.SourceBones;',
    '            return Form.Count == 9;',
    '        }',
    '    return false;',
    '}',
    '',
    'bool FOGExactOriginalLuminousParts::IsInstalled(const FString& VersionId)',
    '{',
    '    TArray<FSoftObjectPath> Paths; int32 SourceBones = 0;',
    '    if (!GetClothed(VersionId, Paths, SourceBones)) return false;',
    '    for (const FSoftObjectPath& Path : Paths)',
    '    {',
    '        USkeletalMesh* Mesh = Cast<USkeletalMesh>(Path.TryLoad());',
    '        if (!Mesh || !Mesh->GetSkeleton() ||',
    '            Mesh->GetRefSkeleton().GetNum() < SourceBones)',
    '            return false;',
    '    }',
    '    return true;',
    '}','']
C.write_text("\n".join(lines),"utf8")
print("EXACT_USER_APPROVED_NATIVE_3D_BINDING_GENERATED",[(k,len(v["installed_native_parts"]),v["rig_bones_source"]) for k,v in forms.items()])
