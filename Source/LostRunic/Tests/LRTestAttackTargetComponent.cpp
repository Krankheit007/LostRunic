/**
 * @file LRTestAttackTargetComponent.cpp
 * @brief 攻击目标测试替身实现；仅 WITH_DEV_AUTOMATION_TESTS 下编译。
 *
 * 关联文件：LRTestAttackTargetComponent.h；所属领域：Tests。
 */
#include "Tests/LRTestAttackTargetComponent.h"

#include "Core/LRGameplayTags.h"
#include "Data/LRItemDefinition.h"

/**
 * @brief 测试替身记录攻击目标被调用的次数，并按 bShouldSucceed 返回成功或失败结果。
 * @param request 自动化测试构造的统一攻击请求。
 * @param definition 测试解析出的物品定义；空手攻击时为 nullptr。
 * @return bShouldSucceed 为 true 时成功，否则返回执行拒绝结果。
 */
FLRItemUseResult ULRTestAttackTargetComponent::ApplyAttack_Implementation(const FLRItemUseRequest& request,
	ULRItemDefinition* definition)
{
	++ApplyCount;
	FLRItemUseResult result;
	result.ItemId = request.ItemId;
	if (!bShouldSucceed)
	{
		result.FailureReason = LRGameplayTags::ItemUseRejectExecution;
		return result;
	}
	result.bSuccess = true;
	return result;
}
