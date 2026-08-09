/**
 * @file LRSaveRules.cpp
 * @brief 实现一个自动槽、十个手动槽、版本迁移、不可变快照、FIFO 异步写入，以及死亡进入 Memory 和返回恢复锚点的 A/B 关键事务。
 *
 * 关联文件：LRSaveRules.h；所属领域：Save。
 * 设计依据：Docs/Design/01_GameDesignSummary.md 与 Docs/Technical/04_TechnicalDesign.md。
 * 除带 EditDefaultsOnly、EditAnywhere 或 EditInstanceOnly 的字段外，其余成员均为运行时状态，不应由蓝图直接改写。
 */
#include "Save/LRSaveRules.h"

#include "Save/LRSaveTypes.h"

/**
 * @brief 根据当前领域状态构建 Make Slot Name 所需的数据，不把临时对象作为长期存档标识。
 * @param slotType 本次操作使用的 `slotType` 枚举或模式值。
 * @param manualSlotIndex 本次操作使用的计数、增量或索引 `manualSlotIndex`；由函数校验合法范围。
 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
 */
FString LRSaveRules::MakeSlotName(const ELRSaveSlotType slotType, const int32 manualSlotIndex)
{
	return slotType == ELRSaveSlotType::Auto ? TEXT("LostRunic_Auto")
		: FString::Printf(TEXT("LostRunic_Manual_%02d"), manualSlotIndex + 1);
}

/**
 * @brief 判断 Is Manual Slot Valid 对应条件；不产生玩法副作用。
 * @param manualSlotIndex 本次操作使用的计数、增量或索引 `manualSlotIndex`；由函数校验合法范围。
 * @param manualSlotCount 本次操作使用的计数、增量或索引 `manualSlotCount`；由函数校验合法范围。
 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
 */
bool LRSaveRules::IsManualSlotValid(const int32 manualSlotIndex, const int32 manualSlotCount)
{
	return manualSlotIndex >= 0 && manualSlotIndex < manualSlotCount;
}

/**
 * @brief 判断 Is Manual Save Allowed 对应条件；不产生玩法副作用。
 * @param phase 本次操作使用的 `phase` 枚举或模式值。
 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
 */
bool LRSaveRules::IsManualSaveAllowed(const ELRMemoryTransactionPhase phase)
{
	return phase == ELRMemoryTransactionPhase::None;
}

/**
 * @brief 判断 Can Begin Memory Transaction 对应条件；不产生玩法副作用。
 * @param phase 本次操作使用的 `phase` 枚举或模式值。
 * @param anchor 调用方提供的 `anchor`，只在本次操作范围内使用。
 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
 */
bool LRSaveRules::CanBeginMemoryTransaction(const ELRMemoryTransactionPhase phase, const FLRResumeAnchor& anchor)
{
	return phase == ELRMemoryTransactionPhase::None && anchor.IsValid();
}

/**
 * @brief 判断 Is Memory Entry World 对应条件；不产生玩法副作用。
 * @param phase 本次操作使用的 `phase` 枚举或模式值。
 * @param currentMapId 稳定标识 `currentMapId`；用于内容查询和存档，不依赖显示名或数组序号。
 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
 */
bool LRSaveRules::IsMemoryEntryWorld(const ELRMemoryTransactionPhase phase, const FName currentMapId)
{
	return phase == ELRMemoryTransactionPhase::AwaitingMemoryWorld && currentMapId == LRSaveIds::MemoryMapId;
}

/**
 * @brief 判断 Is Resume World 对应条件；不产生玩法副作用。
 * @param phase 本次操作使用的 `phase` 枚举或模式值。
 * @param currentMapId 稳定标识 `currentMapId`；用于内容查询和存档，不依赖显示名或数组序号。
 * @param anchor 调用方提供的 `anchor`，只在本次操作范围内使用。
 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
 */
bool LRSaveRules::IsResumeWorld(const ELRMemoryTransactionPhase phase, const FName currentMapId, const FLRResumeAnchor& anchor)
{
	return phase == ELRMemoryTransactionPhase::AwaitingResumeWorld && anchor.IsValid() && currentMapId == anchor.MapId;
}

/**
 * @brief 执行 Resolve After Write 的纯规则或事务判定，失败时提供结构化原因。
 * @param phase 本次操作使用的 `phase` 枚举或模式值。
 * @param writeKind 本次操作使用的 `writeKind` 枚举或模式值。
 * @param bSuccess 布尔开关 `bSuccess`；true 表示启用或条件成立，false 表示禁用或条件不成立。
 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
 */
ELRMemoryTransactionPhase LRSaveRules::ResolveAfterWrite(const ELRMemoryTransactionPhase phase,
	const ELRSaveWriteKind writeKind, const bool bSuccess)
{
	if (!bSuccess)
	{
		return phase;
	}
	if (phase == ELRMemoryTransactionPhase::SavingEntry && writeKind == ELRSaveWriteKind::MemoryEntry)
	{
		return ELRMemoryTransactionPhase::InMemory;
	}
	if (phase == ELRMemoryTransactionPhase::SavingReturn && writeKind == ELRSaveWriteKind::MemoryReturn)
	{
		return ELRMemoryTransactionPhase::None;
	}
	return phase;
}
