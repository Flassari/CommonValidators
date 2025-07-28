using UnrealBuildTool;

public class CommonValidators : ModuleRules
{
	public CommonValidators(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

		PrivateDependencyModuleNames.AddRange(new string[] {
			"Core",
			"CoreUObject",
			"Engine",
			"DataValidation",
			"BlueprintGraph",
			"DeveloperSettings",
			"Kismet",
			"UnrealEd"
		});
	}
}
