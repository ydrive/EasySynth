// Copyright (c) 2022 YDrive Inc. All rights reserved.

#pragma once

#include "CoreMinimal.h"

#include "CameraRig/CameraRigData.h"


/**
 * Class responsible for parsing camera rig yaml string
 */
class FCameraRigYamlParser
{
public:
	/** Entry method for parsing the yaml string */
	bool Parse(const FString& InputString, FCameraRigData& OutCameraRigData);

	/** Returns the latest error message */
	const FString& GetErrorMessage() const { return ErrorMessage; }

private:
	/** Formats the input text so it is easier to parse */
	FString PreprocessInput(const FString InputString);

	/** Parses the file header */
	bool ParseHeader(const FString& InputString, int& Cursor);

	/** Stores the latest error message */
	FString ErrorMessage;
};
