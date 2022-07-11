// Copyright (c) 2022 YDrive Inc. All rights reserved.

#pragma once

#include "CoreMinimal.h"

#include "CameraRig/CameraRigUtils.h"


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

	/** Parses single camera information */
	bool ParseCamera(
		const FString& InputString,
		int& Cursor,
		const int CameraId,
		FCameraRigData::FCameraData& OutCameraData);

	/** Parses single OpenCV matrix information */
	bool ParseMatrix(
		const FString& InputString,
		int& Cursor,
		const FString ExpectedName,
		const int ExpectedRows,
		const int ExpectedCols,
		TArray<double>& OutValues);

	/** Checks if the following characters match the received string */
	bool CheckStringLiteral(const FString& InputString, int& Cursor, const FString& ExpectedString);

	/** Skips all potential whitespaces */
	void EatWhitespace(const FString& InputString, int& Cursor);

	/** Stores the latest error message */
	FString ErrorMessage;
};
