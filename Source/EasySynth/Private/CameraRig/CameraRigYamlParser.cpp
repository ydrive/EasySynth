// Copyright (c) 2022 YDrive Inc. All rights reserved.

#include "CameraRig/CameraRigYamlParser.h"

#include "GenericPlatform/GenericPlatformMath.h"
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

	int OldCursor = Cursor;
	int CameraId = 0;
	while (Cursor < PreprocessedInputString.Len())
	{
		FCameraRigData::FCameraData CameraData;
		if (!ParseCamera(PreprocessedInputString, Cursor, CameraId, CameraData))
		{
			ErrorMessage = FString::Printf(TEXT("Error while parsing camera %d\n%s"), CameraId, *ErrorMessage);
			return false;
		}
		OutCameraRigData.Cameras.Add(CameraData);

		if (Cursor == OldCursor)
		{
			ErrorMessage = "Terminating an infinite loop";
			UE_LOG(LogEasySynth, Error, TEXT("%s: %s"), *FString(__FUNCTION__), *ErrorMessage)
			return false;
		}
		OldCursor = Cursor;

		CameraId += 1;
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
			// Omit consecutive spaces, or spaces inside the []
			if (!bInsideBrackets && !bPreviousSpace)
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
	// TODO: Check valid id
	const int FirstSemi = InputString.Find(";", ESearchCase::IgnoreCase, ESearchDir::FromStart, Cursor);
	const int SecondSemi = InputString.Find(";", ESearchCase::IgnoreCase, ESearchDir::FromStart, FirstSemi + 1);

	if (InputString.Left(SecondSemi + 1) != "%YAML:1.0;---;")
	{
		ErrorMessage = "Invalid yaml file header";
		UE_LOG(LogEasySynth, Warning, TEXT("%s: %s"), *FString(__FUNCTION__), *ErrorMessage)
		return false;
	}

	Cursor = SecondSemi + 1;
	return true;
}

bool FCameraRigYamlParser::ParseCamera(
	const FString& InputString,
	int& Cursor,
	const int CameraId,
	FCameraRigData::FCameraData& OutCameraData)
{
	TArray<double> CameraMatrix;
	const FString CameraMatrixName = FString::Printf(TEXT("cameraMatrix%d"), CameraId);
	if (!ParseMatrix(InputString, Cursor, CameraMatrixName, 3, 3, CameraMatrix))
	{
		return false;
	}

	TArray<double> DistortionCoefficients;
	const FString DistortionCoefficientsName = FString::Printf(TEXT("distortionCoefficients%d"), CameraId);
	if (!ParseMatrix(InputString, Cursor, DistortionCoefficientsName, 14, 1, DistortionCoefficients))
	{
		return false;
	}

	TArray<double> TVec;
	const FString TVecName = FString::Printf(TEXT("t_vec%d"), CameraId);
	if (!ParseMatrix(InputString, Cursor, TVecName, 3, 1, TVec))
	{
		return false;
	}

	TArray<double> QVec;
	const FString QVecName = FString::Printf(TEXT("q_vec%d"), CameraId);
	if (!ParseMatrix(InputString, Cursor, QVecName, 4, 1, QVec))
	{
		return false;
	}

	// Store parsed values inside the camera data structure

	return true;
}

bool FCameraRigYamlParser::ParseMatrix(
	const FString& InputString,
	int& Cursor,
	const FString ExpectedName,
	const int ExpectedRows,
	const int ExpectedCols,
	TArray<double> OutValues)
{
	auto GenerateError = [&]() {
		ErrorMessage = FString::Printf(TEXT("Error while parsing matrix \"%s\"\n%s"), *ExpectedName, *ErrorMessage);
		return false;
	};

	const FString FirstLine = FString::Printf(TEXT("%s: !!opencv-matrix;"), *ExpectedName);
	if (!CheckStringLiteral(InputString, Cursor, FirstLine))
	{
		return GenerateError();
	}

	const FString SecondLine = FString::Printf(TEXT("rows: %d;"), ExpectedRows);
	if (!CheckStringLiteral(InputString, Cursor, SecondLine))
	{
		return GenerateError();
	}

	const FString ThirdLine = FString::Printf(TEXT("cols: %d;"), ExpectedCols);
	if (!CheckStringLiteral(InputString, Cursor, ThirdLine))
	{
		return GenerateError();
	}

	if (!CheckStringLiteral(InputString, Cursor, "dt: d;"))
	{
		return GenerateError();
	}

	if (!CheckStringLiteral(InputString, Cursor, "data: ["))
	{
		return GenerateError();
	}

	for (int i = 0; i < ExpectedRows * ExpectedCols - 1; i++)
	{
		// TODO: Check valid id
		const int NextComma = InputString.Find(",", ESearchCase::IgnoreCase, ESearchDir::FromStart, Cursor);
		UE_LOG(LogEasySynth, Warning, TEXT("%s: %d %d"), *FString(__FUNCTION__), Cursor, NextComma)
		const FString StringValue = InputString.Mid(Cursor, NextComma - Cursor);
		UE_LOG(LogEasySynth, Warning, TEXT("%s: %s"), *FString(__FUNCTION__), *StringValue)
		double Value = FCString::Atod(*StringValue);
		UE_LOG(LogEasySynth, Warning, TEXT("%s: %f"), *FString(__FUNCTION__), Value)
		OutValues.Add(Value);

		Cursor = NextComma + 1;
	}

	{
		// TODO: Check valid id
		const int ClosedBracket = InputString.Find("]", ESearchCase::IgnoreCase, ESearchDir::FromStart, Cursor);
		UE_LOG(LogEasySynth, Warning, TEXT("%s: %d %d"), *FString(__FUNCTION__), Cursor, ClosedBracket)
		const FString StringValue = InputString.Mid(Cursor, ClosedBracket - Cursor);
		UE_LOG(LogEasySynth, Warning, TEXT("%s: %s"), *FString(__FUNCTION__), *StringValue)
		double Value = FCString::Atod(*StringValue);
		UE_LOG(LogEasySynth, Warning, TEXT("%s: %f"), *FString(__FUNCTION__), Value)
		OutValues.Add(Value);

		Cursor = ClosedBracket;
	}

	if (!CheckStringLiteral(InputString, Cursor, "];"))
	{
		return GenerateError();
	}

	return true;
}

bool FCameraRigYamlParser::CheckStringLiteral(const FString& InputString, int& Cursor, const FString& ExpectedString)
{
	EatWhitespace(InputString, Cursor);

	if (Cursor + ExpectedString.Len() > InputString.Len() ||
		InputString.Mid(Cursor, ExpectedString.Len()) != ExpectedString)
	{
		const FString Context = InputString.Mid(
			Cursor,
			FGenericPlatformMath::Min(ExpectedString.Len(), InputString.Len() - Cursor));
		ErrorMessage = FString::Printf(TEXT("Could not match the expected line \"%s\", received \"%s\""), *ExpectedString, *Context);
		UE_LOG(LogEasySynth, Warning, TEXT("%s: %s"), *FString(__FUNCTION__), *ErrorMessage)
		return false;
	}

	Cursor += ExpectedString.Len();
	return true;
}

void FCameraRigYamlParser::EatWhitespace(const FString& InputString, int& Cursor)
{
	while (Cursor < InputString.Len() && (InputString[Cursor] == ' ' || InputString[Cursor] == '\t'))
	{
		Cursor++;
	}
}
