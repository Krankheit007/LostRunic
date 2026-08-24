/**
 * @file LRSaveRules.cpp
 * @brief 实现一个自动槽、十个手动槽、版本迁移、不可变快照、FIFO 异步写入，以及死亡进入 Memory 和返回恢复锚点的 A/B 关键事务。
 *
 * 关联文件：LRSaveRules.h；所属领域：Save。
 * 设计依据：Docs/Technical/08_ArchitectureBoundaries.md。
 * 除带 EditDefaultsOnly、EditAnywhere 或 EditInstanceOnly 的字段外，其余成员均为运行时状态，不应由蓝图直接改写。
 */
#include "Save/LRSaveRules.h"

#include "Save/LRSaveTypes.h"

bool LRSaveRules::IsProtectedOverwrite(const FLRSaveSlotId& slotId)
{
	return slotId.Type == ELRSaveSlotType::Auto;
}

bool LRSaveRules::ResolveContinueCandidate(const TArray<FLRSaveSlotMetadata>& slots,
	FLRSaveSlotId& outSlotId)
{
	const FLRSaveSlotMetadata* candidate = nullptr;
	for (const FLRSaveSlotMetadata& slot : slots)
	{
		if (slot.Health != ELRSaveSlotHealth::Healthy)
		{
			continue;
		}
		if (!candidate || slot.SavedAtUtc > candidate->SavedAtUtc
			|| (slot.SavedAtUtc == candidate->SavedAtUtc && slot.SaveSequence > candidate->SaveSequence))
		{
			candidate = &slot;
		}
	}
	if (!candidate)
	{
		outSlotId = FLRSaveSlotId();
		return false;
	}
	outSlotId = candidate->SlotId;
	return true;
}

bool LRSaveRules::CanContinue(const TArray<FLRSaveSlotMetadata>& slots)
{
	FLRSaveSlotId candidate;
	return ResolveContinueCandidate(slots, candidate);
}

/**
 * @brief 根据当前领域状态构建 Make Slot Name 所需的数据，不把临时对象作为长期存档标识。
 * @param slotType 本次操作使用的 `slotType` 枚举或模式值。
 * @param manualSlotIndex 本次操作使用的计数、增量或索引 `manualSlotIndex`；由函数校验合法范围。
 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
 */
/**
 * @brief 判断 Is Manual Slot Valid 对应条件；不产生玩法副作用。
 * @param manualSlotIndex 本次操作使用的计数、增量或索引 `manualSlotIndex`；由函数校验合法范围。
 * @param manualSlotCount 本次操作使用的计数、增量或索引 `manualSlotCount`；由函数校验合法范围。
 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
 */
/**
 * @brief 判断 Is Manual Save Allowed 对应条件；不产生玩法副作用。
 * @param phase 本次操作使用的 `phase` 枚举或模式值。
 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
 */
bool LRSaveRules::IsManualSaveAllowed(const bool bInMemoryMap, const bool bWorldPaused)
{
	return !bInMemoryMap && bWorldPaused;
}

/**
 * @brief 执行 Resolve After Write 的纯规则或事务判定，失败时提供结构化原因。
 * @param phase 本次操作使用的 `phase` 枚举或模式值。
 * @param writeKind 本次操作使用的 `writeKind` 枚举或模式值。
 * @param bSuccess 布尔开关 `bSuccess`；true 表示启用或条件成立，false 表示禁用或条件不成立。
 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
 */
