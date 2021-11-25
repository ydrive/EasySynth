// Copyright (c) YDrive Inc. All rights reserved.
// Licensed under the MIT License.

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
	bool ExportCameraPoses(ULevelSequence* LevelSequence, const FString& OutputDir);

private:
	/** Extract camera transforms using the sequencer wrapper */
	bool ExtractCameraTransforms();

	/** Saves the estracted camera poses to a file */
	bool SavePosesToFile(const FString& OutputDir);

	/** Sequencer wrapper needed to acces the level sequence properties */
	FSequencerWrapper SequencerWrapper;

	/** Extracted camera pose transforms */
	TArray<FTransform> CameraTransforms;
};
