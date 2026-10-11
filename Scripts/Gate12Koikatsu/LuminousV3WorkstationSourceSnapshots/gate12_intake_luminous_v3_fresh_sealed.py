"""Gate12 sealed Luminous V3 card/state intake; no mods or game files modified."""
import hashlib,json,pathlib,zipfile,datetime,sys
BASE=pathlib.Path(r"D:\Tools\GameAssetPipeline\KoikatsuParty")
SRC=BASE/"handoffs/GATE00_LUMINOUS_V3_Koikatsu_3D_Source_and_Registry_Intake_20261011.zip"
OUT=BASE/"characters/LUMINOUS_VALENTINE_V3/gate12_fresh_export_20261011"
ORIGINAL_GAME=pathlib.Path(r"D:\Tools\Steam\steamapps\common\Koikatsu Party")
STATES=("CLOTHED","UNDERWEAR","NUDE")
SHA_ARCH="0fd7d10d09b925bbd27ae26771a3559d6664171984c1c916e0cc0bc96c62f782"
def sha(b):return hashlib.sha256(b).hexdigest()
assert sha(SRC.read_bytes())==SHA_ARCH
out=OUT/"SEALED_INPUTS"
out.mkdir(parents=True,exist_ok=True)
with zipfile.ZipFile(SRC) as z:
    assert z.testzip() is None
    manifest=json.loads(z.read("GATE01_LUMINOUS_V3/00_MANIFEST.json"))
    listed={x["path"]:x for x in manifest["individual_assets"]}
    states={}
    cards_on_disk=list((ORIGINAL_GAME/"UserData/chara/female").glob("*.png"))
    disk_sizes=set(listed["GATE01_LUMINOUS_V3/03_KK_PARTY_CARDS/PRIMARY_USER_SAVED/"+s+".png"]["bytes"] for s in STATES)
    diskhash={}
    for file in cards_on_disk:
        if file.stat().st_size not in disk_sizes:continue
        diskhash[sha(file.read_bytes())]=str(file)
    for s in STATES:
        name=f"GATE01_LUMINOUS_V3/03_KK_PARTY_CARDS/PRIMARY_USER_SAVED/{s}.png"
        content=z.read(name)
        assert len(content)==listed[name]["bytes"] and sha(content)==listed[name]["sha256"]
        assert content.startswith(b"\x89PNG\r\n\x1a\n")
        path=out/(s+".png")
        if path.exists():
            assert path.read_bytes()==content,("Refuse overwrite drift",s)
        else:
            path.write_bytes(content)
        assert sha(path.read_bytes())==sha(content)
        states[s]={"sha256":sha(content),"bytes":len(content),
           "source":"user_saved_Koikatsu_Party_card_embedded_character_data",
           "embedded_coordinate_zero_outfit_part_ids":manifest["embedded_coordinate_zero_part_ids"][s.lower()],
           "matching_primary_user_saved_party_card":diskhash.get(sha(content)),
           "sealed_staging_path":str(path)}
    for rel,outname in (("06_QA/USER_SAVE_COORDINATE_AUDIT.json","USER_SAVE_COORDINATE_AUDIT.json"),
                        ("06_QA/CHARACTER_BASE_STATE_MANIFEST.json","CHARACTER_BASE_STATE_MANIFEST.json")):
        data=z.read("GATE01_LUMINOUS_V3/"+rel)
        dest=out/outname
        if dest.exists():assert dest.read_bytes()==data
        else:dest.write_bytes(data)
    original="GATE01_LUMINOUS_V3/04_ORIGINAL_SOURCE/ORIGINAL_UNMODIFIED_SOURCE_KK_CARD.png"
    frozen="GATE01_LUMINOUS_V3/01_FROZEN_2D/FROZEN_V3_MODELING_AUTHORITY.png"
    important={"source_original_sha256":sha(z.read(original)),
       "frozen_2D_sha256":sha(z.read(frozen)),
       "manifest_selected_art_component":manifest["selected_splash_art_component"],
       "original_creator_card_redistribution_rights":"not verified"}
receipt={"schema":"GATE12_LUMINOUS_V3_SEALED_INPUT_PRE_EXPORT_1",
    "status":"PASS_ALL_THREE_PRIMARY_USER_SAVES_SHA256_AND_COORDINATE_ZERO_STATES",
    "creation_utc":datetime.datetime.now(datetime.timezone.utc).isoformat(),
    "character":"Luminous Valentine V3","one_character_three_clothing_states":True,
    "GATE00_engineering_go":True,"artwork_approved_fidelity_goal":"production_usable_not_pixel_perfect",
    "source_archive_sha256":SHA_ARCH,"three_states":states,
    "use_unfitted_GLB_as_final":False,"use_fallback_all7":False,
    "fresh_native_Party_export_done":False,"UE_import_done":False,"device_QA_done":False,
    "original_game_installation_modified":False,
    "important":important}
(OUT/"SEALED_INTAKE_RECEIPT.json").write_text(json.dumps(receipt,indent=2,ensure_ascii=False)+"\n",encoding="utf8")
print(json.dumps({"status":receipt["status"],"states":{k:{"sha256":v["sha256"],
    "already_in_party":bool(v["matching_primary_user_saved_party_card"]),
    "outfit_part_ids":v["embedded_coordinate_zero_outfit_part_ids"]} for k,v in states.items()},
    "output_dir":str(OUT)},indent=2))
