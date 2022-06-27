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
class FRendererTarget
{
public:
	explicit FRendererTarget(UTextureStyleManager* TextureStyleManager, const EImageFormat ImageFormat) :
		ImageFormat(ImageFormat),
		TextureStyleManager(TextureStyleManager)
	{}

	/** Returns a name of a specific target */
	virtual FString Name() const = 0;

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

	/** Returns the path to the specific target post process material */
	inline UMaterial* LoadPostProcessMaterial() const
	{
		return DuplicateObject<UMaterial>(
			LoadObject<UMaterial>(nullptr, *FPathUtils::PostProcessMaterialPath(Name())), nullptr);
	}

	/** Handle for managing texture style in the level */
	UTextureStyleManager* TextureStyleManager;
};
