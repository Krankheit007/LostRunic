/**
 * @file LRGameMode.cpp
 * @brief 连接 LostRunic 的 Gameplay Framework：GameMode 管理单机世界规则，PlayerController 解释 Enhanced Input 与 UI 模式，Character 只组合能力组件，GameInstanceSubsystem 提供跨地图内容与调优配置。
 *
 * 关联文件：LRGameMode.h；所属领域：Framework。
 * 设计依据：Docs/Design/01_GameDesignSummary.md 与 Docs/Technical/04_TechnicalDesign.md。
 * 除带 EditDefaultsOnly、EditAnywhere 或 EditInstanceOnly 的字段外，其余成员均为运行时状态，不应由蓝图直接改写。
 */
#include "Framework/LRGameMode.h"

#include "Framework/LRCharacter.h"
#include "Framework/LRGameState.h"
#include "Framework/LRPlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Save/LRSaveSubsystem.h"
#include "UI/LRHUD.h"

/**
 * @brief 创建对象并设置默认子对象、能力开关和安全初值；需要 World、资产或玩家的依赖延迟到初始化阶段解析。
 */
ALRGameMode::ALRGameMode()
{
	DefaultPawnClass = ALRCharacter::StaticClass();
	PlayerControllerClass = ALRPlayerController::StaticClass();
	GameStateClass = ALRGameState::StaticClass();
	HUDClass = ALRHUD::StaticClass();
}

/**
 * @brief 在进入世界后解析运行时依赖、绑定事件并启动所需计时器；构造阶段不访问 World 或玩家对象。
 */
void ALRGameMode::BeginPlay()
{
	Super::BeginPlay();
	if (UGameInstance* gameInstance = GetGameInstance())
	{
		if (ULRSaveSubsystem* saveSubsystem = gameInstance->GetSubsystem<ULRSaveSubsystem>())
		{
			saveSubsystem->HandleWorldReady(Cast<ALRCharacter>(UGameplayStatics::GetPlayerCharacter(this, 0)));
		}
	}
}
