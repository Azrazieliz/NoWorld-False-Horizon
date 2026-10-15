"""Search 11,174 actual installed Unreal 5.8.3 exposed APIs by subsystem,
available methods, full scriptable reference, doc brief, C++ source module.
No Unreal Editor boot required. No internet.
"""
import pathlib,json,re,argparse
HERE=pathlib.Path(__file__).resolve().parent
registry=json.loads((HERE/"UE58_ExposedAPIs_Installed.json").read_text(encoding="utf-8"))
syn={"terrain":"Landscape WorldPartition PCG heightfield terrain GeometryScript",
 "characters":"SkeletalMesh Skeleton MorphTarget PhysicsAsset ControlRig IKRetarget AnimSequence",
 "animation":"AnimSequence AnimationBlueprint ControlRig IKRetarget PoseSearch MotionMatching AnimMontage",
 "combat":"GameplayAbility GameplayEffect GameplayTags AttributeSet IKRetarget Animation notify",
 "ai":"StateTree BehaviorTree Blackboard MassEntity Navigation AIController",
 "camera":"CineCamera LevelSequence MovieScene CameraCut Sequencer",
 "cinematics":"LevelSequence MovieScene CineCamera MoviePipeline Queue",
 "sound":"MetaSound AudioComponent SoundCue SoundWave",
 "vfx":"Niagara NiagaraSystem NiagaraEmitter MaterialExpression",
 "mobile":"Android Mobile Renderer DeviceProfile Packaging Optimization",
 "procedural":"PCGGraph PCGSettings GeometryScript",
 "materials":"MaterialEditingLibrary MaterialExpression Texture2D MaterialInstanceConstant",
 "ui":"UserWidget WidgetBlueprint Slate CommonUI",
 "blueprint":"BlueprintFactory BlueprintEditorLibrary Kismet",
 "input":"EnhancedInput InputAction InputMappingContext",
 "physics":"Chaos RigidBody PhysicsConstraint",
 "persistence":"SaveGame PrimaryDataAsset DataTable Serializer"}
def tokenize(s):
 return re.findall(r"[a-z0-9]+",s.lower())
def query(text,limit=12):
 q=tokenize(text); expanded=set(q)
 for t in q:
  if t in syn:expanded.update(tokenize(syn[t]))
 scores=[]
 for key,r in registry["classes"].items():
  body=key+" "+r.get("description","")+" "+" ".join(r.get("methods",[]))
  words=set(tokenize(body));name=set(tokenize(key));matches=expanded & words
  if not matches:continue
  score=sum(12 if w in name else 3 for w in matches)
  score+=18 if text.lower() in key.lower() else 0
  if key.lower() in expanded:score+=20
  if score>0:scores.append((score,key,r))
 scores.sort(key=lambda x:(-x[0],x[1]))
 return {"engine":registry["engine"],"query":text,
   "total_apis":registry["count"],
   "results":[{"rank":score,"api":key,"type":r["kind"],"domain":r["domain"],
    "description":r.get("description",""),"methods":r.get("methods",[])[:55]}
        for score,key,r in scores[:limit]]}
def main():
 p=argparse.ArgumentParser();p.add_argument("--query",required=True)
 p.add_argument("--limit",type=int,default=12);args=p.parse_args()
 print(json.dumps(query(args.query,max(1,min(40,args.limit))),indent=2,ensure_ascii=False))
if __name__=="__main__":main()
