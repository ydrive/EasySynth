// Copyright (c) YDrive Inc. All rights reserved.
// Licensed under the MIT License.

#pragma once

#include "CoreMinimal.h"

#include "RendererTargets/RendererTarget.h"

class UTextureStyleManager;


/**
 * Class responsible for updating the world properties before
 * the optical flow image target rendering and restoring them after the rendering
*/
class FOpticalFlowImageTarget : public FRendererTarget
{
public:
	explicit FOpticalFlowImageTarget(
		UTextureStyleManager* TextureStyleManager,
		const EImageFormat ImageFormat,
		const float OpticalFlowScale) :
			FRendererTarget(TextureStyleManager, ImageFormat),
			OpticalFlowScale(OpticalFlowScale)
	{}

	/** Returns the name of the target */
	virtual FString Name() const { return TEXT("OpticalFlowImage"); }

	/** Prepares the sequence for rendering the target */
	bool PrepareSequence(ULevelSequence* LevelSequence) override;

	/** Reverts changes made to the sequence by the PrepareSequence */
	bool FinalizeSequence(ULevelSequence* LevelSequence) override;

private:
	/** The clipping range meters when rendering the depth target */
	const float OpticalFlowScale;

	/** The name of the depth range meters material parameter */
	static const FString OpticalFlowScaleParameter;
};
