// Copyright (c) YDrive Inc. All rights reserved.
// Licensed under the MIT License.

#pragma once

#include "CoreMinimal.h"

#include "PathUtils.h"

class UCameraComponent;
class ULevelSequence;


/**
 * Base class for renderer targets responsible for updating the
 * world properties before a specific target rendering and
 * restoring them after the rendering
*/
class FRendererTarget
{
public:
	/** Returns a name of a specific target */
	virtual FString Name() const = 0;

	/** Prepares the sequence for rendering a specific target */
	virtual bool PrepareSequence(ULevelSequence* LevelSequence) = 0;

	/** Reverts changes made to the sequence by the PrepareSequence */
	virtual bool FinalizeSequence(ULevelSequence* LevelSequence) = 0;

protected:
	/** Extracts camera components used by the level sequence */
	TArray<UCameraComponent*> GetCameras(ULevelSequence* LevelSequence);

	/** Removes renderer target specific post-process materials */
	bool ClearCameraPostProcess(ULevelSequence* LevelSequence);

	/** Returns the path to the specific target post process material */
	inline UMaterial* LoadPostProcessMatrial() const
	{
		return LoadObject<UMaterial>(nullptr, *FPathUtils::PostProcessMaterialPath(Name()));
	}
};
