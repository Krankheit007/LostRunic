/**
 * @file LRGameMode.h
 * @brief 连接 LostRunic 的 Gameplay Framework：GameMode 管理单机世界规则，PlayerController 解释 Enhanced Input 与 UI 模式，Character 只组合能力组件，GameInstanceSubsystem 提供跨地图内容与调优配置。
 *
 * 关联文件：LRGameMode.cpp；所属领域：Framework。
 * 设计依据：Docs/Design/01_GameDesignSummary.md 与 Docs/Technical/04_TechnicalDesign.md。
 * 除带 EditDefaultsOnly、EditAnywhere 或 EditInstanceOnly 的字段外，其余成员均为运行时状态，不应由蓝图直接改写。
 */
#pragma once

#include "GameFramework/GameModeBase.h"

#include "LRGameMode.generated.h"

/** 该公开类型定义本文件领域边界的数据或行为；具体字段、参数与约束见下方中文注释。 */
UCLASS(BlueprintType, meta = (DisplayName = "Lost Runic Game Mode"))
class LOSTRUNIC_API ALRGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	/**
	 * @brief 创建对象并设置默认子对象、能力开关和安全初值；需要 World、资产或玩家的依赖延迟到初始化阶段解析。
	 */
	ALRGameMode();

	/**
	 * @brief 在进入世界后解析运行时依赖、绑定事件并启动所需计时器；构造阶段不访问 World 或玩家对象。
	 */
	virtual void BeginPlay() override;

protected:
	/** 首次进入可玩 Home 世界时的稳定出生锚点；关卡可在派生 GameMode 中覆盖。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Lost Runic|Save")
	FName DefaultResumeAnchorId = FName(TEXT("Home.Hall"));

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Lost Runic|Save")
	bool bInitializeResumeAnchor = true;
};
