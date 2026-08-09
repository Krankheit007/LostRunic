// Copyright Epic Games, Inc. All Rights Reserved.

/**
 * @file LostRunicCharacter.h
 * @brief 提供 Unreal 模块入口及原始 TopDown 模板兼容类；新的“家”垂直切片使用 Framework、State、Interaction、AI、Narrative 与 Save 目录中的 LR 领域实现。
 *
 * 关联文件：LostRunicCharacter.cpp；所属领域：Root。
 * 设计依据：Docs/Design/01_GameDesignSummary.md 与 Docs/Technical/04_TechnicalDesign.md。
 * 除带 EditDefaultsOnly、EditAnywhere 或 EditInstanceOnly 的字段外，其余成员均为运行时状态，不应由蓝图直接改写。
 */

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "LostRunicCharacter.generated.h"

class UCameraComponent;
class USpringArmComponent;

/**
 *  A controllable top-down perspective character
 */
UCLASS(abstract)
class ALostRunicCharacter : public ACharacter
{
	GENERATED_BODY()

private:

	/** 该公开类型定义本文件领域边界的数据或行为；具体字段、参数与约束见下方中文注释。 */
	/** Top Down Camera Component 的领域数据，由所属类型负责维护和校验。 仅在蓝图或详情面板中查看，不可编辑。 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UCameraComponent> TopDownCameraComponent;

	/** 该公开类型定义本文件领域边界的数据或行为；具体字段、参数与约束见下方中文注释。 */
	/** Camera Boom 的领域数据，由所属类型负责维护和校验。 仅在蓝图或详情面板中查看，不可编辑。 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USpringArmComponent> CameraBoom;

public:

	/** 该公开类型定义本文件领域边界的数据或行为；具体字段、参数与约束见下方中文注释。 */
	/**
	 * @brief 创建对象并设置默认子对象、能力开关和安全初值；需要 World、资产或玩家的依赖延迟到初始化阶段解析。
	 */
	ALostRunicCharacter();

	/** 该公开类型定义本文件领域边界的数据或行为；具体字段、参数与约束见下方中文注释。 */
	/**
	 * @brief 在进入世界后解析运行时依赖、绑定事件并启动所需计时器；构造阶段不访问 World 或玩家对象。
	 */
	virtual void BeginPlay() override;

	/** 该公开类型定义本文件领域边界的数据或行为；具体字段、参数与约束见下方中文注释。 */
	/**
	 * @brief 实现 Tick 对应的领域步骤；调用关系和状态所有权由本文件所属系统维护。
	 * @param DeltaSeconds 自上次更新经过的秒数，仅用于显式的时间累计。
	 */
	virtual void Tick(float DeltaSeconds) override;

	/** Returns the camera component **/
	/**
	 * @brief 查询 Top Down Camera Component；不修改领域状态。
	 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
	 */
	UCameraComponent* GetTopDownCameraComponent() const { return TopDownCameraComponent.Get(); }

	/** Returns the Camera Boom component **/
	/**
	 * @brief 查询 Camera Boom；不修改领域状态。
	 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
	 */
	USpringArmComponent* GetCameraBoom() const { return CameraBoom.Get(); }

};

