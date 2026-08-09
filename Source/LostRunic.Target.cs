// Copyright Epic Games, Inc. All Rights Reserved.

/**
 * @file LostRunic.Target.cs
 * @brief 定义 LostRunic 游戏目标，供非编辑器构建使用。
 *
 * 关联文件：LostRunic.Build.cs、LostRunicEditor.Target.cs 与 LostRunic.uproject。
 */
using UnrealBuildTool;
using System.Collections.Generic;

public class LostRunicTarget : TargetRules
{
	/**
	 * @brief 使用 UE 5.8 的 V7 默认构建设置创建 Game 目标，并装载 LostRunic 模块。
	 * @param Target Unreal Build Tool 提供的目标平台与配置；该参数不属于运行时或蓝图配置。
	 */
	public LostRunicTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Game;
		DefaultBuildSettings = BuildSettingsVersion.V7;
		IncludeOrderVersion = EngineIncludeOrderVersion.Unreal5_8;
		ExtraModuleNames.Add("LostRunic");
	}
}
