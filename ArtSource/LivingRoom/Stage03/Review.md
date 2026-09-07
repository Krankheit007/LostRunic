# 三级选择性细节验收

二级结构用户已回复“确认通过”。本次基于已通过的 Stage02 工程，经 Blender MCP 增量制作，另存 Stage03；不覆盖二级工程。

本次范围：壁炉柱脚、柱头简化浮雕框、纵向线脚与檐下层次；五幅画框内侧金属唇边；柜门、门板与抽屉的简化面板线脚及小把手；茶几和窗下长桌的桌沿分层。依据为 livingroom.png 中可见的大形与边线。原图无法辨认的雕花图案、书名、器物花纹不臆造。把手背面及线脚截面采用简单推断，继承已确认的运动父节点。

检查结果：既有 1002 个网格/空对象的父级、局部变换、网格顶点和面索引签名未变化；34 个资产根变换和 16 个墙地建筑几何哈希保持。新加 98 个细节网格，资产评估后三角面合计 165728，仅作诊断。声明闭合网格的非流形与退化面检查通过。书柜门及长桌抽屉试开图已渲染，工程恢复关闭。

Review_Comparison.png 为源图与 Blender 概念距离对照；Review_Details.png 为近景细节。没有读取或验证 UE 实际 Gameplay Camera，因此真实游戏镜头 Gate 为待验证，不能以这些预览宣称已通过。三级视觉结论仍待用户验收。

正式 Bevel + Normal、UV、BaseColor/SMK、FBX 与回读检查须在三级通过后继续。当前沿用结构预览圆角和材质，不宣称正式表面或 FBX 验收完成。

重建：先在 Blender 中通过 MCP 单独打开 Stage02/LivingRoom_Stage02.blend，再执行 Tools/Blender/LivingRoom/build_stage03.py；渲染与检查入口为 review_stage03.py，拼图入口为 assemble_stage03.py。
