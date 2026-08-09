/**
 * @file LRItemUseTargetActor.h
 * @brief 实现 4 格快捷栏、背包、笔记、收藏品和统一物品使用事务；快捷栏与交互后选物共用解析入口，失败时回滚消耗并返回结构化原因。
 *
 * 关联文件：LRItemUseTargetActor.cpp；所属领域：Items。
 * 设计依据：Docs/Design/01_GameDesignSummary.md 与 Docs/Technical/04_TechnicalDesign.md。
 * 除带 EditDefaultsOnly、EditAnywhere 或 EditInstanceOnly 的字段外，其余成员均为运行时状态，不应由蓝图直接改写。
 */
#pragma once

#include "GameFramework/Actor.h"
#include "Interaction/LRInteractable.h"
#include "Items/LRItemUseTarget.h"

#include "LRItemUseTargetActor.generated.h"

/** 该公开类型定义本文件领域边界的数据或行为；具体字段、参数与约束见下方中文注释。 */
UCLASS(Blueprintable, meta = (DisplayName = "Lost Runic Item Use Target"))
class LOSTRUNIC_API ALRItemUseTargetActor : public AActor, public ILRInteractable, public ILRItemUseTarget
{
	GENERATED_BODY()

public:
	/**
	 * @brief 创建对象并设置默认子对象、能力开关和安全初值；需要 World、资产或玩家的依赖延迟到初始化阶段解析。
	 */
	ALRItemUseTargetActor();

	/**
	 * @brief 查询 Interaction Options_Implementation；不修改领域状态。
	 * @param interactor 参与本次操作的运行时对象 `interactor`；函数会检查空值和所需接口。
	 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
	 */
	virtual TArray<FLRInteractionOption> GetInteractionOptions_Implementation(AActor* interactor) override;
	/**
	 * @brief 查询 Interaction Location_Implementation；不修改领域状态。
	 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
	 */
	virtual FVector GetInteractionLocation_Implementation() override;
	/**
	 * @brief 执行当前交互选项；物品目标仍通过统一物品事务入口结算。
	 * @param interactor 参与本次操作的运行时对象 `interactor`；函数会检查空值和所需接口。
	 * @param actionTag Gameplay Tag 或标签集合，用于分类、条件、拒绝原因和可诊断事件。
	 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
	 */
	virtual FLRInteractionResult ExecuteInteraction_Implementation(AActor* interactor, FGameplayTag actionTag) override;
	/**
	 * @brief 查询 Item Use Target Tags_Implementation；不修改领域状态。
	 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
	 */
	virtual FGameplayTagContainer GetItemUseTargetTags_Implementation() override;
	/**
	 * @brief 对通用物品目标执行一次性使用逻辑；消耗、失败回滚和事件提交仍由 LRItemUseResolver 统一管理。
	 * @param request 物品 ID、来源栏位、入口、目标与玩家状态组成的统一请求。
	 * @param definition 已按稳定 ID 解析且通过基础标签检查的物品定义。
	 * @return 返回目标执行结果及原因标签，供同一事务决定提交或回滚。
	 */
	virtual FLRItemUseResult ApplyItemUse_Implementation(const FLRItemUseRequest& request,
		ULRItemDefinition* definition) override;

	/** Interaction Option 的领域数据，由所属类型负责维护和校验。 可在 DataAsset 或蓝图类默认值中配置，运行时蓝图只读。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Interaction")
	FLRInteractionOption InteractionOption;

	/** Target Tags 的 Gameplay Tag 条件或分类，用于数据驱动规则与诊断。 可在 DataAsset 或蓝图类默认值中配置，运行时蓝图只读。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item Use")
	FGameplayTagContainer TargetTags;

	/** Event Id 的稳定 FName/GUID 标识；用于定义查询和存档，不依赖显示名或临时 Actor 名称。 C++ 安全默认值为 `NAME_None`。 可在 DataAsset 或蓝图类默认值中配置，运行时蓝图只读。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item Use")
	FName EventId = NAME_None;

	/** One Shot 的开关；true 表示启用，false 表示禁用。 C++ 安全默认值为 `true`。 可在 DataAsset 或蓝图类默认值中配置，运行时蓝图只读。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item Use")
	bool bOneShot = true;

	/**
	 * @brief 判断 Is Completed 对应条件；不产生玩法副作用。
	 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
	 */
	UFUNCTION(BlueprintPure, Category = "Lost Runic|Item Use")
	bool IsCompleted() const { return bCompleted; }

protected:
	/**
	 * @brief 处理 On Item Use Applied 事件，将引擎回调转换为对应领域状态更新。
	 * @param request 不可变领域请求，包含本次操作所需的稳定 ID、来源、目标或原因。
	 * @param definition 数据或调优来源 `definition`；调用期间只读，并按稳定 ID 解析内容。
	 */
	UFUNCTION(BlueprintImplementableEvent, Category = "Lost Runic|Item Use", meta = (DisplayName = "Item Use Applied"))
	void OnItemUseApplied(const FLRItemUseRequest& request, ULRItemDefinition* definition);

private:
	/** Completed 的开关；true 表示启用，false 表示禁用。 C++ 安全默认值为 `false`。 仅在蓝图或详情面板中查看，不可编辑。 */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Item Use", meta = (AllowPrivateAccess = "true"))
	bool bCompleted = false;
};
