// Copyright AmberAeolian. All Rights Reserved.

using UnrealBuildTool;

public class ItemCore : ModuleRules
{
	public ItemCore(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
				"StructUtils",
				"GameplayTags",
				"DebugSystem",
			}
		);

		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"CoreUObject",
				"Engine",
				"NetCore",
			}
		);
	}
}