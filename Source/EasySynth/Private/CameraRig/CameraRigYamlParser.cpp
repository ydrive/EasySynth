// Copyright (c) 2022 YDrive Inc. All rights reserved.

#include "CameraRig/CameraRigYamlParser.h"

#include "Misc/StringBuilder.h"


bool FCameraRigYamlParser::Parse(const FString& InputString, FCameraRigData& OutCameraRigData)
{
	ErrorMessage = "";

	FString PreprocessedInputString = PreprocessInput(InputString);

	int Cursor = 0;

	if (!ParseHeader(InputString, Cursor))
	{
		return false;
	}

	return true;
}

FString FCameraRigYamlParser::PreprocessInput(const FString InputString)
{
	TArray<char> NewString;

	bool bInsideBrackets = false;
	bool bPreviousSpace = false;
	bool bPreviousSemicolon = false;
	for (int i = 0; i < InputString.Len(); i++)
	{
		const char Current = InputString[i];
		if (Current == ' ')
		{
			// Remove consecutive spaces
			if (!bPreviousSpace)
			{
				bPreviousSpace = true;
				bPreviousSemicolon = false;
				NewString.Add(Current);
			}
		}
		else if (Current == '[')
		{
			bInsideBrackets = true;
			bPreviousSpace = false;
			bPreviousSemicolon = false;
			NewString.Add(Current);
		}
		else if (Current == ']')
		{
			bInsideBrackets = false;
			bPreviousSpace = false;
			bPreviousSemicolon = false;
			NewString.Add(Current);
		}
		else if (Current == '\n')
		{
			// Replace new lines with ;
			// Omit the ; inside the []
			if (!bInsideBrackets && !bPreviousSemicolon)
			{
				bPreviousSpace = false;
				bPreviousSemicolon = true;
				NewString.Add(';');
			}
		}
		else if (Current == '\r')
		{
			// Just ignore
		}
		else
		{
			// Copy all other characters
			bPreviousSpace = false;
			bPreviousSemicolon = false;
			NewString.Add(Current);
		}
	}

	// Add string termination
	NewString.Add(0);

	return FString(ANSI_TO_TCHAR(NewString.GetData()));
}

bool FCameraRigYamlParser::ParseHeader(const FString& InputString, int& Cursor)
{
	return true;
}
