/**
 * @file LRSaveTypes.cpp
 * @brief 声明可恢复锚点、库存/叙事分块、槽位类型、写入类型、请求结果与 Memory 事务阶段；磁盘字段使用稳定 FName ID。
 *
 * 关联文件：LRSaveTypes.h；所属领域：Save。
 * 设计依据：Docs/Design/01_GameDesignSummary.md 与 Docs/Technical/04_TechnicalDesign.md。
 * 除带 EditDefaultsOnly、EditAnywhere 或 EditInstanceOnly 的字段外，其余成员均为运行时状态，不应由蓝图直接改写。
 */
#include "Save/LRSaveTypes.h"

namespace LRSaveIds
{
	const FName AutoSlotReason(TEXT("Auto"));
	const FName MemoryEntryReason(TEXT("Memory.Entry"));
	const FName MemoryReturnReason(TEXT("Memory.Return"));
	const FName MemoryMapId(TEXT("Memory"));
}
