// Copyright (c) 2022 YDrive Inc. All rights reserved.

#pragma once

#include "CoreMinimal.h"


/**
 * Structure that contains intrinsic and extrinsic information
 * for each camera inside the camera rig
 */
struct FCameraRigData
{
	/**
	 * Structure that contains information on a single camera
	 */
	struct FCameraData
	{
		FString CameraName;
		double FocalLength;
		double PrincipalPointX;
		double PrincipalPointY;
		FIntPoint SensorSize;
		FTransform Transform;
	};

	/**
	 * Array of cameras inside the camera rig
	 */
	TArray<FCameraData> Cameras;
};


/**
 * Class that provides access to the camera rig YAML file string literals
 */
class FYamlFileStructure
{
	// TODO: To YAML dir
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


/**
 * Class used to convert camera transforms (positions and locations) between desired
 * external coordinate system and UE internal coordinate system.
 *
 * UE coordinate system:
 * - X axis points straight away from the camera
 * - Y axis points to the right
 * - Z axis points up
 *
 * External coordinate system:
 * - X axis points to the right
 * - Y axis points down
 * - Z axis points straight away from the camera
 *
 * Rotations follow right-handed rule with respect to the defined axes.
 */
class FCoordinateSystemConverter
{
	// TODO: To global utility
public:
	/** Converts a transform from UE to external coordinate system */
	static void UEToExternal(
		const FTransform& UETransform,
		TArray<double>& OutLocation,
		TArray<double>& OutRotation,
		const bool bDoInverse = false)
	{
		const FTransform Transform = (bDoInverse ? UETransform.Inverse() : UETransform);

		// Convert from meters to centimeters
		const FVector Location = Transform.GetLocation() * 1.0e-2f;
		// Get rotation quaternion in the UE coordinate system
		const FQuat Rotation = Transform.GetRotation();

		OutLocation.Add(Location.Y);
		OutLocation.Add(-Location.Z);
		OutLocation.Add(Location.X);

		OutRotation.Add(Rotation.W);
		OutRotation.Add(-Rotation.Y);
		OutRotation.Add(Rotation.Z);
		OutRotation.Add(-Rotation.X);
	}

	/** Converts a transform from external to UE coordinate system */
	static FTransform ExternalToUE(
		const TArray<double>& Location,
		const TArray<double>& Rotation,
		const bool bDoInverse = false)
	{
		// Change the coordinate system and convert from meters to centimeters
		const FVector UETranslation = FVector(Location[2], Location[0], -Location[1]) * 100.0;
		// Calculate quaternion in the UE coordinate system
		const FQuat UERotation = FQuat(-Rotation[3], -Rotation[1], Rotation[2], Rotation[0]);

		FTransform UETransform;
		UETransform.SetTranslation(UETranslation);
		UETransform.SetRotation(UERotation);

		return (bDoInverse ? UETransform.Inverse() : UETransform);
	}
};
