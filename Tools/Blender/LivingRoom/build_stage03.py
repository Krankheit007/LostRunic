"""Add source-visible selective detail to the approved Stage02 via Blender MCP."""
import bpy, sys, json, hashlib, math
from pathlib import Path
BASE=Path('D:/25DGame/LostRunic/ArtSource/LivingRoom')
OUT=BASE/'Stage03'
sys.path.insert(0,str(Path(__file__).parent))
from structure_common import box, beam, mesh, root, material

def signature(o):
    data={'parent':o.parent.name if o.parent else None,
          'matrix':[list(r) for r in o.matrix_basis]}
    if o.type=='MESH':
        data.update(vertices=[list(v.co) for v in o.data.vertices],
                    faces=[list(f.vertices) for f in o.data.polygons])
    return hashlib.sha256(json.dumps(data,sort_keys=True).encode()).hexdigest()

def mark(o,source):
    o.name='S03_'+o.name.removeprefix('S02_')
    o['level']=3
    o['concept_evidence']=source
    return o

def strip(p,n,loc,size,mat,source):
    return mark(box(p,n,loc,size,mat,.0015),source)

def frame(p,n,x,y,z,w,h,t,mat,source):
    # Four closed strips; no coplanar overlays on the underlying panel.
    for side in (-1,1):
        strip(p,n+'Vertical',(x+side*(w-t)/2,y,z),(t,.012,h),mat,source)
        strip(p,n+'Horizontal',(x,y,z+side*(h-t)/2),(w-2*t,.012,t),mat,source)

def knob(p,x,y,z,mat):
    mark(beam(p,'HandleStem',(x,y,z),(x,y+.022,z),.007,mat),'Small brass furniture pull visible in concept')
    bpy.ops.mesh.primitive_uv_sphere_add(segments=12,ring_count=6,location=(x,y+.023,z))
    o=bpy.context.object
    o.scale=(.016,.010,.016)
    bpy.ops.object.transform_apply(location=False,rotation=False,scale=True)
    from structure_common import attach
    mark(attach(o,p,'Handle',mat),'Small brass furniture pull; exact hidden profile inferred')

def build():
    if Path(bpy.data.filepath).resolve()!= (BASE/'Stage02/LivingRoom_Stage02.blend').resolve():
        raise RuntimeError('Open approved Stage02 before executing this build')
    OUT.mkdir(exist_ok=True)
    before={o.name:signature(o) for o in bpy.context.scene.objects if o.type in ('MESH','EMPTY')}
    brass=material('DetailAgedBrass',(.29,.205,.085))
    wood=material('DetailWood',(.19,.119,.071))
    p=root('Fireplace'); stone=bpy.data.objects['MantelEnvelope'].data.materials[0]
    for x in (-.68,.68):
        frame(p,'PierMoulding',x,.201,.72,.145,.90,.016,stone,'Concept fireplace vertical recessed pier trim')
        strip(p,'Capital',(x,.199,1.275),(.225,.065,.065),stone,'Concept squared capitals under mantel')
        frame(p,'CapitalInset',x,.237,1.28,.13,.09,.018,stone,'Concept squared capital relief; simplified')
        strip(p,'PierBase',(x,.191,.20),(.225,.055,.07),stone,'Concept fireplace pier foot moulding')
    strip(p,'UnderMantel',(0,.20,1.431),(1.62,.07,.035),stone,'Concept stepped edge below mantel')
    strip(p,'LintelLower',(0,.189,1.245),(1.09,.026,.025),stone,'Concept broad horizontal lintel trim')
    # Painting silhouette is retained; narrow inner metallic lips sit on its frame.
    for p in [o for o in bpy.context.scene.objects if o.name.startswith('ROOT_Painting')]:
        old=next(o for o in p.children if o.name.startswith('FrameEnvelope'))
        x,y,z=old.location;w,d,h=old.dimensions
        frame(p,'InnerFrameLip',x,y+d/2+.004,z,w-.038,h-.038,.012,brass,'Concept thin aged gold inner picture-frame lip')
    # Details belong to each moving panel, so the accepted axes remain authoritative.
    for p in [o for o in bpy.context.scene.objects if o.name.startswith('PIVOT_')]:
        fronts=[o for o in p.children if o.type=='MESH' and (o.name.endswith('_Front') or o.name.endswith('_Panel'))]
        for o in fronts:
            x,y,z=o.location;w,d,h=o.dimensions
            frame(p,'PanelMoulding',x,y+d/2+.006,z,w-.035,h-.035,.012,wood,'Concept furniture panel border; hidden cross-section inferred')
            if p.get('motion_kind')=='SLIDE':
                knob(p,x,y+d/2+.008,z,brass)
            else:
                knob(p,x+(.10 if x>0 else -.10),y+d/2+.008,z+.10,brass)
    # Two broad under-table edge steps, visible in the concept. No micro-carving.
    for name in ('CoffeeTable','WindowConsole'):
        p=root(name);top=next(o for o in p.children if o.name.startswith('TopEnvelope'))
        x,y,z=top.location;w,d,h=top.dimensions
        for side in (-1,1):
            strip(p,'TableEdgeLong',(x,y+side*(d/2-.014),z-h/2-.008),(w-.018,.021,.024),wood,'Concept stepped wooden tabletop perimeter')
            strip(p,'TableEdgeEnd',(x+side*(w/2-.014),y,z-h/2-.008),(.021,d-.06,.024),wood,'Concept stepped wooden tabletop perimeter')
    bpy.context.view_layer.update()
    errors=[n for n,h in before.items() if not bpy.data.objects.get(n) or signature(bpy.data.objects[n])!=h]
    if errors:
        raise RuntimeError('Approved structure modified: '+str(errors))
    (OUT/'preservation.json').write_text(json.dumps({'unchanged_existing_objects':len(before),'errors':errors,'signatures':before},indent=2),encoding='utf-8')
    (OUT/'Acceptance.json').write_text(json.dumps({'level_2':'USER_APPROVED','approval':'确认通过','level_3':'AWAITING_VISUAL_REVIEW','gameplay_camera':'UE camera not available; Blender concept-distance proxy only'},indent=2),encoding='utf-8')
    bpy.ops.wm.save_as_mainfile(filepath=str(OUT/'LivingRoom_Stage03.blend'))
    print('Stage03 saved; preserved objects:',len(before),'new detail meshes:',sum(o.get('level')==3 for o in bpy.context.scene.objects))

build()
