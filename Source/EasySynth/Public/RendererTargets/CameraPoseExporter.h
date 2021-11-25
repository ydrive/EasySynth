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
	bool ExportCameraPoses(ULevelSequence* LevelSequence);

private:
	/** Extract camera transforms using the sequencer wrapper */
	bool ExtractCameraTransforms();

	/** Sequencer wrapper needed to acces the level sequence properties */
	FSequencerWrapper SequencerWrapper;

	/** Extracted camera pose transforms */
	TArray<FTransform> CameraTransforms;
};
