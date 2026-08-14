/**
 * @file LRMovementRules.h
 * @brief 提供移动纯规则：状态×步态合法性矩阵、默认步态、步态×环境脚步噪声解析、噪声环境优先级与室内奔跑房间警戒目标值，供运行时组件与 LostRunic.Movement 自动化测试共同调用。
 *
 * 关联文件：LRMovementRules.cpp；所属领域：Gameplay。
 * 设计依据：Docs/Design/01_GameDesignSummary.md 与 Docs/Technical/04_TechnicalDesign.md。
 * 除带 EditDefaultsOnly、EditAnywhere 或 EditInstanceOnly 的字段外，其余成员均为运行时状态，不应由蓝图直接改写。
 */
#pragma once

#include "Core/LRTypes.h"
#include "GameplayTagContainer.h"

class ULRGuardTuning;
class ULRMovementTuning;

/** 该公开类型定义本文件领域边界的数据或行为；具体字段、参数与约束见下方中文注释。 */
struct LOSTRUNIC_API FLRNoiseResolution
{
	/** Radius 的空间值 `Radius`；距离和位置使用 Unreal 厘米单位。 C++ 安全默认值为 `0.0f`。 */
	float Radius = 0.0f;
	/** Tag 的领域数据，由所属类型负责维护和校验。  */
	FGameplayTag Tag;
};

namespace LRMovementRules
{
	/**
	 * @brief 判断 Is Pace Allowed 对应条件；Normal 全步态，Perception 仅潜行，Courage 走路+奔跑，Memory 仅走路。
	 * @param mode 本次操作使用的 `mode` 枚举或模式值。
	 * @param pace 本次操作使用的 `pace` 枚举或模式值。
	 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
	 */
	LOSTRUNIC_API bool IsPaceAllowed(ELRPerceptionMode mode, ELRMovementPace pace);
	/**
	 * @brief 查询 Get Default Pace 对应条件；进入状态时强制应用。
	 * @param mode 本次操作使用的 `mode` 枚举或模式值。
	 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
	 */
	LOSTRUNIC_API ELRMovementPace GetDefaultPace(ELRPerceptionMode mode);
	/**
	 * @brief 按步态×环境解析脚步噪声；潜行无声（半径 0 + Sneak 标签，仅供动画/表现），室内奔跑返回 Run.Indoor 标签（房间传播在组件层处理，此半径仅作无房间兜底）。
	 * @param pace 本次操作使用的 `pace` 枚举或模式值。
	 * @param environment 本次操作使用的 `environment` 枚举或模式值。
	 * @param tuning 数据或调优来源 `tuning`；调用期间只读，并按稳定 ID 解析内容。
	 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
	 */
	LOSTRUNIC_API FLRNoiseResolution ResolveFootstepNoise(ELRMovementPace pace, ELRNoiseEnvironment environment,
		const ULRMovementTuning& tuning);
	/**
	 * @brief 按固定优先级从重叠集合解析环境：Indoor > OutdoorStealth > Outdoor；空集合默认 Outdoor。
	 * @param environments 调用方提供的 `environments`，只在本次操作范围内使用。
	 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
	 */
	LOSTRUNIC_API ELRNoiseEnvironment ResolveEnvironmentFromSet(const TArray<ELRNoiseEnvironment>& environments);
	/**
	 * @brief 解析室内奔跑的房间警戒目标值：当前房间 max(当前警戒, RoomRunAlertLevel)；相邻房间 max(当前警戒, 当前警戒+AdjacentRoomRunAlertAmount)。多房间候选取最大由调用方完成，不在此累加。
	 * @param bCurrentRoom 布尔开关 `bCurrentRoom`；true 表示启用或条件成立，false 表示禁用或条件不成立。
	 * @param currentAlert 本次操作使用的计数、增量或索引 `currentAlert`；由函数校验合法范围。
	 * @param tuning 数据或调优来源 `tuning`；调用期间只读，并按稳定 ID 解析内容。
	 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
	 */
	LOSTRUNIC_API int32 ResolveRoomRunTargetLevel(bool bCurrentRoom, int32 currentAlert,
		const ULRGuardTuning& tuning);
}
