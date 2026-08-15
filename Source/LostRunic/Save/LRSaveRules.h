/**
 * @file LRSaveRules.h
 * @brief 实现一个自动槽、十个手动槽、版本迁移、不可变快照、FIFO 异步写入，以及死亡进入 Memory 和返回恢复锚点的 A/B 关键事务。
 *
 * 关联文件：LRSaveRules.cpp；所属领域：Save。
 * 设计依据：Docs/Design/01_GameDesignSummary.md 与 Docs/Technical/04_TechnicalDesign.md。
 * 除带 EditDefaultsOnly、EditAnywhere 或 EditInstanceOnly 的字段外，其余成员均为运行时状态，不应由蓝图直接改写。
 */
#pragma once

#include "Save/LRSaveTypes.h"

/** 该公开类型定义本文件领域边界的数据或行为；具体字段、参数与约束见下方中文注释。 */
namespace LRSaveRules
{
	/**
	 * @brief 根据当前领域状态构建 Make Slot Name 所需的数据，不把临时对象作为长期存档标识。
	 * @param slotType 本次操作使用的 `slotType` 枚举或模式值。
	 * @param manualSlotIndex 本次操作使用的计数、增量或索引 `manualSlotIndex`；由函数校验合法范围。
	 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
	 */
	LOSTRUNIC_API FString MakeSlotName(ELRSaveSlotType slotType, int32 manualSlotIndex = INDEX_NONE);
	/**
	 * @brief 判断 Is Manual Slot Valid 对应条件；不产生玩法副作用。
	 * @param manualSlotIndex 本次操作使用的计数、增量或索引 `manualSlotIndex`；由函数校验合法范围。
	 * @param manualSlotCount 本次操作使用的计数、增量或索引 `manualSlotCount`；由函数校验合法范围。
	 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
	 */
	LOSTRUNIC_API bool IsManualSlotValid(int32 manualSlotIndex, int32 manualSlotCount);
	/**
	 * @brief 判断 Is Manual Save Allowed 对应条件；不产生玩法副作用。
	 * @param phase 本次操作使用的 `phase` 枚举或模式值。
	 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
	 */
	LOSTRUNIC_API bool IsManualSaveAllowed(ELRMemoryTransactionPhase phase, bool bWorldPaused);
	/**
	 * @brief 判断 Can Begin Memory Transaction 对应条件；不产生玩法副作用。
	 * @param phase 本次操作使用的 `phase` 枚举或模式值。
	 * @param anchor 调用方提供的 `anchor`，只在本次操作范围内使用。
	 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
	 */
	LOSTRUNIC_API bool CanBeginMemoryTransaction(ELRMemoryTransactionPhase phase, const FLRResumeAnchor& anchor);
	/**
	 * @brief 判断 Is Memory Entry World 对应条件；不产生玩法副作用。
	 * @param phase 本次操作使用的 `phase` 枚举或模式值。
	 * @param currentMapId 稳定标识 `currentMapId`；用于内容查询和存档，不依赖显示名或数组序号。
	 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
	 */
	LOSTRUNIC_API bool IsMemoryEntryWorld(ELRMemoryTransactionPhase phase, FName currentMapId);
	/**
	 * @brief 判断 Is Resume World 对应条件；不产生玩法副作用。
	 * @param phase 本次操作使用的 `phase` 枚举或模式值。
	 * @param currentMapId 稳定标识 `currentMapId`；用于内容查询和存档，不依赖显示名或数组序号。
	 * @param anchor 调用方提供的 `anchor`，只在本次操作范围内使用。
	 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
	 */
	LOSTRUNIC_API bool IsResumeWorld(ELRMemoryTransactionPhase phase, FName currentMapId, const FLRResumeAnchor& anchor);
	LOSTRUNIC_API ELRMemoryTransactionPhase ResolveAfterWrite(ELRMemoryTransactionPhase phase,
		ELRSaveWriteKind writeKind, bool bSuccess);
}
