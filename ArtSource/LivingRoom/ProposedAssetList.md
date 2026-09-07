# LivingRoom — Proposed Asset List

来源：`D:/GameDesign/DontForgetAdele/livingroom.png`。范围与尺度方向已于首次方案获用户确认；本清单将既有范围正式化，不代表所有项目已完成。用户已修正一级工程并授权进入二级；最新逐资产状态见 `Stage02/Manifest.json` 和 `Stage02/Review.md`。

下表保留最初 Proposed 清单供追溯。实际位置、建筑范围和尺寸以用户修正的 `Approved/LivingRoom_Level01_UserApproved.blend` 为准；二级已完成主要家具结构、软包、柜门/抽屉及运动轴心，待结构验收。

二级漏项补充：壁炉、茶几、两处圆桌上的花瓶/花束/烛台/器物/杯碟主体现已补齐，现有21本书已具备封面、书页块及书脊，新增道具分类和装配关系见 `Stage02/Manifest.json` 的 `independent_props`。Decoration 是资产类别，不代表整体留待三级。

流程：`Docs/Art/02_ConceptDrivenBlenderWorkflow.md`。尺度是单图推定；房间参考包络为 7×6×3.4 m。Hero 表示重点制作，Reusable 表示可复用资产，Decoration 表示陈设角色；这些标签不决定静态或动画类型。

下表尺寸以米计，按局部宽×深×高或直径×高描述。准确的当前世界坐标包络与原点见 `Stage01/manifest.json`，旋转后的世界包络不能误用作局部尺寸。所有本轮成品都只是一级体块。

| 资产 ID / 图中对象 | 角色 | 建议尺寸 | 复杂度 | 静态/动画分类 | 当前阶段 |
| --- | --- | --- | --- | --- | --- |
| Sofa / 绿沙发 | Hero | 2.20×0.93×1.02 | 中 | 静态 | 一级 |
| ArmchairOchre / 赭黄扶手椅 | Hero | 0.88×0.86×1.08 | 中 | 静态 | 一级 |
| ArmchairRusset / 红褐扶手椅 | Hero | 0.80×0.82×1.00 | 中 | 静态 | 一级 |
| CoffeeTable / 茶几 | Hero、Reusable | 1.25×0.65×0.48 | 中 | 静态 | 一级 |
| ArchBookcase / 拱顶书柜 | Hero | 0.80×0.38×2.59 | 中 | 主体静态；底柜门后续独立、铰链轴心 | 一级 |
| Fireplace / 壁炉 | Hero | 1.85×0.78×1.56 | 中 | 静态；火焰交由 UE 表现 | 一级 |
| WindowConsole / 窗下长桌 | Reusable | 1.80×0.48×0.83 | 中 | 静态主体；确认可见抽屉结构后预留滑轨轴心 | 一级 |
| Ottoman / 脚凳 | Reusable | 0.72×0.65×0.54 | 低至中 | 静态 | 一级 |
| WoodChair / 木椅 | Reusable | 0.47×0.48×1.03 | 中 | 静态 | 一级 |
| SmallCabinet / 左下小柜 | Reusable | 0.76×0.51×0.91 | 中 | 静态主体；可动面板在二级识别 | 一级 |
| SideTableCup / 椅旁圆桌 | Reusable | 直径0.47×0.56 | 低至中 | 静态 | 一级 |
| SideTableLamp / 沙发旁圆桌 | Reusable | 直径0.49×0.59 | 低至中 | 静态 | 一级 |
| SideTablePlant / 右墙圆桌 | Reusable | 直径0.58×0.70 | 低至中 | 静态 | 一级 |
| Rug / 地毯 | Decoration | 3.85×4.12 | 低几何、选择性图案 | 静态 | 一级 |
| CoatStand / 衣帽架与挂衣 | Reusable、Decoration | 高约1.8 | 中 | 静态 | 一级 |
| LogBasket / 柴篮 | Decoration | 0.42×0.35×0.44 | 低至中 | 静态 | 一级包络 |
| FireTools / 炉具架 | Decoration | 0.31×0.25×0.50 | 低 | 静态 | 一级包络 |
| Window / 大窗 | Reusable | 开口宽2.30、高2.40、窗台0.78 | 中 | 静态；本轮不推定开窗动画 | 一级 |
| Curtains / 窗帘 | Decoration | 每侧宽约0.34、高3.22 | 中 | 静态垂搭，无布料模拟 | 一级包络 |
| PaintingMantel / 壁炉上画 | Decoration | 1.20×1.50 | 低 | 静态 | 一级平面 |
| PaintingBackSmall / 书柜右画 | Decoration | 0.45×0.68 | 低 | 静态 | 一级平面 |
| PaintingCorner / 窗左侧画 | Decoration | 0.49×0.65 | 低 | 静态 | 一级，遮挡已修正 |
| PaintingRight / 右墙画 | Decoration | 0.67×0.91 | 低 | 静态 | 一级平面 |
| PaintingLeft / 左侧柜上画 | Decoration | 0.56×0.78 | 低 | 静态 | 一级；剖切视图隐藏 |
| Palm / 窗边高盆栽 | Decoration | 总高约1.55 | 中 | 静态，无叶片动画 | 一级冠幅占位 |
| ForegroundPlant / 前景盆栽 | Decoration | 总高约1.15 | 中 | 静态 | 一级冠幅占位 |
| ConsolePlant / 长桌盆栽 | Decoration | 桌上高约0.63 | 低至中 | 静态 | 一级冠幅占位 |
| CabinetPlant / 小柜盆栽 | Decoration | 柜上高约0.55 | 低至中 | 静态 | 一级冠幅占位 |
| ConsoleLamp / 长桌台灯 | Reusable、Decoration | 高0.52 | 中 | 灯具静态；明灭交 UE | 一级 |
| CabinetLamp / 小柜台灯 | Reusable、Decoration | 高0.52 | 中 | 灯具静态；明灭交 UE | 一级 |
| SofaLamp / 沙发旁灯 | Decoration | 高0.39 | 中 | 灯具静态；明灭交 UE | 一级 |
| WallLampLeft / 左壁灯 | Reusable、Decoration | 高0.38 | 低至中 | 灯具静态；明灭交 UE | 一级 |
| WallLampRight / 右壁灯 | Reusable、Decoration | 高0.38 | 低至中 | 灯具静态；明灭交 UE | 一级 |

## 已在范围内、尚未建立独立资产的内容

| 内容 | 角色 / 分类 | 尺度来源与后续阶段 |
| --- | --- | --- |
| 坐垫、靠垫、披毯 | Decoration，静态 | 由座椅包络推导，在二级建立主要体积和垂搭，不提前绘制细褶 |
| 门扇、门框、墙板、地板模块 | Reusable，门扇可动 | 门洞暂约1×2.36m；在二级确定可见结构；不扩展原图之外的房间 |
| 书籍、杯碟、花瓶与花束 | Decoration，静态 | 依据图中桌面比例，约0.05–0.40m；可复用集合，不能用任意道具替代 |
| 壁炉烛台、钟饰、书柜雕像 | Decoration，静态 | 仅保留可辨认外形，大体积二级、少量明确装饰三级；模糊部分进入 Assumptions |
| 扶手卷边、弯腿、框架、柜门、抽屉 | 所属家具结构 | 在二级归属到对应家具，不能靠三级细节补轮廓错误 |

每件家具最终独立 Binary FBX；同款可复用网格，实例摆放不能烘死进唯一家具坐标。桌面道具默认独立复用，座椅自带软装按资产装配需求拆分。最终分类、层级和动画契约会随二级结构确认后写入 Manifest。

当前不运行骨骼动画或自动开合，不宣称柜体内部/背部结构有图像证据。独立假设见 `Assumptions.md`。
