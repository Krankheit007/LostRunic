/**
 * @file LRGuardCharacter.h
 * @brief 实现“家”垂直切片的守卫感知、0-11 警戒值、StateTree 行为切换、调查追逐与捕获死亡流程。规则层只计算状态，Controller 负责接入 UE 感知、导航和计时器。
 *
 * 关联文件：LRGuardCharacter.cpp；所属领域：AI。
 * 设计依据：Docs/Design/01_GameDesignSummary.md 与 Docs/Technical/04_TechnicalDesign.md。
 * 除带 EditDefaultsOnly、EditAnywhere 或 EditInstanceOnly 的字段外，其余成员均为运行时状态，不应由蓝图直接改写。
 */
#pragma once

#include "GameFramework/Character.h"

#include "LRGuardCharacter.generated.h"

class AActor;
class ALRGuardAIController;
class ULRCourageResponseComponent;
class ULRAlertComponent;
class ULRGuardDefinition;
class UWidgetComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FLRPlayerCaptured, AActor*, playerActor);

/** 该公开类型定义本文件领域边界的数据或行为；具体字段、参数与约束见下方中文注释。 */
UCLASS(BlueprintType, meta = (DisplayName = "Lost Runic Guard Character"))
class LOSTRUNIC_API ALRGuardCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	/**
	 * @brief 创建对象并设置默认子对象、能力开关和安全初值；需要 World、资产或玩家的依赖延迟到初始化阶段解析。
	 */
	ALRGuardCharacter();

	/**
	 * @brief 在进入世界后解析运行时依赖：将世界警戒条 Widget 初始化到本守卫的警戒快照。
	 */
	virtual void BeginPlay() override;

	/**
	 * @brief 查询 Alert Component；不修改领域状态。
	 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
	 */
	UFUNCTION(BlueprintPure, Category = "Lost Runic|AI")
	ULRAlertComponent* GetAlertComponent() const { return Alert; }

	/**
	 * @brief 确认目标仍可捕获后提交死亡状态请求，并启动死亡到 Memory 的存档事务。
	 * @param target 本次规则检查或操作的目标对象。
	 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
	 */
	UFUNCTION(BlueprintCallable, Category = "Lost Runic|AI")
	bool CaptureTarget(AActor* target);

	/**
	 * @brief 查询 Definition；不修改领域状态。
	 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
	 */
	UFUNCTION(BlueprintPure, Category = "Lost Runic|AI")
	ULRGuardDefinition* GetDefinition() const { return Definition; }

	/**
	 * @brief 查询 Courage Response Component；不修改领域状态。
	 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
	 */
	UFUNCTION(BlueprintPure, Category = "Lost Runic|AI")
	ULRCourageResponseComponent* GetCourageResponseComponent() const { return CourageResponse; }

	/**
	 * @brief 查询 Patrol Point；不修改领域状态。
	 * @param index 目标元素索引，调用前必须满足对应容器边界。
	 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
	 */
	AActor* GetPatrolPoint(int32 index) const;
	/**
	 * @brief 查询 Patrol Point Count；不修改领域状态。
	 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
	 */
	int32 GetPatrolPointCount() const { return PatrolPoints.Num(); }

	/** 当 Player Captured 发生时广播；蓝图可绑定该委托以更新表现，不应在回调中改写核心规则。  */
	UPROPERTY(BlueprintAssignable, Category = "Lost Runic|AI")
	FLRPlayerCaptured OnPlayerCaptured;

protected:
	/** Definition 的领域数据，由所属类型负责维护和校验。 可在 DataAsset 或蓝图类默认值中配置，运行时蓝图只读。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Guard")
	TObjectPtr<ULRGuardDefinition> Definition;

	/** Patrol Points 的领域数据，由所属类型负责维护和校验。 可在关卡中的蓝图实例详情面板配置。 */
	UPROPERTY(EditInstanceOnly, Category = "Guard|Patrol")
	TArray<TObjectPtr<AActor>> PatrolPoints;

private:
	/** Alert 的领域数据，由所属类型负责维护和校验。 仅在蓝图或详情面板中查看，不可编辑。 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<ULRAlertComponent> Alert;

	/** Courage Response 的领域数据，由所属类型负责维护和校验。 仅在蓝图或详情面板中查看，不可编辑。 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<ULRCourageResponseComponent> CourageResponse;

	/** Alert Widget 的世界空间 WidgetComponent；WidgetClass 与样式由蓝图配置，C++ 只负责初始化绑定。 仅在蓝图或详情面板中查看，不可编辑。 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UWidgetComponent> AlertWidget;
};
