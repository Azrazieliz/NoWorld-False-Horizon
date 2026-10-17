"""Build a REAL editable Koikatsu witch-dragon character; no paid inference.
Single Maker session. Phases: base, variation, finish. Native snapshots and screenshots.
Original Steam install/source cards untouched. Noncanonical new-ID output.
"""
from __future__ import annotations
import argparse,hashlib,io,json,pathlib,shutil,sys,time,uuid,contextlib
import kk_one_session as master
import kk_native_visual_search as native

OUT=pathlib.Path(r"D:\UnrealProjects\NW_Blender_Private_Review_20261017\WITCH_20261018_NATIVE_BUILD")
OUT.mkdir(parents=True,exist_ok=True)
master.profile_valid();master.game_guards(stage=False)
submit=native.client_factory()
session=time.perf_counter()
def op(name,args=None,optional=False,timeout=11):
    args=args or {}
    t=time.perf_counter()
    try:
        nonce,secs,receipt=native.invoke(submit,{"op":name,"args":args},timeout=timeout)
        result={"op":name,"args":{k:v for k,v in args.items() if k!="morphs"},
            "morph_count":len(args.get("morphs",[])),
            "success":True,"seconds":secs,"nonce":nonce,"data":str(receipt.get("data",""))[-1700:]}
    except Exception as exc:
        result={"op":name,"args":{k:v for k,v in args.items() if k!="morphs"},
            "success":False,"error":str(exc)[:350],
            "seconds":round(time.perf_counter()-t,2)}
        print("NATIVE_FAILURE",name,result["error"],flush=True)
        if not optional:raise
    return result
def render(view,label):
    # fixed native camera bank; file never overwritten, full image at original camera res.
    n,secs,_=native.invoke(submit,{"op":"render_camera_view","args":{"group":view}},timeout=16)
    f=master.RUNTIME/"camera_bank"/f"SM01_CAM_{n}_{view}.png"
    if not f.exists():raise FileNotFoundError(f)
    target=OUT/(label+"_"+view+".png")
    if target.exists():raise FileExistsError(target)
    shutil.copy2(f,target)
    print("RENDER_READY",str(target),target.stat().st_size,flush=True)
    return {"camera":view,"image":str(target),"sha256":hashlib.sha256(target.read_bytes()).hexdigest()}
def acc(slot,guid,cat,item,parent,scale=None,pos=None,rot=None):
    r=[op("apply_accessory",{"slot":slot,"guid":guid,"category":cat,"original_id":item,"parent":parent},optional=True)]
    # Exact center-relative unit scale. Never use giant uncalibrated position offsets.
    for group,v in [("scale",scale),("pos",pos),("rot",rot)]:
        if v is not None and r[0]["success"]:
            r.append(op("set_accessory_vector",{"slot":slot,"group":group,
                "value":v[0],"v2":v[1],"v3":v[2]},optional=True))
    return r
def morphs():
    face={0:.41,1:.51,3:.42,4:.54,5:.32,6:.44,7:.54,8:.42,9:.32,
          13:.39,14:.31,15:.34,16:.38,19:.50,20:.49,
          24:.44,25:.42,26:.48,27:.45,28:.48,29:.47,
          30:.51,31:.48,32:.49,33:.29,34:.43,35:.44,
          38:.46,39:.5,41:.46,42:.38,44:.51,45:.50,46:.52}
    body={0:.80,1:.43,2:.98,3:.67,4:.47,5:.61,8:.86,9:.67,10:.73,
          15:.38,17:.44,18:.47,19:.42,20:.49,21:.46,22:.48,
          23:.39,24:.38,25:.31,26:.46,27:.36,28:.46,
          29:.62,30:.50,31:.43,32:.47,33:.40,34:.48,35:.43,
          38:.59,39:.55,40:.54,41:.53,42:.48,43:.48}
    return [{"group":g,"index":i,"value":float(x.get(i,.5))}
         for g,n,x in (("face",52,face),("body",44,body)) for i in range(n)]
def save_report(name,events,images):
    path=OUT/(name+"_RECEIPT.json")
    if path.exists():raise FileExistsError(path)
    path.write_text(json.dumps({"schema":"NW_WITCH_REAL_NATIVE_BUILD_V1",
        "phase":name,"events":events,"camera_images":images,
        "native_card_artistic_acceptance":False,
        "source_reference_width":832,"source_reference_height":1536,
        "source_artwork_available_in_chat":True,
        "native_build_on_authorized_PC":True},indent=2))
def base():
    events=[]
    # Fresh new character derived from current Maker native character; never modify existing card.
    # First remove current school preset accessories, if any, without touching source cards.
    for i in range(12):
        events.append(op("apply_accessory_native",{"slot":i,"effective_id":0,"category":122,
            "parent":"a_n_headtop"},optional=True))
    events.append(op("apply_morph_batch",{"morphs":morphs()}))
    events.append(op("set_eye_iris",{"value":.16,"v2":.43,"v3":.70}))
    events.append(op("set_iris_asset",{"effective_id":6},optional=True))
    for idx,hid in enumerate([36,19,2,0]):
        events.append(op("set_hair",{"index":idx,"effective_id":hid},optional=True))
    for idx,c in enumerate([(.09,.13,.32),(.10,.15,.33),(.06,.49,.62),(.10,.16,.34)]):
        events.append(op("set_hair_palette",{"index":idx,"value":c[0],"v2":c[1],"v3":c[2]},optional=True))
    events.append(op("set_clothes_top",{"guid":"Kraitx.Lisa.Mod","category":105,
        "original_id":130539}))
    events.append(op("set_clothes",{"index":1,"guid":"com.Quokka.sideslitskirtlong",
        "category":106,"original_id":122},optional=True))
    for idx in (4,5,6):
        events.append(op("set_clothes",{"index":idx,"effective_id":0},optional=True))
    for idx in (7,8):
        events.append(op("set_clothes",{"index":idx,"guid":"Kraitx.Lisa.Mod",
            "category":112,"original_id":130543},optional=True))
    events+=acc(0,"Kraitx.Mona.Mod",122,130584,"a_n_headtop",scale=(1.17,1.12,1.15))
    events+=acc(1,"cream002",123,2110,"a_n_head",scale=(.9,.9,.9),pos=(0,0,0))
    events+=acc(2,"Zeta.GenericAcc",126,10,"a_n_waist",scale=(1.0,1.0,1.0))
    events+=acc(3,"GaryuX.rinwellmod",129,6551432,"a_n_hand_R",scale=(.85,.85,.85))
    events+=acc(4,"kushinahairpack.mod",122,221100,"a_n_headtop",scale=(1.0,1.0,1.0))
    images=[render("front","BASE_WITCH"),render("face","BASE_WITCH")]
    save_report("BASE_WITCH",events,images)
    return events,images
def variation(which):
    events=[]
    if which=="yelan":
        events.append(op("set_clothes_top",{"guid":"com.redrayz.yelan","category":105,"original_id":590}))
        events.append(op("set_clothes",{"index":1,"guid":"com.redrayz.yelan","category":106,"original_id":591},optional=True))
    elif which=="lisa":
        events.append(op("set_clothes_top",{"guid":"Kraitx.Lisa.Mod","category":105,"original_id":130539}))
        events.append(op("set_clothes",{"index":1,"guid":"com.Quokka.sideslitskirtlong",
            "category":106,"original_id":122},optional=True))
    elif which=="dark":
        events.append(op("set_clothes_top",{"guid":"[Darobgy,Darobulance]VoidQueen_C2",
            "category":105,"original_id":52228}))
        events.append(op("set_clothes",{"index":1,"guid":"com.Quokka.sideslitskirtlong",
            "category":106,"original_id":122},optional=True))
    else:raise ValueError(which)
    images=[render("front",which.upper()+"_WITCH"),render("face",which.upper()+"_WITCH")]
    save_report(which.upper()+"_WITCH",events,images)
    return events,images
def accessories():
    events=[]
    # Override Mona hat with asymmetric deep purple brim shape if the latter fits better.
    events+=acc(0,"Augh.WitchHatDeux",122,2,"a_n_headtop",scale=(1.65,1.33,1.4),
                rot=(0,0,-12))
    # No enormous position offsets; a centered piece can sit naturally on the scalp.
    events+=acc(5,"Zeta.GenericAcc",122,2,"a_n_headtop",scale=(.68,.68,.75))
    events+=acc(6,"com.Quokka.thatbook",129,124,"a_n_hand_R",scale=(.82,.82,.82))
    images=[render("front","HAT_TRIAL"),render("face","HAT_TRIAL")]
    save_report("HAT_TRIAL",events,images)
    return events,images
def finalize():
    # Current Maker candidate must be approved through direct reference-vs-candidate inspection.
    name="WITCH_REF_832x1536_20261018_A"
    if (OUT/(name+"_CARD_RECEIPT.json")).exists():raise FileExistsError("Existing native final")
    events=[]
    images=[render("front",name+"_FINAL"),render("face",name+"_FINAL")]
    nonce="WITCH_NW_"+uuid.uuid4().hex[:14]
    buffer=io.StringIO()
    with contextlib.redirect_stdout(buffer):
        rc=submit({"op":"save_derived_card","nonce":nonce},20)
    receipt=json.loads(buffer.getvalue().strip().splitlines()[-1])
    if rc!=0 or receipt.get("status")!="RECEIPT_WRITTEN":
        raise RuntimeError("Card save failed:"+str(receipt)[:850])
    card_name="SM01_AUTOTEST_"+nonce+".png"
    verify=op("verify_derived_card",{"file_name":card_name},timeout=20)
    source=master.SANDBOX/"UserData"/"chara"/"female"/card_name
    if not source.is_file():raise FileNotFoundError("Native card missing:"+str(source))
    dst=OUT/(name+"_EDITABLE_KOIKATSU_CARD.png")
    if dst.exists():raise FileExistsError(dst)
    shutil.copy2(source,dst)
    result={"native_card":str(dst),"sha256":hashlib.sha256(dst.read_bytes()).hexdigest(),
      "saved_nonce":nonce,"native_receipt":receipt["data"][-900:],
      "verify_receipt":verify,"images":images,
      "valid_editable_card":verify["success"],"original_install_modified":False,
      "user_reference_visual_acceptance":False}
    (OUT/(name+"_CARD_RECEIPT.json")).write_text(json.dumps(result,indent=2))
    print("ACTUAL_NATIVE_CARD_CREATED",json.dumps({"card":str(dst),"bytes":dst.stat().st_size,
        "verify":verify["success"],"images":[r["image"] for r in images]}),flush=True)
def main():
    parser=argparse.ArgumentParser()
    parser.add_argument("--phase",choices=("base","yelan","lisa","dark","accessories","finish"),required=True)
    a=parser.parse_args()
    if a.phase=="base":base()
    elif a.phase in ("yelan","lisa","dark"):variation(a.phase)
    elif a.phase=="accessories":accessories()
    else:finalize()
if __name__=="__main__":main()
