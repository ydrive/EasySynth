// Copyright (c) 2022 YDrive Inc. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "IImageWrapper.h"

#include "PathUtils.h"

class UCameraComponent;
class ULevelSequence;

class UTextureStyleManager;


/**
 * Base class for renderer targets responsible for updating the
 * world properties before a specific target rendering and
 * restoring them after the rendering
*/
class FRendererTargetSet
{
public:
	explicit FRendererTargetSet(UTextureStyleManager* TextureStyleManager, const EImageFormat ImageFormat) :
		ImageFormat(ImageFormat),
		TextureStyleManager(TextureStyleManager)
	{}

	/** Returns a name of the target set*/
	virtual FString Name() const = 0;

	/** Returns a name of targets in the set */
	virtual TArray<FString> TargetNames() const = 0;

	/** Prepares the sequence for rendering a specific target */
	virtual bool PrepareSequence(ULevelSequence* LevelSequence) = 0;

	/** Reverts changes made to the sequence by the PrepareSequence */
	virtual bool FinalizeSequence(ULevelSequence* LevelSequence) = 0;

	/** Output image format selected for this target */
	const EImageFormat ImageFormat;

protected:
	/** Extracts camera components used by the level sequence */
	TArray<UCameraComponent*> GetCameras(ULevelSequence* LevelSequence);

	/** Removes renderer target specific post-process materials */
	bool ClearCameraPostProcess(ULevelSequence* LevelSequence);

	/** Returns paths to post process materials for each target */
	inline TArray<UMaterial*> LoadPostProcessMaterials() const
	{
		TArray<UMaterial*> Materials;
		for (int i = 0; i < TargetNames().Num(); i++)
		{
			Materials.Add(
				DuplicateObject<UMaterial>(
					LoadObject<UMaterial>(
						nullptr,
						*FPathUtils::PostProcessMaterialPath(TargetNames()[i])),
					nullptr));
		}
	}

	/** Handle for managing texture style in the level */
	UTextureStyleManager* TextureStyleManager;
};
