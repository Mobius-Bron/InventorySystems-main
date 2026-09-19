using UnrealBuildTool;

public class OldMultiplayerInventory : ModuleRules
{
	public OldMultiplayerInventory(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
				"NetCore",
				"StructUtils",
				"GameplayTags",
				"DebugHelper"
			}
		);

		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"CoreUObject",
				"Engine",
				"Slate",
				"SlateCore",
				"EnhancedInput",
				"UMG",
				"InputCore"
			}
		);
	}
}
