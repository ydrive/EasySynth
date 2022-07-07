// Copyright (c) 2022 YDrive Inc. All rights reserved.

#include "CameraRig/CameraRigYamlParser.h"

#include "Misc/StringBuilder.h"


bool FCameraRigYamlParser::Parse(const FString& InputString, FCameraRigData& OutCameraRigData)
{
	ErrorMessage = "";

	FString PreprocessedInputString = PreprocessInput(InputString);

	int Cursor = 0;

	if (!ParseHeader(PreprocessedInputString, Cursor))
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
			// Mark brackets open
			bInsideBrackets = true;
			bPreviousSpace = false;
			bPreviousSemicolon = false;
			NewString.Add(Current);
		}
		else if (Current == ']')
		{
			// Mark brackets closed
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

	if (NewString.Last() != ';')
	{
		NewString.Add(';');
	}

	// Add string termination
	NewString.Add(0);

	return FString(ANSI_TO_TCHAR(NewString.GetData()));
}

bool FCameraRigYamlParser::ParseHeader(const FString& InputString, int& Cursor)
{
	// Parse the first two lines
	const int FirstSemi = InputString.Find(";", ESearchCase::IgnoreCase, ESearchDir::FromStart, Cursor);
	const int SecondSemi = InputString.Find(";", ESearchCase::IgnoreCase, ESearchDir::FromStart, FirstSemi + 1);
	UE_LOG(LogEasySynth, Error, TEXT("%s: %d %d"), *FString(__FUNCTION__), FirstSemi, SecondSemi)

	if (InputString.Left(SecondSemi + 1) != "%YAML:1.0;---;")
	{
		ErrorMessage = "Invalid yaml file header";
		UE_LOG(LogEasySynth, Error, TEXT("%s: %s"), *FString(__FUNCTION__), *ErrorMessage)
		return false;
	}

	Cursor = SecondSemi + 1;
	return true;
}
