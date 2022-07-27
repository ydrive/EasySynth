// Copyright (c) 2022 YDrive Inc. All rights reserved.

#pragma once

#include "CoreMinimal.h"


/**
 * Class used to convert camera transforms (positions and locations) between desired
 * external coordinate system and UE internal coordinate system.
 *
 * UE coordinate system (left-handed):
 * - X axis points straight away from the camera
 * - Y axis points to the right
 * - Z axis points up
 * - Translation uint is centimeters
 * - Quaternion sequence is XYZW
 *
 * External coordinate system (right-handed):
 * - X axis points straight away from the camera
 * - Y axis points to the left
 * - Z axis points up
 * - Translation uint is meters
 * - Quaternion sequence is WXYZ
 */
class FCoordinateSystemConverter
{
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

		OutLocation.Add(Location.X);
		OutLocation.Add(-Location.Y);
		OutLocation.Add(Location.Z);

		OutRotation.Add(Rotation.W);
		OutRotation.Add(-Rotation.X);
		OutRotation.Add(Rotation.Y);
		OutRotation.Add(-Rotation.Z);
	}

	/** Converts a transform from external to UE coordinate system */
	static FTransform ExternalToUE(
		const TArray<double>& Location,
		const TArray<double>& Rotation,
		const bool bDoInverse = false)
	{
		// Change the coordinate system and convert from meters to centimeters
		const FVector UETranslation = FVector(Location[0], -Location[1], Location[2]) * 100.0;
		// Calculate quaternion in the UE coordinate system
		const FQuat UERotation = FQuat(-Rotation[1], Rotation[2], -Rotation[3], Rotation[0]);

		FTransform UETransform;
		UETransform.SetTranslation(UETranslation);
		UETransform.SetRotation(UERotation);

		return (bDoInverse ? UETransform.Inverse() : UETransform);
	}
};
