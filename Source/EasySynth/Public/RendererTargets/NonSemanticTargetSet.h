// Copyright (c) 2022 YDrive Inc. All rights reserved.
#pragma once

#include "CoreMinimal.h"

#include "RendererTargets/RendererTargetSet.h"

class UTextureStyleManager;


/**
 * Class responsible for updating the world properties before
 * the color image target rendering and restoring them after the rendering
*/
class FNonSemanticTargetSet : public FRendererTargetSet
{
public:
	explicit FNonSemanticTargetSet(
		UTextureStyleManager* TextureStyleManager,
		const EImageFormat ImageFormat,
		const float DepthRangeMeters,
		const float OpticalFlowScale,
		const bool RenderDepth,
		const bool RenderNormals,
		const bool RenderOpticalFlow) :
			FRendererTargetSet(TextureStyleManager, ImageFormat),
		    DepthRangeMeters(DepthRangeMeters),
			OpticalFlowScale(OpticalFlowScale),
			RenderDepth(RenderDepth),
			RenderNormals(RenderNormals),
			RenderOpticalFlow(RenderOpticalFlow)
	{}


	/** Returns a name of the target set*/
	virtual FString Name() const
	{
		return TEXT("NonSemanticTargetSet");
	}

	/** Returns the names of the targets to be rendered*/
	virtual TArray<FString> TargetNames() const
	{
		TArray<FString> TargetNames;
		TargetNames.Add(TEXT("ColorImage"));
		if (RenderDepth)
		{
			TargetNames.Add(TEXT("DepthImage"));
		}
		if (RenderNormals)
		{
			TargetNames.Add(TEXT("NormalImage"));
		}
		if (RenderOpticalFlow)
		{
			TargetNames.Add(TEXT("OpticalFlowImage"));
		}
		return TargetNames;
	}

	/** Prepares the sequence for rendering the target */
	bool PrepareSequence(ULevelSequence* LevelSequence) override;

	/** Reverts changes made to the sequence by the PrepareSequence */
	bool FinalizeSequence(ULevelSequence* LevelSequence) override;

private:
	/** The name of the depth range meters material parameter */
	static const FString DepthRangeMetersParameter;

	/** The name of the optical flow scale material parameter */
	static const FString OpticalFlowScaleParameter;

	/** The clipping range meters when rendering the depth target */
	const float DepthRangeMeters;

	/** The scaling coefficient for increasing the saturation of optical flow images */
	const float OpticalFlowScale;
		
	/** Which targets will be rendered besides color image */
	const bool RenderDepth;
	const bool RenderNormals;
	const bool RenderOpticalFlow;
};
