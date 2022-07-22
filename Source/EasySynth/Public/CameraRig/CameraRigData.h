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
