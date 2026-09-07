"""Reuse stage02 review utilities with isolated stage03 output and expanded checks."""
from pathlib import Path
p=Path('D:/25DGame/LostRunic/Tools/Blender/LivingRoom/review_stage02.py')
code=p.read_text(encoding='utf-8').replace("OUT=BASE/'Stage02'","OUT=BASE/'Stage03'")
code=code.replace("obj.get('level')!=2","obj.get('level') not in (2,3)")
code=code.replace("'level':2","'level':3")
code=code.replace("'level_2':'AWAITING_USER_STRUCTURE_REVIEW'","'level_2':'USER_APPROVED', 'level_3':'AWAITING_USER_DETAIL_REVIEW', 'gameplay_camera':'UE not verified; Blender concept-distance proxy'")
code=code.replace("OUT/'LivingRoom_Stage02.blend'","OUT/'LivingRoom_Stage03.blend'")
exec(compile(code,str(p),'exec'))
