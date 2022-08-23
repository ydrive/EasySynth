// Copyright (c) 2022 YDrive Inc. All rights reserved.

#pragma once

#include "CoreMinimal.h"

#include "SequencerWrapper.h"

class UCameraComponent;
class ULevelSequence;


/**
 * Class which instance is used to export camera poses into a file.
 * An object of this class should be discarded when its job is done
*/
class FCameraPoseExporter
{
public:
	/**
	 * Export camera poses from the sequence to a file,
	 * to export rig poses, pass nullptr for the CameraComponent
	 */
	bool ExportCameraPoses(
		ULevelSequence* LevelSequence,
		const FIntPoint OutputImageResolution,
		const FString& OutputDir,
		UCameraComponent* CameraComponent);

private:
	/** Extract camera transforms using the sequencer wrapper */
	bool ExtractCameraTransforms(const bool bAccumulateCameraOffset);

	/** Saves the extracted camera poses to a file */
	bool SavePosesToCSV(const FString& FilePath);

	/** Sequencer wrapper needed to acces the level sequence properties */
	FSequencerWrapper SequencerWrapper;

	/** Resolution of output images */
	FIntPoint OutputResolution;

	/** Extracted camera pose transforms */
	TArray<FTransform> CameraTransforms;

	/** Frame timestamps */
	TArray<double> Timestamps;
};
