// Copyright (c) YDrive Inc. All rights reserved.
// Licensed under the MIT License.

using UnrealBuildTool;

public class EasySynth : ModuleRules
{
	public EasySynth(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicIncludePaths.AddRange(
			new string[] {
				// ... add public include paths required here ...
			}
			);


		PrivateIncludePaths.AddRange(
			new string[] {
				// ... add other private include paths required here ...
			}
			);


		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
				// ... add other public dependencies that you statically link with here ...
			}
			);


		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				// Default modules
				"Projects",
				"InputCore",
				"UnrealEd",
				"ToolMenus",
				"CoreUObject",
				"Engine",
				"Slate",
				"SlateCore",
				// Editor bindings
				"EditorScriptingUtilities",
				"UnrealEd",
				// Sequencer module
				"LevelSequence",
				"LevelSequenceEditor",
				"MovieRenderPipelineCore",
				"MovieRenderPipelineEditor",
				"MovieScene",
				"MovieSceneTracks",
				"Sequencer",
				// Editor tab functionality
				"DesktopWidgets",
				"PropertyEditor",
				// ... add private dependencies that you statically link with here ...
			}
			);


		DynamicallyLoadedModuleNames.AddRange(
			new string[]
			{
				// ... add any modules that your module loads dynamically here ...
			}
			);
	}
}
