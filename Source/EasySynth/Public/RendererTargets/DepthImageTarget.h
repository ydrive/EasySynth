// Copyright (c) 2022 YDrive Inc. All rights reserved.

#pragma once

#include "CoreMinimal.h"

#include "RendererTargets/RendererTarget.h"

class UTextureStyleManager;


/**
 * Class responsible for updating the world properties before
 * the depth image target rendering and restoring them after the rendering
*/
class FDepthImageTarget : public FRendererTarget
{
public:
	explicit FDepthImageTarget(
		UTextureStyleManager* TextureStyleManager,
		const EImageFormat ImageFormat,
		const float DepthRangeMeters) :
			FRendererTarget(TextureStyleManager, ImageFormat),
			DepthRangeMeters(DepthRangeMeters)
	{}

	/** Returns the name of the target */
	virtual FString Name() const { return TEXT("DepthImage"); }

	/** Prepares the sequence for rendering the target */
	bool PrepareSequence(ULevelSequence* LevelSequence) override;

	/** Reverts changes made to the sequence by the PrepareSequence */
	bool FinalizeSequence(ULevelSequence* LevelSequence) override;

private:
	/** The clipping range meters when rendering the depth target */
	const float DepthRangeMeters;

	/** The name of the depth range meters material parameter */
	static const FString DepthRangeMetersParameter;
};
