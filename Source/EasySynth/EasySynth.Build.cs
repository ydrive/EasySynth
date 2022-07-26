// Copyright (c) 2022 YDrive Inc. All rights reserved.

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
				"CinematicCamera",
				"CoreUObject",
				"Engine",
				"InputCore",
				"Landscape",
				"Projects",
				"Slate",
				"SlateCore",
				"ToolMenus",
				"UnrealEd",
				// Editor bindings
				"EditorScriptingUtilities",
				"UnrealEd",
				// Sequencer module
				"LevelSequence",
				"LevelSequenceEditor",
				"MovieRenderPipelineCore",
				"MovieRenderPipelineEditor",
				"MovieRenderPipelineRenderPasses",
				"MovieScene",
				"MovieSceneTracks",
				"Sequencer",
				// Editor tab functionality
				"AppFramework",
				"DesktopPlatform",
				"DesktopWidgets",
				"MainFrame",
				"PropertyEditor",
				// Image formats
				"UEOpenExrRTTI",
				// JSON parsing
				"Json", "JsonUtilities",
				// ... add private dependencies that you statically link with here ...
			}
			);


		DynamicallyLoadedModuleNames.AddRange(
			new string[]
			{
				// ... add any modules that your module loads dynamically here ...
			}
			);

		// Required for UEOpenExr
		AddEngineThirdPartyPrivateStaticDependencies(Target, "zlib");
		AddEngineThirdPartyPrivateStaticDependencies(Target, "UEOpenExr");
	}
}
