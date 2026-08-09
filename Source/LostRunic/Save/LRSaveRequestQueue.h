/**
 * @file LRSaveRequestQueue.h
 * @brief 实现一个自动槽、十个手动槽、版本迁移、不可变快照、FIFO 异步写入，以及死亡进入 Memory 和返回恢复锚点的 A/B 关键事务。
 *
 * 关联文件：LRSaveRequestQueue.cpp；所属领域：Save。
 * 设计依据：Docs/Design/01_GameDesignSummary.md 与 Docs/Technical/04_TechnicalDesign.md。
 * 除带 EditDefaultsOnly、EditAnywhere 或 EditInstanceOnly 的字段外，其余成员均为运行时状态，不应由蓝图直接改写。
 */
#pragma once

#include "Save/LRSaveTypes.h"

/** 该公开类型定义本文件领域边界的数据或行为；具体字段、参数与约束见下方中文注释。 */
namespace LRSaveRequestQueue
{
	/**
	 * @brief 按既定顺序将 Enqueue 请求加入队列，保留调用时快照。
	 * @param queue 本次领域操作的结构化数据 `queue`；字段语义由对应 USTRUCT 定义。
	 * @param request 不可变领域请求，包含本次操作所需的稳定 ID、来源、目标或原因。
	 */
	LOSTRUNIC_API void Enqueue(TArray<FLRQueuedSaveRequest>& queue, FLRQueuedSaveRequest&& request);
	/**
	 * @brief 从 FIFO 队首移动出一个不可变存档请求；空队列返回 false。
	 * @param queue 本次领域操作的结构化数据 `queue`；字段语义由对应 USTRUCT 定义。
	 * @param outRequest 本次领域操作的结构化数据 `outRequest`；字段语义由对应 USTRUCT 定义。
	 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
	 */
	LOSTRUNIC_API bool Dequeue(TArray<FLRQueuedSaveRequest>& queue, FLRQueuedSaveRequest& outRequest);
}
