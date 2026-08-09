/**
 * @file LRSaveGame.h
 * @brief 实现一个自动槽、十个手动槽、版本迁移、不可变快照、FIFO 异步写入，以及死亡进入 Memory 和返回恢复锚点的 A/B 关键事务。
 *
 * 关联文件：LRSaveGame.cpp；所属领域：Save。
 * 设计依据：Docs/Design/01_GameDesignSummary.md 与 Docs/Technical/04_TechnicalDesign.md。
 * 除带 EditDefaultsOnly、EditAnywhere 或 EditInstanceOnly 的字段外，其余成员均为运行时状态，不应由蓝图直接改写。
 */
#pragma once

#include "GameFramework/SaveGame.h"
#include "Save/LRSaveTypes.h"

#include "LRSaveGame.generated.h"

/** 该公开类型定义本文件领域边界的数据或行为；具体字段、参数与约束见下方中文注释。 */
UCLASS(BlueprintType, meta = (DisplayName = "Lost Runic Save Game"))
class LOSTRUNIC_API ULRSaveGame : public USaveGame
{
	GENERATED_BODY()

public:
	static constexpr int32 LatestVersion = 1;

	/**
	 * @brief 按 SaveVersion 顺序迁移旧存档；当前支持 v0 到 v1，并拒绝未知或损坏数据。
	 * @param outError 输出校验失败原因；成功时保持为空。
	 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
	 */
	bool MigrateToLatest(FString& outError);

	/** Save Version 的领域数据，由所属类型负责维护和校验。 C++ 安全默认值为 `LatestVersion`。 蓝图可读取但不可写入。该字段参与存档序列化。 */
	UPROPERTY(BlueprintReadOnly, SaveGame, Category = "Save")
	int32 SaveVersion = LatestVersion;

	/** Last Saved Utc 的领域数据，由所属类型负责维护和校验。 蓝图可读取但不可写入。该字段参与存档序列化。 */
	UPROPERTY(BlueprintReadOnly, SaveGame, Category = "Save")
	FDateTime LastSavedUtc;

	/** 最近可安全恢复的地图、锚点、位置和朝向；Memory 写入不得覆盖。 蓝图可读取但不可写入。该字段参与存档序列化。 */
	UPROPERTY(BlueprintReadOnly, SaveGame, Category = "Resume")
	FLRResumeAnchor ResumeAnchor;

	/** Inventory 的领域数据，由所属类型负责维护和校验。 蓝图可读取但不可写入。该字段参与存档序列化。 */
	UPROPERTY(BlueprintReadOnly, SaveGame, Category = "Progress")
	FLRSaveInventoryChunk Inventory;

	/** Narrative 的领域数据，由所属类型负责维护和校验。 蓝图可读取但不可写入。该字段参与存档序列化。 */
	UPROPERTY(BlueprintReadOnly, SaveGame, Category = "Progress")
	FLRSaveNarrativeChunk Narrative;

	/** Legacy Map Id 的稳定 FName/GUID 标识；用于定义查询和存档，不依赖显示名或临时 Actor 名称。 C++ 安全默认值为 `NAME_None`。 该字段参与存档序列化。 */
	UPROPERTY(SaveGame, meta = (DeprecatedProperty, DeprecationMessage = "Migrated into ResumeAnchor in save version 1."))
	FName LegacyMapId = NAME_None;

	/** Legacy Location 的领域数据，由所属类型负责维护和校验。 C++ 安全默认值为 `FVector::ZeroVector`。 该字段参与存档序列化。 */
	UPROPERTY(SaveGame, meta = (DeprecatedProperty, DeprecationMessage = "Migrated into ResumeAnchor in save version 1."))
	FVector LegacyLocation = FVector::ZeroVector;

	/** Legacy Rotation 的领域数据，由所属类型负责维护和校验。 C++ 安全默认值为 `FRotator::ZeroRotator`。 该字段参与存档序列化。 */
	UPROPERTY(SaveGame, meta = (DeprecatedProperty, DeprecationMessage = "Migrated into ResumeAnchor in save version 1."))
	FRotator LegacyRotation = FRotator::ZeroRotator;
};
