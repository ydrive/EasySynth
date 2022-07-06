// Copyright (c) 2022 YDrive Inc. All rights reserved.

#include "CameraRig/CameraRigYamlParser.h"


bool FCameraRigYamlParser::Parse(const FString& InputString, FCameraRigData& OutCameraRigData)
{
	InitializeParser(InputString);

	if (false)
	{
		ErrorMessage = "Some error";
		UE_LOG(LogEasySynth, Warning, TEXT("%s: %s"), *FString(__FUNCTION__), *ErrorMessage)
		return false;
	}

	return true;
}

void FCameraRigYamlParser::InitializeParser(const FString& InputString)
{
	ParsingString = InputString;
}
