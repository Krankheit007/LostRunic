/**
 * @file LRHidePoint.h
 * @brief 实现固定/可移动躲藏点、守卫可见性接口和统一噪声发布，使守卫通过事件感知玩家而非轮询角色速度或修改基础视野。
 *
 * 关联文件：LRHidePoint.cpp；所属领域：Stealth。
 * 设计依据：Docs/Design/01_GameDesignSummary.md 与 Docs/Technical/04_TechnicalDesign.md。
 * 除带 EditDefaultsOnly、EditAnywhere 或 EditInstanceOnly 的字段外，其余成员均为运行时状态，不应由蓝图直接改写。
 */
#pragma once

#include "GameFramework/Actor.h"
#include "Interaction/LRInteractable.h"

#include "LRHidePoint.generated.h"

class USceneComponent;

/** 该公开类型定义本文件领域边界的数据或行为；具体字段、参数与约束见下方中文注释。 */
UCLASS(Blueprintable, meta = (DisplayName = "Lost Runic Hide Point"))
class LOSTRUNIC_API ALRHidePoint : public AActor, public ILRInteractable
{
	GENERATED_BODY()

public:
	/**
	 * @brief 创建对象并设置默认子对象、能力开关和安全初值；需要 World、资产或玩家的依赖延迟到初始化阶段解析。
	 */
	ALRHidePoint();

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
	 * @brief 查询 Hide Location；不修改领域状态。
	 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
	 */
	FVector GetHideLocation() const { return GetActorLocation(); }
	/**
	 * @brief 查询 Exit Location；不修改领域状态。
	 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
	 */
	FVector GetExitLocation() const { return GetActorLocation() + GetActorForwardVector() * ExitOffset; }
	/**
	 * @brief 实现 Allows Movement 对应的领域步骤；调用关系和状态所有权由本文件所属系统维护。
	 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
	 */
	bool AllowsMovement() const { return bAllowMovementWhileHidden; }

private:
	/** Scene Root 的领域数据，由所属类型负责维护和校验。 仅在蓝图或详情面板中查看，不可编辑。 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USceneComponent> SceneRoot;

	/** Interaction Option 的领域数据，由所属类型负责维护和校验。 可在 DataAsset 或蓝图类默认值中配置，运行时蓝图只读。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Hide", meta = (AllowPrivateAccess = "true"))
	FLRInteractionOption InteractionOption;

	/** Allow Movement While Hidden 的开关；true 表示启用，false 表示禁用。 C++ 安全默认值为 `false`。 可在对应资产、DataTable 行或蓝图实例中配置。 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Hide", meta = (AllowPrivateAccess = "true"))
	bool bAllowMovementWhileHidden = false;

	/** Exit Offset 的领域数据，由所属类型负责维护和校验。 C++ 安全默认值为 `100.0f`。 可在对应资产、DataTable 行或蓝图实例中配置。编辑器约束：单位 `cm`，最小值 `0.0`，最大值 `500.0`。 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Hide", meta = (AllowPrivateAccess = "true", ClampMin = "0.0", ClampMax = "500.0", Units = "cm"))
	float ExitOffset = 100.0f;
};
