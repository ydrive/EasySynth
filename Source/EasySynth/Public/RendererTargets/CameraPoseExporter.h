// Copyright (c) 2022 YDrive Inc. All rights reserved.

#pragma once

#include "CoreMinimal.h"

#include "SequencerWrapper.h"

class ULevelSequence;


/**
 * Class which instance is used to export camera poses into a file.
 * An object of this class should be discarded when its job is done
*/
class FCameraPoseExporter
{
public:
	/** Export camera poses from the sequence to a file */
	bool ExportCameraPoses(
		ULevelSequence* LevelSequence,
		const FIntPoint OutputImageResolution,
		const FString& OutputDir,
		const int RigCameraId);

private:
	/** Extract camera transforms using the sequencer wrapper */
	bool ExtractCameraTransforms();

	/** Saves the extracted camera poses to a file */
	bool SavePosesToCSV(const FString& OutputDir, const int RigCameraId);

	/** Sequencer wrapper needed to acces the level sequence properties */
	FSequencerWrapper SequencerWrapper;

	/** Resolution of output images */
	FIntPoint OutputResolution;

	/** Extracted camera pose transforms */
	TArray<FTransform> CameraTransforms;

	/** Camera focal lengths for each image, expressed in pixel units */
	TArray<FVector2D> PixelFocalLengths;
};
