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

	/** Returns the error message */
	const FString& GetErrorMessage() const { return ErrorMessage; }

private:
	/** Initializes the parser with a new input string */
	void InitializeParser(const FString& InputString);

	/** Stores the input string */
	FString ParsingString;

	/** Stores the error message */
	FString ErrorMessage;
};
