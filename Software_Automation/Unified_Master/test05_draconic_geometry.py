"""Test 5: editable rigged crystalline-seraph-dragon geometry on native KKBP body.
No canonical assets changed. Blender 5.2.2. User permits omission of jewelry.
Procedural geometry based on rigid reference anatomy, not an image billboard.
"""
import bpy, math, random, json, pathlib, time
from mathutils import Vector
from math import sin, cos, pi
R=random.Random(20261017)
OUT=pathlib.Path(r'D:\UnrealProjects\NW_Blender_Private_Review_20261017\TEST05_CRYSTAL_SERAPH')
OUT.mkdir(parents=True,exist_ok=True)
START=time.perf_counter()
def coll(name):
 c=bpy.data.collections.get(name) or bpy.data.collections.new(name)
 if not c.name in bpy.context.scene.collection.children:bpy.context.scene.collection.children.link(c)
 return c
def put(o,c):
 for t in list(o.users_collection):t.objects.unlink(o)
 c.objects.link(o)
 return o
ART=coll('T05 | Editable Crystal Anatomy')
MAT=coll('T05 | Source Body Rig')
WINGS=coll('T05 | Four Articulated Wings')
HAIR=coll('T05 | Long Plum Hair')
SHELL=coll('T05 | Crystalline Armor Shell')
HORNS=coll('T05 | Antler Crown')
TAIL=coll('T05 | Rigged Scaled Dragon Tail')
RENDER=coll('T05 | Camera and Studio')
def material(name,color,metal=0,rough=.34,transmission=0,alpha=1,noise=.16):
 m=bpy.data.materials.new('T05 '+name);m.use_nodes=True;m.diffuse_color=(*color,alpha)
 n=m.node_tree.nodes;links=m.node_tree.links
 p=n.get('Principled BSDF')
 p.inputs['Base Color'].default_value=(*color,1)
 p.inputs['Metallic'].default_value=metal;p.inputs['Roughness'].default_value=rough
 p.inputs['Coat Weight'].default_value=.2 if metal else .09
 p.inputs['Coat Roughness'].default_value=.14
 if transmission:p.inputs['Transmission Weight'].default_value=transmission
 if alpha<1:
  p.inputs['Alpha'].default_value=alpha
  try:m.surface_render_method='DITHERED'
  except Exception:pass
 if noise:
  geo=n.new('ShaderNodeTexCoord');geo.location=(-620,0)
  vn=n.new('ShaderNodeTexNoise');vn.location=(-420,0)
  vn.inputs['Scale'].default_value=55 if metal else 125
  vn.inputs['Detail'].default_value=3
  vn.inputs['Roughness'].default_value=.7
  links.new(geo.outputs['Generated'],vn.inputs['Vector'])
  bump=n.new('ShaderNodeBump');bump.location=(-210,-130)
  bump.inputs['Strength'].default_value=noise
  bump.inputs['Distance'].default_value=.035 if metal else .016
  links.new(vn.outputs['Fac'],bump.inputs['Height']);links.new(bump.outputs['Normal'],p.inputs['Normal'])
 return m
ivory=material('Frosted Ivory Armor',(.72,.73,.75),.64,.25,noise=.23)
bright=material('Polished Silver Scale Tips',(.94,.95,1),.77,.16,noise=.07)
shadow=material('Smoky Graphite Armor',(.12,.122,.14),.67,.28,noise=.31)
shadow2=material('Aged Dark Lilac Steel',(.22,.18,.24),.70,.28,noise=.25)
plum=material('Plum Root Hair',(.22,.115,.20),.14,.25,noise=.12)
mauve=material('Plum Highlight Hair',(.46,.25,.35),.13,.27,noise=.10)
lilac=material('Lavender Hair Tips',(.76,.64,.75),.10,.32,noise=.08)
gold=material('Champagne Edges',(.62,.40,.23),.85,.22,noise=.09)
void=material('Amethyst Cosmic Void',(.025,.014,.046),.34,.1,noise=.3)
glow=material('Inner Violet Core',(.30,.15,.5),.14,.14,noise=.03)
skin=material('Pale Ivory Skin',(.78,.69,.70),.0,.4,noise=.09)
membrane=material('Smoky Rose Wing Membranes',(.26,.145,.21),.2,.4,transmission=.18,alpha=.84,noise=.34)
translucent=material('Frosted Pearlescent Wing Veil',(.64,.61,.69),.35,.22,transmission=.25,alpha=.74,noise=.21)
amethyst=material('Iridescent Crystal',(.47,.31,.54),.5,.12,transmission=.24,alpha=.93,noise=.07)
def make_mesh(name,verts,faces,mat,collection,inds=None):
 m=bpy.data.meshes.new(name);m.from_pydata(verts,[],faces);m.update()
 o=bpy.data.objects.new(name,m);collection.objects.link(o)
 for mm in (mat if isinstance(mat,(tuple,list)) else [mat]):m.materials.append(mm)
 if inds:
  for poly,idx in zip(m.polygons,inds):poly.material_index=idx
 for poly in m.polygons:
  if len(poly.vertices)>3:poly.use_smooth=True
 return o
def curve(name,pts,width,ma,collection,variable=None,res=4):
 cu=bpy.data.curves.new(name,'CURVE');cu.dimensions='3D';cu.resolution_u=20
 cu.bevel_depth=width;cu.bevel_resolution=res
 spl=cu.splines.new('BEZIER');spl.bezier_points.add(len(pts)-1)
 for i,(p,v) in enumerate(zip(spl.bezier_points,pts)):
  p.co=v;p.handle_left_type='AUTO';p.handle_right_type='AUTO'
  if variable:p.radius=max(.04,variable(i/(len(pts)-1)))
 ob=bpy.data.objects.new(name,cu);collection.objects.link(ob);cu.materials.append(ma);return ob
def taper_mesh(name,a,b,width,thickness,mat,collection,uv=False,flex=0):
 # leaf/feather geometry with channeled center ridge and swept pointed tip
 A=Vector(a);B=Vector(b);direction=(B-A)
 vertical=Vector((0,0,1))
 side=direction.cross(vertical)
 if side.length<.02:side=direction.cross(Vector((0,1,0)))
 side.normalize()
 fore=side.cross(direction).normalized()
 verts=[];faces=[];cols=5;rows=10
 for i in range(rows):
  t=i/(rows-1)
  center=A+direction*t+fore*(flex*sin(t*pi))
  silhouette=width*(sin(pi*t)**.65)*(.87+.13*sin(5.2*t+1.2))
  for j in range(cols):
   q=(j-2)/2
   v=center+side*silhouette*q+fore*(thickness*(1-q*q)*(1-t)*(.8+.2*cos(4*t)))
   verts.append(tuple(v))
 for i in range(rows-1):
  for j in range(cols-1):
   idx=i*cols+j;faces.append((idx,idx+1,idx+cols+1,idx+cols))
 o=make_mesh(name,verts,faces,mat,collection)
 return o
def mesh_smooth(o):
 mod=o.modifiers.new('Feather smooth surface','SOLIDIFY');mod.thickness=.003
 mod.offset=0
 return o
def crystal(name,a,b,r,ma=bright,collection=SHELL,facets=8):
 A=Vector(a);B=Vector(b);axis=(B-A).normalized()
 u=axis.cross(Vector((0,1,0)))
 if u.length<.1:u=axis.cross(Vector((0,0,1)))
 u.normalize();v=axis.cross(u).normalized()
 rings=[(A,r*.32),(A+(B-A)*.23,r),(A+(B-A)*.80,r*.50),(B,.02*r)]
 verts=[]
 for c,w in rings:
  for i in range(facets):
   ang=2*pi*i/facets
   verts.append(tuple(c+w*(u*cos(ang)+v*sin(ang))))
 faces=[]
 for k in range(len(rings)-1):
  for i in range(facets):
   j=(i+1)%facets
   faces.append((k*facets+i,k*facets+j,(k+1)*facets+j,(k+1)*facets+i))
 faces.append(tuple(range(facets-1,-1,-1)))
 return make_mesh(name,verts,faces,ma,collection)
def bone_rig(name,collection):
 data=bpy.data.armatures.new(name+' skeleton');o=bpy.data.objects.new(name+' rig',data);collection.objects.link(o)
 return o
def add_bones(rig,records):
 bpy.context.view_layer.objects.active=rig
 bpy.ops.object.mode_set(mode='EDIT')
 for name,start,end,parent in records:
  b=rig.data.edit_bones.new(name);b.head=start;b.tail=end;b.use_connect=False
  if parent:b.parent=rig.data.edit_bones[parent]
 bpy.ops.object.mode_set(mode='OBJECT')
 return rig
def rig_object(o,rig,bone):
 # Geometric bone deformation for authored procedural geometry, not mere unrigged staging.
 group=o.vertex_groups.new(name=bone)
 group.add(list(range(len(o.data.vertices))),1.0,'REPLACE')
 mod=o.modifiers.new('TEST05 real armature skinning','ARMATURE');mod.object=rig
 return o
# Native KKBP imported character, two armatures and all 452 native morphs remain editable.
for ob in bpy.data.objects:
 if ob.type=='ARMATURE':
  ob.scale=(12.45,)*3
  put(ob,MAT)
 elif ob.type=='MESH' and not ob.name.startswith('T05 '):
  ob.scale=(12.45,)*3
  put(ob,MAT)
  if ob.name=='model.001_mesh':
   ob.hide_render=True;ob.hide_viewport=True
  else:
   for i,m in enumerate(list(ob.data.materials)):
    if not m:continue
    n=m.name.lower()
    if 'hitomi' in n or 'eye' in n and 'white' not in n:
     c=material('Iridescent Amber Eyes '+str(i),(.55,.37,.16),.18,.19,noise=.08)
    elif 'body' in n or 'face' in n:
     c=skin
    elif 'eyeline' in n or 'mayuge' in n or 'matuyge' in n:
     c=shadow2
    elif 'sirome' in n:
     c=bright
    else:c=skin
    ob.data.materials[i]=c
   ob.name='TEST05 | Real KKBP Skinned Character Base'
# New anatomy rig: explicit usable wing and tail joints, independent from the KKBP skeleton
RIG=bone_rig('T05 Wing + Tail',WINGS)
records=[]
for sign,label in [(-1,'L'),(1,'R')]:
 records += [
 (f'{label}_upper_root',(sign*.18,.13,1.36),(sign*.47,.17,1.75),None),
 (f'{label}_upper_tip',(sign*.47,.17,1.75),(sign*.99,.19,2.1),f'{label}_upper_root'),
 (f'{label}_lower_root',(sign*.19,.14,1.2),(sign*.48,.18,.9),None),
 (f'{label}_lower_tip',(sign*.48,.18,.9),(sign*.85,.22,.38),f'{label}_lower_root')]
tailpaths=[(0,.18,.71),(.08,.27,.60),(.22,.30,.40),(.42,.33,.23),(.61,.31,.13),(.73,.29,.15),(.79,.28,.28),(.88,.29,.35),(1.05,.29,.26)]
for i in range(len(tailpaths)-1):
 a=tailpaths[i];b=tailpaths[i+1]
 records.append((f'Tail_{i:02d}',a,b,f'Tail_{i-1:02d}' if i else None))
add_bones(RIG,records)
def feather(name,a,b,width,matcol=ivory,thick=.018,rigbone=None,collection=WINGS):
 o=taper_mesh(name,a,b,width,thick,matcol,collection,flex=.02*(R.random()-.5))
 mesh_smooth(o)
 if rigbone:rig_object(o,RIG,rigbone)
 return o
def bat_wing(sign,label):
 # Main ribbed wing arm sweeps UP and OUT, folds back in a draconic semicircle.
 root=Vector((sign*.22,.17,1.34))
 leading=[root,Vector((sign*.39,.22,1.66)),Vector((sign*.61,.26,1.91)),
          Vector((sign*.94,.34,2.14)),Vector((sign*1.06,.39,2.09))]
 bones=[
  [root,Vector((sign*.46,.25,1.81)),Vector((sign*.94,.32,2.12))],
  [root,Vector((sign*.58,.27,1.58)),Vector((sign*1.10,.36,1.78))],
  [root,Vector((sign*.69,.26,1.38)),Vector((sign*1.08,.33,1.41))],
  [root,Vector((sign*.76,.25,1.18)),Vector((sign*.99,.28,1.05))]
 ]
 # Different membrane panels, outer edges scalloped, with small translucent incisions.
 for k in range(3):
  upper=bones[k];lower=bones[k+1]
  for j in range(4):
   t0=j/4;t1=(j+1)/4
   def proj(path,t):
    return path[0]*(1-t)**2+path[1]*2*t*(1-t)+path[2]*t*t
   a=proj(upper,t0);b=proj(upper,t1)
   c=proj(lower,t1);dd=proj(lower,t0)
   mid=(b+c)*.5+Vector((0,.016,-.028))
   verts=[tuple(a),tuple(b),tuple(c),tuple(dd),tuple(mid)]
   m=membrane if k<2 else translucent
   o=make_mesh(f'{label} Wing membrane {k}.{j}',verts,[(0,1,4),(0,4,3),(1,2,4),(4,2,3)],m,WINGS)
   rig_object(o,RIG,f'{label}_upper_root' if t1<.5 else f'{label}_upper_tip')
 for k,path in enumerate(bones):
  curve(f'{label} wing radial rib {k}',path,.010 if k<2 else .006,shadow,WINGS,
        variable=lambda t:1.2-.85*t)
  for j in range(17):
   t=.22+j/20
   a=path[0]*(1-t)**2+path[1]*2*t*(1-t)+path[2]*t*t
   direction=Vector((sign*(.035+.02*R.random()),.005,.045*R.random()))
   o=feather(f'{label} Wing armored shingle {k}.{j}',a-.45*direction,
             a+direction,.010+.007*(1-t),shadow if (j+k)%4==0 else ivory,
             .008,f'{label}_upper_tip' if t>.5 else f'{label}_upper_root')
 for i in range(68):
  t=(i+.3)/68
  u=R.uniform(.0,1)
  span=R.randrange(3)
  a=bones[span][0]*(1-t)**2+bones[span][1]*2*t*(1-t)+bones[span][2]*t*t
  b=bones[span+1][0]*(1-t)**2+bones[span+1][1]*2*t*(1-t)+bones[span+1][2]*t*t
  origin=a.lerp(b,u)
  tangent=(b-a).normalized()
  dest=origin+tangent*(.05+.08*R.random())+Vector((sign*.014,0,.015))
  feather(f'{label} Membrane scales {i}',origin,dest,.008+.007*R.random(),
          bright if R.random()>.72 else shadow2,.009,
          f'{label}_upper_root' if t<.5 else f'{label}_upper_tip')
 # Scalloped torn membrane tips: ivory edge feather blades taper toward wing outer.
 for k,path in enumerate(bones[1:]):
  for j in range(8):
   t=.45+j*.077
   a=path[0]*(1-t)**2+path[1]*2*t*(1-t)+path[2]*t*t
   b=a+Vector((sign*(.05+.055*R.random()),0,-(.055+.095*R.random())))
   feather(f'{label} Blade wing rim {k}.{j}',a,b,.011,bright if j%3 else shadow,.015,f'{label}_upper_tip')
 curve(f'{label} upper leading armored spine',leading,.025,shadow,WINGS,
       variable=lambda t:1.0-.7*t)
 for i in range(38):
  t=i/38
  p0=leading[0].lerp(leading[1],min(1,t*1.7)) if t<.55 else leading[2].lerp(leading[-1],(t-.55)/.45)
  feather(f'{label} Wing spine plating {i}',p0,
          p0+Vector((sign*.024,0,.036)),.012 if i%5 else .018,
          ivory if i%4 else shadow,.011,f'{label}_upper_root' if i<17 else f'{label}_upper_tip')
def lower_wing(sign,label):
 # Broad pair of pale overlapping feather fans, reaching almost to feet.
 root=Vector((sign*.19,.23,1.17))
 for layer in range(4):
  for k in range(55):
   t=k/54
   angle=pi*.25+(t*1.02)
   length=(.47+.51*sin(pi*t)**.55)*(1+.15*layer)
   # Ends describe a graceful elliptical sweep outward/downward.
   x=sign*(.26+.58*sin(t*pi*.64)+.025*layer)
   z=1.36-.78*t-.09*layer
   anchor=root+Vector((sign*(.08+.13*t),.02*layer,-.12*t))
   tip=Vector((x+.06*sign*length,.17+.014*layer,z-.29*length))
   tip+=Vector((sign*R.uniform(-.025,.025),0,R.uniform(-.05,.04)))
   side=R.uniform(.017,.035)+(.005 if layer==0 else 0)
   bone=f'{label}_lower_tip' if t>.43 else f'{label}_lower_root'
   ma=bright if layer%3==0 and k%4==0 else (ivory if (k+layer)%5 else shadow2)
   o=feather(f'{label} Secondary flight feather {layer}.{k}',anchor,tip,side,
        ma,thick=.02,rigbone=bone)
 # Transparent open spaces, segmented wing knuckles.
 for n in range(3):
  x=sign*(.24+.09*n)
  curve(f'{label} Lower wing flex shaft {n}',
       [(x,.22,1.17),(sign*(.4+.13*n),.29,.91),(sign*(.67+.10*n),.25,.46)],
       .013,shadow,WINGS,variable=lambda t:1.3-.8*t)
for sign,label in [(-1,'L'),(1,'R')]:
 bat_wing(sign,label)
 lower_wing(sign,label)
print('TEST05_WINGS_DONE',len(WINGS.objects),round(time.perf_counter()-START,2),flush=True)
# A character-rooted crown with branching crystalline antlers; jewelry omitted.
for sign,label in [(-1,'L'),(1,'R')]:
 major=[
 (sign*.09,-.06,1.69),(sign*.15,-.055,1.80),(sign*.30,-.055,1.92),
 (sign*.31,-.01,2.06),(sign*.40,.015,2.20)]
 curve(f'{label} antler main root',major,.020,shadow,HORNS,variable=lambda t:1-.86*t)
 for branch in range(5):
  t=.20+.14*branch
  idx=min(3,int(t*4))
  origin=Vector(major[idx])
  tip=origin+Vector((sign*(.09+.038*branch),-.02,.16+.035*branch))
  # Distinct horn branching silhouette, graded ivory-black facets.
  curve(f'{label} Fractured horn branch {branch}',[origin,origin.lerp(tip,.58),tip],
        .012,ivory if branch%2 else shadow,HORNS,
        variable=lambda u:1.1-.9*u)
  crystal(f'{label} Horn tip {branch}',origin.lerp(tip,.65),tip+.02*Vector((sign,0,1)),
          .009,bright,HORNS,facets=6)
 for j in range(18):
  t=j/17
  aa=Vector(major[0]).lerp(Vector(major[3]),t)
  aa+=Vector((0,-.010,0))
  bb=aa+Vector((sign*.017,0,.027+.02*R.random()))
  crystal(f'{label} Horn overlapping scales {j}',aa,bb,.011,
          bright if j%3 else shadow,HORNS,facets=6)
 for j in range(4):
  a=Vector((sign*(.115+j*.04),-.075,1.75+.03*j))
  b=a+Vector((sign*(.05+.02*j),-.02,.045+.024*j))
  crystal(f'{label} Brow arc spines {j}',a,b,.020,shadow2,HORNS)
# Hair — authored dozens of broad locks with individual finer secondary strands.
for side in [-1,1]:
 for layer in range(4):
  for j in range(31):
   t=j/30
   alpha=(t-.5)*pi*.91
   sidebias=side*(.05+abs(sin(alpha))*.14)
   root=Vector((sidebias+R.uniform(-.012,.012),-.065+.06*R.random(),1.56+.045*cos(alpha)))
   z=.40+R.random()*.62 if layer!=3 else 1.18+R.random()*.13
   x_end=side*(.27+.14*layer+.25*sin(pi*t)+R.random()*.11)
   curve(f'Flowing orchid hair {side}.{layer}.{j}',
      [root,Vector((root.x*.95,-.14,1.32)),
       Vector((x_end*.80,-.13,z+.19)),
       Vector((x_end,-.10,z))],
      .0021+.0021*R.random() if layer in (1,2) else .0044,
      plum if layer==0 else (mauve if layer==1 else lilac),HAIR,
      variable=lambda u:max(.035,(1-u)**.60),res=2)
 # Outer strands around shoulders and down legs, widely arcing with visible silhouette.
 for j in range(23):
  t=j/22;start=Vector((side*(.10+t*.08),.02,1.62+.02*R.random()))
  x=side*(.45+.15*R.random())
  curve(f'Cascade hair ribbon {side} {j}',
       [start,Vector((side*(.22+.1*t),-.18,1.24)),
        Vector((x,-.16,.87+.17*R.random())),
        Vector((x+side*.08,-.05,.46+.22*R.random()))],
       .0023+.002*R.random(),plum if j%3==0 else (mauve if j%3==1 else lilac),HAIR,
       variable=lambda u:(1-u)**.58,res=2)
# Forehead swept bangs, controlled in front of the face instead of floating.
for j in range(36):
 t=j/35
 start=Vector((-.14+.28*t,-.111,1.70+.014*sin(t*pi)))
 mid=Vector((-.13+.26*t,-.178,1.615+.02*sin(t*3*pi)))
 end=Vector((-.11+.25*t,-.192,1.49+.038*R.random()))
 curve(f'Face silhouette fringe {j}',[start,mid,end],
   .0034 if j%3==0 else .0022,plum if j%4 else mauve,HAIR,
   variable=lambda u:.95-.87*u,res=2)
print('TEST05_HAIR_HORNS_DONE',len(HAIR.objects),len(HORNS.objects),round(time.perf_counter()-START,2),flush=True)
# Torso armor integrated into draconic anatomy rather than an overlaid shirt mesh.
# Encircle the black jewel-like central void with a star-shaped crystalline aperture.
upper=1.24;lower=.73
N=26;ring=[];rim=[]
for i in range(N):
 t=2*pi*i/N
 x=(.09+.036*sin(3*t+.7)+.011*sin(11*t))*sin(t)
 z=.965+(.245+.018*sin(5*t+1.4))*cos(t)
 y=-.154-.019*cos(t*4)
 ring.append((x,y,z))
 rim.append((x*1.17,y-.008,z+(.026 if cos(t)>0 else -.016)*cos(t)))
v=ring+rim
faces=[]
for i in range(N):faces.append((i,(i+1)%N,(i+1)%N+N,i+N))
o=make_mesh('Hollow night core | engraved broken rim',v,faces,[void,gold],SHELL)
# Actual dark inset oval surface, with depth and purple faceted iridescence.
center=Vector((0,-.153,.96))
verts=[tuple(center+Vector((0,.034,0)))]
for i in range(N):
 x,y,z=ring[i];verts.append((x*.91,y+.023,z))
faces=[(0,i+1,(i+1)%N+1) for i in range(N)]
o=make_mesh('Recessed obsidian aether organ',verts,faces,void,SHELL)
for i in range(N):
 a=Vector(rim[i]);c=Vector(rim[(i+1)%N])
 crystal(f'Aperture rim splinter {i}',a,
       a+Vector((a.x*.27,-.011,.045*sin(i*2.17)+.024)),.010 if i%3 else .019,
       bright if i%4 else gold,SHELL)
for i in range(8):
 t=2*pi*i/8
 x=.058*sin(t);z=.965+.175*cos(t)
 crystal(f'Inner void suspended facets {i}',(x,-.191,z),
       (x+.02*sin(2*t),-.22,z+.07*cos(t)),.008,glow if i%2 else amethyst,SHELL,facets=5)
# Scale plates over upper arms, shoulders, bust, sides, hips, legs.
for sign,label in [(-1,'L'),(1,'R')]:
 # Shoulder pauldrons: translucent ivory + smoky-black scales.
 for layer in range(4):
  for j in range(11):
   t=j/10
   x=sign*(.15+.015*layer+.12*t)
   y=-.09+.005*layer
   z=1.41-.025*layer-.26*t
   a=(x,y,z);b=(x+sign*(.018+.014*R.random()),y-.035,z-.07-.026*R.random())
   feather(f'{label} shoulder obsidian feathers {layer}.{j}',a,b,.020,
     ivory if (j+layer)%3 else shadow,.018,None,SHELL)
 # Sculptural wing-root plate across breast; visible in front of native rig.
 for layer in range(5):
  for j in range(11):
   t=j/10
   x=sign*(.05+.16*t)
   z=1.41-.047*layer+.024*sin(t*pi)
   y=-.147-.012*sin(t*pi)
   a=(x,y,z);b=(x+sign*(.005+.01*t),y-.015,z-.045)
   feather(f'{label} fitted breast scale {layer}.{j}',a,b,.019,
      ivory if layer%2 else bright,.016,None,SHELL)
 # Side abdominal armor frames waist aperture without hiding it.
 for layer in range(9):
  for j in range(5):
   x=sign*(.135+.016*j)
   y=-.137+.006*j
   z=1.26-.055*layer
   feather(f'{label} torso flank lamella {layer}.{j}',(x,y,z),
      (x+sign*.01,y-.014,z-.065),.017 if j%3 else .023,
      shadow2 if j==4 else ivory,.020,None,SHELL)
 # Thighs: overlapping dragon-scale pattern over actual character legs.
 for row in range(18):
  z=.89-row*.025
  if z<.4:break
  for j in range(6):
   angle=(j-2.5)*pi/8
   x=sign*(.115+(.06+.013*sin(row*.5))*cos(angle))
   y=-.086-.047*sin(angle+.65)
   feather(f'{label} scale thigh {row}.{j}',(x,y,z),
      (x+sign*.008,-.097-.055*sin(angle+.65),z-.054),.023,
      ivory if (row+j)%4 else shadow2,.013,None,SHELL)
 # Calves/shins, joint guards, talon feet.
 for row in range(18):
  z=.42-row*.021
  for j in range(5):
   x=sign*(.095+(j-2)*.024)
   y=-.095-.016*cos(j*pi/4)
   feather(f'{label} shin scale {row}.{j}',(x,y,z),
           (x+sign*.008,y-.014,z-.044),.020,
           shadow if j in (0,4) else (ivory if row%3 else bright),.019,None,SHELL)
 for claw in range(3):
  x=sign*(.065+claw*.039)
  crystal(f'{label} foot talon {claw}',(x,-.11,.085),
          (x+sign*.016,-.19,.022),.020,shadow,SHELL,facets=6)
# Dragon tail: skinned segment cylinders and needle-like armored feathered fins.
tailbase=[Vector(p) for p in tailpaths]
for i in range(len(tailbase)-1):
 a=tailbase[i];b=tailbase[i+1]
 width=.10*(1-i/(len(tailbase)-1))+.014
 crystal(f'Tail bony segment {i}',a,b,width,shadow,TAIL,facets=8)
 for j in range(23):
  t=j/23
  p=a.lerp(b,t)
  ang=2*pi*j/23
  off=Vector((width*.70*cos(ang),-width*.50*sin(ang),.018*sin(j*1.8)))
  end=p+off+Vector((.04+.055*(i/8),.012,-.025))
  item=feather(f'Tail obsidian crystalline scales {i}.{j}',p+off,end,.012+.007*(1-i/8),
            ivory if j%4 else shadow2,.014,f'Tail_{i:02d}',TAIL)
  if j%6==0:
   crystal(f'Tail razor spines {i}.{j}',p+Vector((0,0,width*.4)),
           p+Vector((.05,-.002,width*1.45)),.021,bright,TAIL,facets=5)
# Clawed dragon tail terminal ice fan.
for j in range(16):
 angle=2*pi*j/16
 base=tailbase[-1]+Vector((.012*sin(angle),0,.012*cos(angle)))
 tip=tailbase[-1]+Vector((.09+.08*R.random(),-.04,.10*cos(angle)))
 crystal(f'Tail fin crystal blade {j}',base,tip,.022,
         bright if j%3 else amethyst,TAIL,facets=5)
print('TEST05_BODY_TAIL_DONE',len(SHELL.objects),len(TAIL.objects),round(time.perf_counter()-START,2),flush=True)
# Final studio: native PBR material nodes, strong silhouette with visible facet/strand contrast.
world=bpy.data.worlds.new('TEST05 neutral high-key studio')
bpy.context.scene.world=world;world.use_nodes=True
background=world.node_tree.nodes.get('Background')
background.inputs['Color'].default_value=(.68,.69,.73,1)
background.inputs['Strength'].default_value=.65
scene=bpy.context.scene
scene.render.engine='BLENDER_EEVEE'
scene.render.resolution_x=1280
scene.render.resolution_y=1580
scene.render.resolution_percentage=85
scene.render.image_settings.file_format='PNG'
scene.render.image_settings.color_mode='RGBA'
scene.render.film_transparent=False
scene.view_settings.view_transform='AgX'
scene.view_settings.look='AgX - Medium High Contrast'
scene.view_settings.exposure=.22
scene.view_settings.gamma=1.0
scene.render.image_settings.color_depth='8'
scene.render.filepath=str(OUT/'TEST05_CrystalSeraph_FullBody.png')
try:scene.render.engine='BLENDER_EEVEE'
except Exception:pass
try:scene.eevee.taa_render_samples=48
except Exception:pass
camera_data=bpy.data.cameras.new('Test05 proportional orthographic camera')
camera=bpy.data.objects.new('Test05 Full-Body Camera',camera_data)
RENDER.objects.link(camera);scene.camera=camera
camera.location=Vector((0,-5.2,1.105))
target=Vector((0,0,1.10))
camera.rotation_euler=(target-camera.location).to_track_quat('-Z','Y').to_euler()
camera_data.type='ORTHO';camera_data.ortho_scale=2.83
def area(name,location,power,side,color):
 data=bpy.data.lights.new(name,'AREA');light=bpy.data.objects.new(name,data);RENDER.objects.link(light)
 light.location=location
 light.rotation_euler=(Vector((0,0,1.16))-light.location).to_track_quat('-Z','Y').to_euler()
 light.data.energy=power;light.data.shape='DISK';light.data.size=side;light.data.color=color
area('Soft Key + Facial Detail',(-1.7,-2.8,3.3),370,3.6,(1,1,1))
area('Silver Wing Rim',(1.9,1.1,2.9),610,2.7,(.94,.92,1))
area('Rose Hair Bounce',(-2.0,.7,1.2),295,3.0,(1,.82,.91))
area('Warm Gold Facets',(1.8,-2.3,.9),140,2.6,(1,.9,.78))
# no jewelry/pendants created — intentionally excluded under user's request.
# Decorative scene elements are test-only, not Registry or canonical production content.
blend=OUT/'TEST05_CrystallineDragonSeraph_Editable.blend'
if blend.exists():raise FileExistsError('QA scene exists, will not overwrite')
# Preserve full named collections, armatures, mesh objects, vertex groups, modifiers and materials.
bpy.ops.wm.save_as_mainfile(filepath=str(blend))
blender_source_objects=[o for o in MAT.objects if o.type=='MESH']
meshes=[o for o in bpy.data.objects if o.type=='MESH']
shapes=sum(max(0,len(o.data.shape_keys.key_blocks)-1) for o in meshes if o.data.shape_keys)
triangles=sum(len(o.data.polygons) for o in meshes)
receipt={
 'schema':'NW_TEST05_CRYSTAL_SERAPH_EDITABLE_3D_V1',
 'status':'SOURCE_3D_GENERATED_PENDING_VISUAL_QA',
 'source':'native verified Test03 KKBP rigged body - provisional geometry base; new Test05 KK native card is independent until aligned',
 'non_canonical':True,'omitted':'jewelry and pendants',
 'wings':'four articulated layered bat+feather hybrid wings',
 'horns':'paired faceted branched antlers',
 'tail':'segmented 8-bone scaly spine and crystalline fins',
 'torso':'recessed dark crystalline core with broken rim',
 'hair':'long plum roots / lavender ends, layered individual native curves',
 'armatures':[(o.name,len(o.data.bones)) for o in bpy.data.objects if o.type=='ARMATURE'],
 'mesh_objects':len(meshes),'triangles_approx':triangles,'shape_keys':shapes,
 'wing_objects':len(WINGS.objects),'hair_objects':len(HAIR.objects),
 'shell_objects':len(SHELL.objects),'tail_objects':len(TAIL.objects),'horn_objects':len(HORNS.objects),
 'qa_blend_path':str(blend),'qa_blend_bytes':blend.stat().st_size,
 'visual_reference_fidelity_accepted':False,'UE_runtime_accepted':False,
 'elapsed_before_render_seconds':round(time.perf_counter()-START,1)}
(OUT/'TEST05_3D_BUILD_RECEIPT.json').write_text(json.dumps(receipt,indent=2))
print('TEST05_EDITABLE_SAVED',str(blend),round(time.perf_counter()-START,1),'meshes',len(meshes),'shapes',shapes,flush=True)
bpy.ops.render.render(write_still=True)
print('TEST05_FULL_RENDER_COMPLETE',str(OUT/'TEST05_CrystalSeraph_FullBody.png'),round(time.perf_counter()-START,1),flush=True)
