#!/usr/bin/env python3
"""Validate actual user-approved design/intake deltas only; no native asset claiming."""
import json,hashlib,re,pathlib
R=pathlib.Path(__file__).resolve().parent
def J(s):return json.loads((R/s).read_text(encoding="utf-8-sig"))
def H(p):return hashlib.sha256((R/p).read_bytes()).hexdigest()
def check(condition,msg):
 if not condition:raise AssertionError(msg)
print('Gate11 2026-10-20 additive registry source-validation')
a=J('luminous_family_versions_and_wardrobe.json'); b=J('base_v6_approved_design_ids.json');k=J('sources/g04_luminous_base_v6_exact_unmodified.json')
d=J('gate07_approved_policy_not_items.json');r7=J('sources/g07_feedback_decisions_exact_unmodified.json');m=J('gate09_four_native_clips_technical_intake.json');w=J('gate02_r1_verified_candidate_source.json');e=J('gate12_consumption_eligibility.json');q=J('registry_intake_delta.json')
check(H(b['source_payload']['path'])==b['source_payload']['sha256'],'kit source immutable sha');check(H(d['source_exact_unmodified']['path'])==d['source_exact_unmodified']['sha256'],'G07 sha');check(k['frozen_foundation_commit']==q['foundation_sha']==r7['basis_commit'],'frozen Foundation');
check(a['identity']['content_id']==b['intended_identity_content_id']==e['can_consume_as_source_now']['luminous_base_reference']['identity_id'],'identity join');check(a['versions']['BASE']['content_id']==b['intended_base_version_content_id'],'base version join')
keys=[x['content_id'] for x in b['ability_content_ids']+b['named_status_content_ids']];check(len(keys)==len(set(keys))==16,'unique exact source-linked IDs');check(all(re.fullmatch(r'[a-z0-9_.\-/]+:[a-z0-9_.\-/]+',x) for x in keys),'content ID grammar');
for f in b['ability_content_ids']:
 orig=next((z for z in k['core_skills']+[k['ultimate'],k['world_fantasm']] if z['id']==f['source_key']),None);check(orig and orig['name']==f['name'],'ability exact sourced')
check(sum(x['slot_type']=='active' for x in b['ability_content_ids'])==3,'active count');check(sum(x['slot_type']=='passive' for x in b['ability_content_ids'])==6,'passive count')
check(len(b['named_status_content_ids'])==len(k['statuses'])==5,'exact statuses');check(set(b['effect_family_source_identifiers'])==set(k['effect_taxonomy']['base_effect_families']),'effect families unchanged')
check(set(a['versions']['BASE']['wardrobes'])=={'CLOTHED','UNDERWEAR','NUDE'},'3 base states');check(a['versions']['BASE']['wardrobes']['CLOTHED']['role']=='DEFAULT_BASE','default');check(a['versions']['ALTER']['content_id'] is None and not a['versions']['ALTER']['base_kit_inherited'],'Alter no invented ID / kit')
check(all(not x['eligible_for_other_wardrobes_or_Alter'] for x in m['clips']) and len(m['clips'])==4 and len(set(x['source_fbx_sha256'] for x in m['clips']))==4,'four unique rig-specific anims');check(m['skeleton_bone_count']==975 and not m['runtime_anim_bp_connected'],'rig-only QA');
check(w['stable_placements']==10454 and w['tiles']==625 and not w['canonical_location_content_ids'],'R1 source only');check(not w['gate11_artifact_ready'],'World not ready')
accepted={f'G07-D{x:02}' for x in (1,2,5,7,8,9,11,12,13,14,16,17,18)};deferred={f'G07-D{x:02}' for x in (3,4,6,10,15)};check(set(d['accepted_design_direction_ids'])==accepted and set(d['deferred_ids'])==deferred,'13 vs 5')
check(all(x['user_feedback']['code']==('A' if x['decision_id'] in accepted else 'D') for x in r7['decisions']),'source feedback');check(sum(d['conditional_mr_top_all_six_banner']['rarity_rates_bps'].values())==10000,'banner rate total');check(not d['conditional_mr_top_all_six_banner']['enabled'] and not d['conditional_mr_top_all_six_banner']['pool_content_ids'],'no banner activated');
check(q['historical_registry']['records_unchanged']==31 and not q['immutable_rules']['no_artifact_ready'] is False,'baseline/no ready');check(not a['package']['activated'] and a['ownership']['owned_manifestation_uuid'] is None,'no package, ownership');check(d['source_approved_content_definitions']['Dharma_id'] is None and not d['source_approved_content_definitions']['individual_items_or_material_ids'],'no item fabrication');check(all(not x.get('native_visual_catalog_eligible',False) for x in [a]),'no visual activation');
check(b['approval_scopes']['alter_inherits_base_kit'] is False and a['wardrobe_privacy_rule']['privacy_changes_equipment'] is False and a['wardrobe_privacy_rule']['privacy_changes_actual_wardrobe'] is False,'base/alter/privacy invariants');
print('PASS: 38 source/logic invariants in nine immutable/source-derived records; native Unreal/device tests NOT executed')
