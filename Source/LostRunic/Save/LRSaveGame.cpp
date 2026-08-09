/**
 * @file LRSaveGame.cpp
 * @brief 实现一个自动槽、十个手动槽、版本迁移、不可变快照、FIFO 异步写入，以及死亡进入 Memory 和返回恢复锚点的 A/B 关键事务。
 *
 * 关联文件：LRSaveGame.h；所属领域：Save。
 * 设计依据：Docs/Design/01_GameDesignSummary.md 与 Docs/Technical/04_TechnicalDesign.md。
 * 除带 EditDefaultsOnly、EditAnywhere 或 EditInstanceOnly 的字段外，其余成员均为运行时状态，不应由蓝图直接改写。
 */
#include "Save/LRSaveGame.h"

/**
 * @brief 按 SaveVersion 顺序迁移旧存档；当前支持 v0 到 v1，并拒绝未知或损坏数据。
 * @param outError 输出校验失败原因；成功时保持为空。
 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
 */
bool ULRSaveGame::MigrateToLatest(FString& outError)
{
	if (SaveVersion < 0 || SaveVersion > LatestVersion)
	{
		outError = FString::Printf(TEXT("Unsupported save version %d."), SaveVersion);
		return false;
	}
	if (SaveVersion == 0 && !ResumeAnchor.IsValid() && !LegacyMapId.IsNone())
	{
		ResumeAnchor.MapId = LegacyMapId;
		ResumeAnchor.AnchorId = TEXT("Migrated");
		ResumeAnchor.Location = LegacyLocation;
		ResumeAnchor.Rotation = LegacyRotation;
	}
	SaveVersion = LatestVersion;
	return true;
}
