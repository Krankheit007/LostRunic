/**
 * @file LRAttackTargetResolver.h
 * @brief 独立攻击目标筛选：只保留实现 ILRAttackTarget、满足攻击距离、朝向、遮挡和目标状态（免疫）的候选，不复用交互系统的当前焦点、目标接口或 HUD 提示语义。
 *
 * 关联文件：LRAttackTargetResolver.cpp；所属领域：Items。
 * 设计依据：Docs/Design/01_GameDesignSummary.md 与 Docs/Technical/04_TechnicalDesign.md。
 * 除带 EditDefaultsOnly、EditAnywhere 或 EditInstanceOnly 的字段外，其余成员均为运行时状态，不应由蓝图直接改写。
 */
#pragma once

#include "Components/ActorComponent.h"

#include "LRAttackTargetResolver.generated.h"

class ULRStateTuning;

/** 攻击候选的纯规则评分；距离为平方距离，facingDot 为朝向点积，避免开方与角度换算。 */
USTRUCT(BlueprintType, meta = (DisplayName = "Lost Runic Attack Candidate Score"))
struct LOSTRUNIC_API FLRAttackCandidateScore
{
	GENERATED_BODY()

	/** 与攻击者的平方距离；距离和位置使用 Unreal 厘米单位。 */
	UPROPERTY(BlueprintReadOnly, Category = "Attack")
	float DistanceSquared = 0.0f;

	/** 攻击者朝向量与指向目标向量的点积；接近 1 表示正前方。 */
	UPROPERTY(BlueprintReadOnly, Category = "Attack")
	float FacingDot = 0.0f;

	/** 是否位于攻击距离内。 */
	UPROPERTY(BlueprintReadOnly, Category = "Attack")
	bool bInRange = false;

	/** 是否位于朝向锥内。 */
	UPROPERTY(BlueprintReadOnly, Category = "Attack")
	bool bFacingAllowed = false;

	/** 是否未被世界几何遮挡。 */
	UPROPERTY(BlueprintReadOnly, Category = "Attack")
	bool bNotOccluded = false;

	/** 是否可被攻击（未免疫）。 */
	UPROPERTY(BlueprintReadOnly, Category = "Attack")
	bool bVulnerable = false;
};

namespace LRAttackTargetRules
{
	/**
	 * @brief 在合法攻击候选中选择最近的未免疫目标；全部失败时返回 INDEX_NONE。
	 * @param candidates 本次领域操作的结构化数据 `candidates`；字段语义由对应 USTRUCT 定义。
	 * @param tuning 数据或调优来源 `tuning`；调用期间只读，并按稳定 ID 解析内容。
	 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
	 */
	LOSTRUNIC_API int32 SelectBestTarget(const TArray<FLRAttackCandidateScore>& candidates,
		const ULRStateTuning& tuning);
	/**
	 * @brief 判断 Is Facing Allowed 对应条件；不产生玩法副作用。
	 * @param forwardDot 调用方提供的 `forwardDot`，只在本次操作范围内使用。
	 * @param tuning 数据或调优来源 `tuning`；调用期间只读，并按稳定 ID 解析内容。
	 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
	 */
	LOSTRUNIC_API bool IsFacingAllowed(float forwardDot, const ULRStateTuning& tuning);
	/**
	 * @brief 判断 Is In Range 对应条件；不产生玩法副作用。
	 * @param distanceSquared 空间值 `distanceSquared`；距离和位置使用 Unreal 厘米单位。
	 * @param tuning 数据或调优来源 `tuning`；调用期间只读，并按稳定 ID 解析内容。
	 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
	 */
	LOSTRUNIC_API bool IsInRange(float distanceSquared, const ULRStateTuning& tuning);
}

/** 该公开类型定义本文件领域边界的数据或行为；具体字段、参数与约束见下方中文注释。 */
UCLASS(ClassGroup = "Lost Runic", BlueprintType, meta = (BlueprintSpawnableComponent, DisplayName = "Lost Runic Attack Target Resolver"))
class LOSTRUNIC_API ULRAttackTargetResolver : public UActorComponent
{
	GENERATED_BODY()

public:
	/**
	 * @brief 创建对象并设置默认子对象、能力开关和安全初值；需要 World、资产或玩家的依赖延迟到初始化阶段解析。
	 */
	ULRAttackTargetResolver();

	/**
	 * @brief 扫描世界并返回唯一最近的合法攻击目标；未找到时返回 false 并给出原因标签。
	 * @param instigator 攻击发起者；用于距离、朝向和遮挡计算。
	 * @param outTarget 本次领域操作的结构化数据 `outTarget`；字段语义由对应 USTRUCT 定义。
	 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
	 */
	bool FindAttackTarget(AActor* instigator, AActor*& outTarget) const;

private:
	/**
	 * @brief 判断 Is Occluded 对应条件；不产生玩法副作用。
	 * @param instigator 攻击发起者；用于距离、朝向和遮挡计算。
	 * @param target 本次规则检查或操作的目标对象。
	 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
	 */
	bool IsOccluded(AActor* instigator, const AActor* target) const;
	/**
	 * @brief 查询 Effective Tuning；不修改领域状态。
	 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
	 */
	const ULRStateTuning& GetEffectiveTuning() const;

	/** 运行时解析出的调优资产缓存；不序列化，不由蓝图编辑。 该字段仅为运行时缓存，不进入存档。 */
	UPROPERTY(Transient)
	TObjectPtr<const ULRStateTuning> Tuning;
};
