// Copyright Epic Games, Inc. All Rights Reserved.

/**
 * @file LostRunic.Build.cs
 * @brief 配置 LostRunic Runtime 模块的编译依赖和公共包含目录。
 *
 * 关联文件：LostRunic.Target.cs、LostRunicEditor.Target.cs 与 LostRunic.uproject。
 * GameplayTags、GameplayTasks、StateTree、EnhancedInput、UMG、Niagara 等依赖分别服务于
 * 04_TechnicalDesign 中的状态、输入、守卫 AI、UI、表现和存档垂直切片。
 */
using UnrealBuildTool;

public class LostRunic : ModuleRules
{
	/**
	 * @brief 创建 LostRunic 模块规则，声明 UE 5.8 编译所需的公共/私有模块和 include 路径。
	 * @param Target Unreal Build Tool 提供的只读目标规则；用于选择当前平台与构建配置，不在蓝图中配置。
	 */
	public LostRunic(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[] {
			"Core",
			"CoreUObject",
			"Engine",
			"EnhancedInput",
			"AIModule",
			"DeveloperSettings",
			"StateTreeModule",
			"GameplayStateTreeModule",
			"GameplayTags",
			"SUDS",
			"UMG",
		});

		PrivateDependencyModuleNames.AddRange(new string[] {
			"InputCore",
			"NavigationSystem",
			"GameplayTasks",
			"Niagara",
			"PhysicsCore",
			"Slate",
			"SlateCore"
		});

		PublicIncludePaths.AddRange(new string[] {
			"LostRunic",
			"LostRunic/Variant_Strategy",
			"LostRunic/Variant_Strategy/UI",
			"LostRunic/Variant_TwinStick",
			"LostRunic/Variant_TwinStick/AI",
			"LostRunic/Variant_TwinStick/Gameplay",
			"LostRunic/Variant_TwinStick/UI"
		});

		// 仅在未来启用在线功能时加入 OnlineSubsystem；当前设计为 Windows 单机。
		// PrivateDependencyModuleNames.Add("OnlineSubsystem");

		// 若未来使用 OnlineSubsystemSteam，还必须在 LostRunic.uproject 的插件列表中显式启用。
	}
}
