// Copyright (c) 2022 YDrive Inc. All rights reserved.

#pragma once

#include "CoreMinimal.h"

#include "Camera/CameraComponent.h"


class FPathUtils
{
public:
	/**
	 * Universal utils
	*/

	/** Plugin name to be referenced by other methods */
	static const FString PluginName;

	/**
	 * Plugin content utils
	 * Point to the contents of the plugin
	*/

	/** Plugin content directory, because editor utils cannot tell us that */
	static FString PluginContentDir()
	{
		return FString::Printf(TEXT("/%s"), *PluginName);
	}

	/** Path to the plain color material asset */
	static FString PlainColorMaterialPath()
	{
		return PluginContentDir() / PlainColorMaterialAssetName;
	}

	/** Path to the plugin specific movie pipeline config preset */
	static FString DefaultMoviePipelineConfigPath()
	{
		return PluginContentDir() / DefaultMoviePipelineConfigAssetName;
	}

	/** Directory containing post-process materials for render targets */
	static FString PostProcessMaterialsDir()
	{
		return PluginContentDir() / PostProcessMaterialDirName;
	}

	/** Path to the specific post-process material */
	static FString PostProcessMaterialPath(const FString& TargetName)
	{
		return PostProcessMaterialsDir() / FString::Printf(TEXT("M_PP%s"), *TargetName);
	}

	/** Clean name of the plan color material asset */
	static const FString PlainColorMaterialAssetName;

	/** Clean name of the movie pipeline config asset */
	static const FString DefaultMoviePipelineConfigAssetName;

	/** Clean name of the movie pipeline config asset */
	static const FString PostProcessMaterialDirName;

	/**
	 * Specific project content utils
	 * Point to the content created by the plugin inside the specific project
	*/

	/**
	 * Plugin's working directory inside the specific project
	 * Can return the absolute path to the directory, or the in-game reference path
	*/
	static FString ProjectPluginContentDir(const bool bIsAbsolute = false)
	{
		const FString Prefix = (bIsAbsolute ? FPaths::ProjectContentDir() : TEXT("/Game"));
		return Prefix / PluginName;
	}

	/** Path to the project-specific texture mapping asset */
	static FString TextureMappingAssetPath()
	{
		return ProjectPluginContentDir() / TextureMappingAssetName;
	}

	/** Path to the project-specific widget state asset */
	static FString WidgetStateAssetPath()
	{
		return ProjectPluginContentDir() / WidgetStateAssetName;
	}

	/** Clean name of the texture mapping asset */
	static const FString TextureMappingAssetName;

	/** Clean name of the widget state asset */
	static const FString WidgetStateAssetName;

	/**
	 * Project output file saving utils
	*/

	/** Path to the default rendering output directory */
	static FString DefaultRenderingOutputPath()
	{
		return FPaths::ProjectSavedDir() / RenderingOutputDirName;
	}

	/** Full path to the camera rig ROS JSON file */
	static FString CameraRigFilePath(const FString& Directory)
	{
		return Directory / CameraRigFileName;
	}

	/** Full path to the semantic classes CSV file */
	static FString SemanticClassesFilePath(const FString& Directory)
	{
		return Directory / SemanticClassesFileName;
	}

	/** Gets original camera name from the received camera component */
	static FString GetCameraName(UCameraComponent* CameraComponent)
	{
		const FString CameraName = CameraComponent->GetReadableName();
		return CameraName.Right(CameraName.Len() - CameraName.Find(".") - 1);
	}

	/** Path to the specific rig camera output directory */
	static FString RigCameraDir(const FString& Directory, UCameraComponent* CameraComponent)
	{
		return Directory / GetCameraName(CameraComponent);
	}

	/** Full path to the camera poses output file */
	static FString CameraPosesFilePath(const FString& Directory, UCameraComponent* CameraComponent)
	{
		return RigCameraDir(Directory, CameraComponent) / CameraPosesFileName;
	}

	/** Full path to the camera rig poses output file */
	static FString CameraRigPosesFilePath(const FString& Directory)
	{
		return Directory / CameraPosesFileName;
	}

	/** Clean name of the rendering output directory */
	static const FString RenderingOutputDirName;

	/** Clean name of the camera rig ROS JSON output file */
	static const FString CameraRigFileName;

	/** Clean name of the semantic classes CSV output file */
	static const FString SemanticClassesFileName;

	/** Clean name of the camera poses output file */
	static const FString CameraPosesFileName;
};
