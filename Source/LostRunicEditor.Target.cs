// Copyright Epic Games, Inc. All Rights Reserved.

/**
 * @file LostRunicEditor.Target.cs
 * @brief 定义 LostRunicEditor 目标，用于编辑器、PIE、Data Validation 和 LostRunic.* 自动化测试。
 *
 * 关联文件：LostRunic.Build.cs、LostRunic.Target.cs 与 LostRunic.uproject。
 */
using UnrealBuildTool;
using System.Collections.Generic;

public class LostRunicEditorTarget : TargetRules
{
	/**
	 * @brief 使用 UE 5.8 的 V7 默认构建设置创建 Editor 目标，并装载 LostRunic 模块。
	 * @param Target Unreal Build Tool 提供的目标平台与配置；该参数不属于运行时或蓝图配置。
	 */
	public LostRunicEditorTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Editor;
		DefaultBuildSettings = BuildSettingsVersion.V7;
		IncludeOrderVersion = EngineIncludeOrderVersion.Unreal5_8;
		ExtraModuleNames.Add("LostRunic");
	}
}
