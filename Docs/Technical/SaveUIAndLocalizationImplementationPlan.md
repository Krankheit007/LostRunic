# 主菜单、存档 UI 与本地化实施计划

## 目标

在现有 Save V2、UMG Screen 与 Enhanced Input 架构上，完成附件计划要求的 Catalog 生命周期门禁、UTC/元数据兼容、FocusTarget、存档 UI 父类与 Controller 所有权、主菜单前端、本地化 resolver 和定向自动化测试。

## 现有边界

- `ULRSaveSubsystem` 已拥有 A/B Catalog、异步操作队列和 Memory 事务；本次扩展不新建第二套存档系统。
- `ULRSaveWidgetController` 继续作为 HUD 所有的 UObject；Widget 只绑定/解绑委托。
- 纯规则放入 Save/UI helper，磁盘读取只保留在启动恢复和正式操作路径。
- WBP Designer、String Table、Localization Target、地图 World Settings 与 Home Anchor 作为资产装配项登记在 `Docs/Technical/06_BlueprintConfigurationGuide.md`。

## 实施任务

1. 测试先行：新增 Catalog 状态、非 Ready 查询、Continue resolver、FocusTarget、UTC 格式化和 fixture 兼容测试；先确认新增测试因接口缺失而失败。
2. Catalog 生命周期：增加 `ELRSaveCatalogState`、状态/快照委托、已验证内存快照查询、启动恢复状态转换和用户请求 `RejectedBusy` 门禁。
3. Metadata/时间：先冻结当前 V1 真实二进制 fixture，再增加 `CollectedCount`；实现 UTC 写入及固定时区/culture 格式化纯函数。
4. UI 模型：增加 FocusTarget/ConfirmViewModel、CreateDisplayIndex、健康原因映射和 Controller 操作身份校验；确保 Host teardown 后异步回调无效。
5. Widget/输入：增加五个专用父类、`IA_LRUIDelete`/`UIDeleteAction`/`ELRUICommand::Delete`，并为正式 WBP 记录 BindWidget 契约。
6. 前端：增加 `ALRMainMenuGameMode`、`ALRMainMenuHUD`，接入 Continue/NewGame/Load/Quit 和 Catalog Ready 状态。
7. Content/Localization：增加 `UIStringTable`、map display key、`ResolveUIText`、累计时长、收藏品和 UTC 展示格式 resolver，并更新蓝图配置指南。
8. 验证：构建 `LostRunicEditor`，运行 `LostRunic.Save`/`LostRunic.UI`/`LostRunic.Input` 定向测试，并登记仍需 Unreal Editor 手工完成的资产步骤。

## 约束

- 不编辑 `Binaries/`、`Intermediate/`、`DerivedDataCache/` 或 `Saved/`。
- 不重置现有未提交 WBP 修改。
- 代码遵循现有模块边界和项目级日志类别；不增加无必要 Tick 或防御性测试。
- 任何无法由当前沙箱完成的 UAsset/Localization 配置，必须在配置指南中明确记录。
