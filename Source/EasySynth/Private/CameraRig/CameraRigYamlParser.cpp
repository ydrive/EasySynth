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
	const int32 FirstSemi = InputString.Find(";", ESearchCase::IgnoreCase, ESearchDir::FromStart, Cursor);
	if (FirstSemi == INDEX_NONE)
	{
		ErrorMessage = "First new line not found";
		UE_LOG(LogEasySynth, Warning, TEXT("%s: %s"), *FString(__FUNCTION__), *ErrorMessage)
		return false;
	}

	const int32 SecondSemi = InputString.Find(";", ESearchCase::IgnoreCase, ESearchDir::FromStart, FirstSemi + 1);
	if (SecondSemi == INDEX_NONE)
	{
		ErrorMessage = "Second new line not found";
		UE_LOG(LogEasySynth, Warning, TEXT("%s: %s"), *FString(__FUNCTION__), *ErrorMessage)
		return false;
	}

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
	if (!ParseMatrix(InputString, Cursor, FYamlFileStructure::CameraMatrixName(CameraId), 3, 3, CameraMatrix))
	{
		return false;
	}

	TArray<double> DistortionCoeff;
	if (!ParseMatrix(InputString, Cursor, FYamlFileStructure::DistortionCoeffName(CameraId), 14, 1, DistortionCoeff))
	{
		return false;
	}

	TArray<double> TVec;
	if (!ParseMatrix(InputString, Cursor, FYamlFileStructure::TVecName(CameraId), 3, 1, TVec))
	{
		return false;
	}

	TArray<double> QVec;
	if (!ParseMatrix(InputString, Cursor, FYamlFileStructure::QVecName(CameraId), 4, 1, QVec))
	{
		return false;
	}

	// Apply needed transformations to the loaded translation and location
	const bool bDoInverse = true;
	OutCameraData.Transform = FCoordinateSystemConverter::ExternalToUE(TVec, QVec, bDoInverse);

	return true;
}

bool FCameraRigYamlParser::ParseMatrix(
	const FString& InputString,
	int& Cursor,
	const FString ExpectedName,
	const int ExpectedRows,
	const int ExpectedCols,
	TArray<double>& OutValues)
{
	auto GenerateError = [&]() {
		ErrorMessage = FString::Printf(TEXT("Error while parsing matrix \"%s\"\n%s"), *ExpectedName, *ErrorMessage);
		return false;
	};

	const bool bForParser = true;

	if (!CheckStringLiteral(InputString, Cursor, FYamlFileStructure::NameLine(ExpectedName, bForParser)))
	{
		return GenerateError();
	}

	if (!CheckStringLiteral(InputString, Cursor, FYamlFileStructure::RowsLine(ExpectedRows, bForParser)))
	{
		return GenerateError();
	}

	if (!CheckStringLiteral(InputString, Cursor, FYamlFileStructure::ColsLine(ExpectedCols, bForParser)))
	{
		return GenerateError();
	}

	if (!CheckStringLiteral(InputString, Cursor, FYamlFileStructure::DTLine(bForParser)))
	{
		return GenerateError();
	}

	if (!CheckStringLiteral(InputString, Cursor, FYamlFileStructure::DataLineStart(bForParser)))
	{
		return GenerateError();
	}

	for (int i = 0; i < ExpectedRows * ExpectedCols - 1; i++)
	{
		// Parse all matrix values (except the last one)
		const int32 NextComma = InputString.Find(",", ESearchCase::IgnoreCase, ESearchDir::FromStart, Cursor);
		if (NextComma == INDEX_NONE)
		{
			ErrorMessage = FString::Printf(TEXT("Comma number %d not found while parsing matrix data"), i + 1);
			UE_LOG(LogEasySynth, Warning, TEXT("%s: %s"), *FString(__FUNCTION__), *ErrorMessage)
			return false;
		}

		const FString StringValue = InputString.Mid(Cursor, NextComma - Cursor);
		double Value = FCString::Atod(*StringValue);
		OutValues.Add(Value);

		Cursor = NextComma + 1;
	}

	{
		// Parse the last matrix value
		const int32 ClosedBracket = InputString.Find("]", ESearchCase::IgnoreCase, ESearchDir::FromStart, Cursor);
		if (ClosedBracket == INDEX_NONE)
		{
			ErrorMessage = "Closed bracket not found while parsing matrix data";
			UE_LOG(LogEasySynth, Warning, TEXT("%s: %s"), *FString(__FUNCTION__), *ErrorMessage)
			return false;
		}

		const FString StringValue = InputString.Mid(Cursor, ClosedBracket - Cursor);
		double Value = FCString::Atod(*StringValue);
		OutValues.Add(Value);

		Cursor = ClosedBracket;
	}

	if (!CheckStringLiteral(InputString, Cursor, FYamlFileStructure::DataLineEnd(bForParser)))
	{
		return GenerateError();
	}

	return true;
}

bool FCameraRigYamlParser::CheckStringLiteral(const FString& InputString, int& Cursor, const FString& ExpectedString)
{
	EatWhitespace(InputString, Cursor);

	// Check if the following InputString character exactly match the ExpectedString
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
