// Copyright (c) 2022 YDrive Inc. All rights reserved.

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

	/** Reverts changes made to the sequence by PrepareSequence */
	bool FinalizeSequence(ULevelSequence* LevelSequence) override;

private:
	/** The scaling coefficient for increasing the saturation of optical flow images */
	const float OpticalFlowScale;

	/** The name of the optical flow scale material parameter */
	static const FString OpticalFlowScaleParameter;
};
