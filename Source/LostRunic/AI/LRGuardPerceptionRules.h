/**
 * @file LRGuardPerceptionRules.h
 * @brief 实现“家”垂直切片的守卫感知、0-11 警戒值、StateTree 行为切换、调查追逐与捕获死亡流程。规则层只计算状态，Controller 负责接入 UE 感知、导航和计时器。
 *
 * 关联文件：LRGuardPerceptionRules.cpp；所属领域：AI。
 * 设计依据：Docs/Technical/08_ArchitectureBoundaries.md。
 * 除带 EditDefaultsOnly、EditAnywhere 或 EditInstanceOnly 的字段外，其余成员均为运行时状态，不应由蓝图直接改写。
 */
#pragma once

#include "AI/LRGuardTypes.h"
#include "GameplayTagContainer.h"

class ULRGuardTuning;

/** 该公开类型定义本文件领域边界的数据或行为；具体字段、参数与约束见下方中文注释。 */
struct LOSTRUNIC_API FLRNoiseResponse
{
	/** Respond 的开关；true 表示启用，false 表示禁用。 C++ 安全默认值为 `false`。 */
	bool bRespond = false;
	/** Delta 的领域数据，由所属类型负责维护和校验。 C++ 安全默认值为 `0`。 */
	int32 Delta = 0;
	/** Is Attract 的开关；true 表示启用，false 表示禁用。 C++ 安全默认值为 `false`。 */
	bool bIsAttract = false;
};

namespace LRGuardPerceptionRules
{
	/** Resolves the continuous sight score after all binary gates have passed. */
	LOSTRUNIC_API FLRGuardVisibilityResult EvaluateVisibility(float distance, float forwardDot,
		bool bHasValidContact, bool bHasLineOfSight, bool bHardVisibility,
		float movementFactor, float exposureFactor, float lightingFactor, float postureFactor,
		const ULRGuardTuning& tuning);
	/** Applies the approved linear distance curve inside SightRadius. */
	LOSTRUNIC_API float ResolveDistanceFactor(float distance, const ULRGuardTuning& tuning);
	/** Recomputes a score from a sample without changing any state. */
	LOSTRUNIC_API float CalculateVisibilityScore(const FLRGuardVisibilityResult& sample);
	/** Resolves the stage from effective exposure seconds without emitting transition events. */
	LOSTRUNIC_API ELRGuardDetectionStage ResolveDetectionStage(float effectiveExposureSeconds,
		const ULRGuardTuning& tuning);
	/** Returns whether a pending investigation snapshot contains a new location or stimulus context. */
	LOSTRUNIC_API bool HasNewInvestigationContext(const FLRGuardKnowledgeSnapshot& previousSnapshot,
		const FLRGuardKnowledgeSnapshot& currentSnapshot);
	/** Applies score-weighted exposure integration or configured decay, with a bounded delta. */
	LOSTRUNIC_API float IntegrateDetectionExposure(float currentExposureSeconds,
		const FLRGuardVisibilityResult& sample, float deltaSeconds, const ULRGuardTuning& tuning);
	LOSTRUNIC_API float DecayDetectionExposure(float currentExposureSeconds, float deltaSeconds,
		const ULRGuardTuning& tuning);

	LOSTRUNIC_API bool CanConfirmSight(float distance, float forwardDot, bool bOccluded, bool bHidden,
		const ULRGuardTuning& tuning);
	/**
	 * @brief 判断 Can Hear 对应条件；不产生玩法副作用。
	 * @param distance 空间值 `distance`；距离和位置使用 Unreal 厘米单位。
	 * @param sourceRadius 空间值 `sourceRadius`；距离和位置使用 Unreal 厘米单位。
	 * @param tuning 数据或调优来源 `tuning`；调用期间只读，并按稳定 ID 解析内容。
	 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
	 */
	LOSTRUNIC_API bool CanHear(float distance, float sourceRadius, const ULRGuardTuning& tuning);
	/**
	 * @brief 按噪声原因标签解析守卫应做的警戒响应；CD 与观察时序由调用方组件执行，本函数只做语义映射。
	 * @param reason 噪声原因 Gameplay Tag，例如 Noise.Footstep.Walk 或 Noise.Footstep.Run.Indoor。
	 * @param currentAlert 守卫当前警戒值 0-11。
	 * @param tuning 数据或调优来源 `tuning`；调用期间只读，并按稳定 ID 解析内容。
	 * @return 结构化响应：是否响应、Delta 与是否走吸引语义（IsAttract 时调用方使用带 CD 门控的 ApplyAttract）。
	 */
	LOSTRUNIC_API FLRNoiseResponse ResolveNoiseAlertDelta(FGameplayTag reason, int32 currentAlert,
		const ULRGuardTuning& tuning);
}
