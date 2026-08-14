/**
 * @file LRMovementRules.cpp
 * @brief 实现移动纯规则：状态×步态合法性矩阵、默认步态、步态×环境脚步噪声解析、噪声环境优先级与室内奔跑房间警戒目标值。
 *
 * 关联文件：LRMovementRules.h；所属领域：Gameplay。
 * 设计依据：Docs/Design/01_GameDesignSummary.md 与 Docs/Technical/04_TechnicalDesign.md。
 * 除带 EditDefaultsOnly、EditAnywhere 或 EditInstanceOnly 的字段外，其余成员均为运行时状态，不应由蓝图直接改写。
 */
#include "Gameplay/LRMovementRules.h"

#include "Core/LRGameplayTags.h"
#include "Data/LRGuardTuning.h"
#include "Data/LRMovementTuning.h"

/**
 * @brief 判断 Is Pace Allowed 对应条件；Normal 全步态，Perception 仅潜行，Courage 走路+奔跑，Memory 仅走路。
 * @param mode 本次操作使用的 `mode` 枚举或模式值。
 * @param pace 本次操作使用的 `pace` 枚举或模式值。
 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
 */
bool LRMovementRules::IsPaceAllowed(const ELRPerceptionMode mode, const ELRMovementPace pace)
{
	switch (mode)
	{
	case ELRPerceptionMode::Normal:
		return true;
	case ELRPerceptionMode::Perception:
		return pace == ELRMovementPace::Sneak;
	case ELRPerceptionMode::Courage:
		return pace != ELRMovementPace::Sneak;
	case ELRPerceptionMode::Memory:
		return pace == ELRMovementPace::Walk;
	}
	return false;
}

/**
 * @brief 查询 Get Default Pace 对应条件；进入状态时强制应用。
 * @param mode 本次操作使用的 `mode` 枚举或模式值。
 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
 */
ELRMovementPace LRMovementRules::GetDefaultPace(const ELRPerceptionMode mode)
{
	return mode == ELRPerceptionMode::Perception ? ELRMovementPace::Sneak : ELRMovementPace::Walk;
}

/**
 * @brief 按步态×环境解析脚步噪声；潜行无声（半径 0 + Sneak 标签，仅供动画/表现），室内奔跑返回 Run.Indoor 标签（房间传播在组件层处理，此半径仅作无房间兜底）。
 * @param pace 本次操作使用的 `pace` 枚举或模式值。
 * @param environment 本次操作使用的 `environment` 枚举或模式值。
 * @param tuning 数据或调优来源 `tuning`；调用期间只读，并按稳定 ID 解析内容。
 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
 */
FLRNoiseResolution LRMovementRules::ResolveFootstepNoise(const ELRMovementPace pace,
	const ELRNoiseEnvironment environment, const ULRMovementTuning& tuning)
{
	FLRNoiseResolution resolution;
	switch (pace)
	{
	case ELRMovementPace::Sneak:
		resolution.Radius = 0.0f;
		resolution.Tag = LRGameplayTags::NoiseFootstepSneak;
		return resolution;
	case ELRMovementPace::Run:
		if (environment == ELRNoiseEnvironment::Indoor)
		{
			resolution.Radius = tuning.IndoorRunNoiseRadius;
			resolution.Tag = LRGameplayTags::NoiseFootstepRunIndoor;
			return resolution;
		}
		resolution.Radius = environment == ELRNoiseEnvironment::OutdoorStealth
			? tuning.OutdoorStealthRunNoiseRadius : tuning.OutdoorNoiseRadius;
		resolution.Tag = LRGameplayTags::NoiseFootstepRun;
		return resolution;
	case ELRMovementPace::Walk:
		break;
	}

	resolution.Radius = environment == ELRNoiseEnvironment::Indoor
		? tuning.IndoorWalkNoiseRadius : tuning.OutdoorNoiseRadius;
	resolution.Tag = environment == ELRNoiseEnvironment::Outdoor
		? LRGameplayTags::NoiseFootstepWalkFaint : LRGameplayTags::NoiseFootstepWalk;
	return resolution;
}

/**
 * @brief 按固定优先级从重叠集合解析环境：Indoor > OutdoorStealth > Outdoor；空集合默认 Outdoor。
 * @param environments 调用方提供的 `environments`，只在本次操作范围内使用。
 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
 */
ELRNoiseEnvironment LRMovementRules::ResolveEnvironmentFromSet(const TArray<ELRNoiseEnvironment>& environments)
{
	ELRNoiseEnvironment resolved = ELRNoiseEnvironment::Outdoor;
	for (const ELRNoiseEnvironment environment : environments)
	{
		if (environment == ELRNoiseEnvironment::Indoor)
		{
			return ELRNoiseEnvironment::Indoor;
		}
		if (environment == ELRNoiseEnvironment::OutdoorStealth)
		{
			resolved = ELRNoiseEnvironment::OutdoorStealth;
		}
	}
	return resolved;
}

/**
 * @brief 解析室内奔跑的房间警戒目标值：当前房间 max(当前警戒, RoomRunAlertLevel)；相邻房间 max(当前警戒, 当前警戒+AdjacentRoomRunAlertAmount)。多房间候选取最大由调用方完成，不在此累加。
 * @param bCurrentRoom 布尔开关 `bCurrentRoom`；true 表示启用或条件成立，false 表示禁用或条件不成立。
 * @param currentAlert 本次操作使用的计数、增量或索引 `currentAlert`；由函数校验合法范围。
 * @param tuning 数据或调优来源 `tuning`；调用期间只读，并按稳定 ID 解析内容。
 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
 */
int32 LRMovementRules::ResolveRoomRunTargetLevel(const bool bCurrentRoom, const int32 currentAlert,
	const ULRGuardTuning& tuning)
{
	if (bCurrentRoom)
	{
		return FMath::Max(currentAlert, tuning.RoomRunAlertLevel);
	}
	return FMath::Max(currentAlert, currentAlert + tuning.AdjacentRoomRunAlertAmount);
}
