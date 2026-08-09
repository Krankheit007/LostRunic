/**
 * @file LRGameInstanceSubsystem.h
 * @brief 连接 LostRunic 的 Gameplay Framework：GameMode 管理单机世界规则，PlayerController 解释 Enhanced Input 与 UI 模式，Character 只组合能力组件，GameInstanceSubsystem 提供跨地图内容与调优配置。
 *
 * 关联文件：LRGameInstanceSubsystem.cpp；所属领域：Framework。
 * 设计依据：Docs/Design/01_GameDesignSummary.md 与 Docs/Technical/04_TechnicalDesign.md。
 * 除带 EditDefaultsOnly、EditAnywhere 或 EditInstanceOnly 的字段外，其余成员均为运行时状态，不应由蓝图直接改写。
 */
#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"

#include "LRGameInstanceSubsystem.generated.h"

class ULRGameContentSet;
class ULRGameTuningSet;

/** 该公开类型定义本文件领域边界的数据或行为；具体字段、参数与约束见下方中文注释。 */
UCLASS(meta = (DisplayName = "Lost Runic Game Instance Subsystem"))
class LOSTRUNIC_API ULRGameInstanceSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	/**
	 * @brief 初始化子系统拥有的长期状态与事件绑定。
	 * @param collection 调用方提供的 `collection`，只在本次操作范围内使用。
	 */
	virtual void Initialize(FSubsystemCollectionBase& collection) override;
	/**
	 * @brief 释放子系统事件绑定和运行时缓存。
	 */
	virtual void Deinitialize() override;

	/**
	 * @brief 查询 Tuning Set；不修改领域状态。
	 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
	 */
	UFUNCTION(BlueprintPure, Category = "Lost Runic|Data")
	ULRGameTuningSet* GetTuningSet() const { return TuningSet; }

	/**
	 * @brief 查询 Content Set；不修改领域状态。
	 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
	 */
	UFUNCTION(BlueprintPure, Category = "Lost Runic|Data")
	ULRGameContentSet* GetContentSet() const { return ContentSet; }

	/**
	 * @brief 判断 Has Valid Configuration 对应条件；不产生玩法副作用。
	 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
	 */
	bool HasValidConfiguration() const { return bConfigurationValid; }

private:
	/** Tuning Set 的领域数据，由所属类型负责维护和校验。 该字段仅为运行时缓存，不进入存档。 */
	UPROPERTY(Transient)
	TObjectPtr<ULRGameTuningSet> TuningSet;

	/** Content Set 的领域数据，由所属类型负责维护和校验。 该字段仅为运行时缓存，不进入存档。 */
	UPROPERTY(Transient)
	TObjectPtr<ULRGameContentSet> ContentSet;

	/** Configuration Valid 的运行时状态；由所属类型维护，不在蓝图中配置。 */
	bool bConfigurationValid = false;
};
