/**
 * @file LRGuardTuning.h
 * @brief Guard 0-11 警戒、视觉确认、调查移动和捕获的唯一调优来源。
 *
 * UE Sight 的半径、LoseSightRadius 和 PeripheralVisionAngleDegrees 由 Guard Controller
 * Blueprint 的 inherited AIPerception 配置；本结构体只保存玩法节奏参数。
 */
#pragma once

#include "CoreMinimal.h"

#include "LRGuardTuning.generated.h"

/** 可在 BP_LRGuardController Class Defaults 中调整的 Guard 玩法参数。 */
USTRUCT(BlueprintType, meta = (DisplayName = "守卫调优设置"))
struct LOSTRUNIC_API FLRGuardTuningSettings
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Guard|警戒", meta = (DisplayName = "警戒增加量", ToolTip = "普通吸引注意事件增加的警戒值；有效范围为0到10，警戒不会超过10。", ClampMin = "1", ClampMax = "10"))
	int32 AttractAlertAmount = 1;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Guard|警戒", meta = (DisplayName = "可疑观察时间", ToolTip = "0变为1，或白色档位中的新异常被接受后，警戒保持不变的时间。", ClampMin = "0.1", ClampMax = "30.0", Units = "s"))
	float SuspiciousObserveSeconds = 3.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Guard|警戒", meta = (DisplayName = "调查观察时间", ToolTip = "Guard 抵达红色调查点后保持警戒不变的观察时间。", ClampMin = "0.1", ClampMax = "30.0", Units = "s"))
	float InvestigateObserveSeconds = 3.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Guard|警戒", meta = (DisplayName = "视觉追逐确认宽限", ToolTip = "低警戒首次看到敌对角色后，从6确认到11前的短暂宽限；期间警戒冻结。", ClampMin = "0.0", ClampMax = "10.0", Units = "s"))
	float SightToChaseGraceSeconds = 0.5f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Guard|警戒", meta = (DisplayName = "白色刺激冷却", ToolTip = "白色1到5档接受噪声刺激后，下一次噪声增加警戒前的基础冷却。", ClampMin = "0.0", ClampMax = "10.0", Units = "s"))
	float SuspiciousStimulusCooldownSeconds = 0.5f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Guard|警戒", meta = (DisplayName = "红色刺激冷却", ToolTip = "红色6到10档接受噪声刺激后，下一次噪声增加警戒前的基础冷却。", ClampMin = "0.0", ClampMax = "10.0", Units = "s"))
	float InvestigateStimulusCooldownSeconds = 0.2f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Guard|警戒", meta = (DisplayName = "警戒衰减量", ToolTip = "每个衰减周期降低的警戒值。", ClampMin = "1", ClampMax = "10"))
	int32 AlertDecayAmount = 1;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Guard|警戒", meta = (DisplayName = "警戒衰减间隔", ToolTip = "观察结束后，警戒自然衰减的周期。", ClampMin = "0.05", ClampMax = "10.0", Units = "s"))
	float AlertDecayIntervalSeconds = 0.5f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Guard|警戒", meta = (DisplayName = "当前房奔跑警戒下限", ToolTip = "当前房间首次接受玩家奔跑时，警戒至少提升到该白色档位；达到下限后后续奔跑按普通增加量累积。", ClampMin = "1", ClampMax = "5"))
	int32 RoomRunAlertLevel = 5;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Guard|警戒", meta = (DisplayName = "相邻房奔跑警戒增加量", ToolTip = "相邻房间收到玩家奔跑传播时增加的警戒值，最高到10。", ClampMin = "1", ClampMax = "10"))
	int32 AdjacentRoomRunAlertAmount = 1;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Guard|视觉", meta = (DisplayName = "视觉跟踪间隔", ToolTip = "Raw UE Sight Contact 存在时检查有效视觉的定时器间隔；Hard Hidden 不会停止该定时器。", ClampMin = "0.01", ClampMax = "1.0", Units = "s"))
	float SightTrackingIntervalSeconds = 0.1f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Guard|警戒", meta = (DisplayName = "奔跑首次刺激冷却倍率", ToolTip = "玩家奔跑发出的噪声首次进入当前警戒档位时使用的冷却倍率。", ClampMin = "0.0", ClampMax = "4.0"))
	float FirstAttractRunCooldownMultiplier = 0.6f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Guard|警戒", meta = (DisplayName = "走路首次刺激冷却倍率", ToolTip = "玩家走路发出的噪声首次进入当前警戒档位时使用的冷却倍率。", ClampMin = "0.0", ClampMax = "4.0"))
	float FirstAttractWalkCooldownMultiplier = 1.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Guard|警戒", meta = (DisplayName = "潜行首次刺激冷却倍率", ToolTip = "玩家潜行发出的噪声首次进入当前警戒档位时使用的冷却倍率。", ClampMin = "0.0", ClampMax = "4.0"))
	float FirstAttractSneakCooldownMultiplier = 1.6f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Guard|移动", meta = (DisplayName = "巡逻速度", ToolTip = "Guard Idle/Patrol 时的移动速度。", ClampMin = "1.0", ClampMax = "1000.0", Units = "cm/s"))
	float PatrolSpeed = 120.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Guard|移动", meta = (DisplayName = "调查速度", ToolTip = "Guard 前往最新异常位置时的移动速度。", ClampMin = "1.0", ClampMax = "1000.0", Units = "cm/s"))
	float InvestigateSpeed = 170.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Guard|移动", meta = (DisplayName = "追逐速度", ToolTip = "Guard Chase 时的移动速度。", ClampMin = "1.0", ClampMax = "1000.0", Units = "cm/s"))
	float ChaseSpeed = 300.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Guard|移动", meta = (DisplayName = "调查到达误差", ToolTip = "调查导航请求的接受半径。", ClampMin = "1.0", ClampMax = "500.0", Units = "cm"))
	float MoveAcceptanceRadius = 50.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Guard|移动", meta = (DisplayName = "调查重定向距离", ToolTip = "连续视觉跟踪期间，最新可见位置移动超过该距离后才重新发送调查导航请求；离散噪声会直接重定向。", ClampMin = "1.0", ClampMax = "1000.0", Units = "cm"))
	float InvestigateMoveRetargetDistanceCm = 75.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Guard|捕获", meta = (DisplayName = "捕获半径", ToolTip = "Guard 追上已确认敌对角色后触发死亡占位的距离。", ClampMin = "10.0", ClampMax = "500.0", Units = "cm"))
	float CaptureRadius = 75.0f;

	/** 校验调优值和速度/档位关系；失败原因供编辑器和运行时日志使用。 */
	bool Validate(FString& outError) const;
};
