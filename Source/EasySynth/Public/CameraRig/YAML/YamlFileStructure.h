// Copyright (c) 2022 YDrive Inc. All rights reserved.

#pragma once

#include "CoreMinimal.h"


/**
 * Class that provides access to the camera rig YAML file string literals
 */
class FYamlFileStructure
{
public:
	/**
	 * Component names
	 */

	/** Returns the camera matrix component name given the camera id */
	static FString CameraMatrixName(const int CameraId)
	{
		return FString::Printf(TEXT("cameraMatrix%d"), CameraId);
	}

	/** Returns the distortion coefficients component name given the camera id */
	static FString DistortionCoeffName(const int CameraId)
	{
		return FString::Printf(TEXT("distortionCoefficients%d"), CameraId);
	}

	/** Returns the t_vec component name given the camera id */
	static FString TVecName(const int CameraId)
	{
		return FString::Printf(TEXT("t_vec%d"), CameraId);
	}

	/** Returns the q_vec component name given the camera id */
	static FString QVecName(const int CameraId)
	{
		return FString::Printf(TEXT("q_vec%d"), CameraId);
	}

	/**
	 * Line templates
	 */

	/** Returns the OpenCV matrix name line */
	static FString NameLine(const FString& Name, const bool bForParser = false)
	{
		return FString::Printf(TEXT("%s: !!opencv-matrix%s"), *Name, *Semi(bForParser));
	}

	/** Returns the rows line */
	static FString RowsLine(const int Rows, const bool bForParser = false)
	{
		return FString::Printf(TEXT("%srows: %d%s"), *Spaces(bForParser), Rows, *Semi(bForParser));
	}

	/** Returns the cols line */
	static FString ColsLine(const int Cols, const bool bForParser = false)
	{
		return FString::Printf(TEXT("%scols: %d%s"), *Spaces(bForParser), Cols, *Semi(bForParser));
	}

	/** Returns the dt line */
	static FString DTLine(const bool bForParser = false)
	{
		return FString::Printf(TEXT("%sdt: d%s"), *Spaces(bForParser), *Semi(bForParser));
	}

	/** Returns the data line start */
	static FString DataLineStart(const bool bForParser = false)
	{
		return FString::Printf(TEXT("%sdata: ["), *Spaces(bForParser));
	}

	/** Returns the data line end */
	static FString DataLineEnd(const bool bForParser = false)
	{
		return FString::Printf(TEXT("]%s"), *Semi(bForParser));
	}

private:
	/** Insert spaces if needed */
	static FString Spaces(const bool bForParser)
	{
		return (bForParser ? "" : "   ");
	}

	/** Insert semicolon if needed */
	static FString Semi(const bool bForParser)
	{
		return (bForParser ? ";" : "");
	}
};
