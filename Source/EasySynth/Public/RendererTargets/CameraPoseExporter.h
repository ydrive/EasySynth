// Copyright (c) YDrive Inc. All rights reserved.
// Licensed under the MIT License.

#pragma once

#include "CoreMinimal.h"

class ULevelSequence;


class FCameraPoseExporter
{
public:
	/** Export camera poses from the sequence to a file */
	bool ExportCameraPoses(ULevelSequence* LevelSequence);

private:
};
