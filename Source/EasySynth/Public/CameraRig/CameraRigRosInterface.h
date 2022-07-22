// Copyright (c) 2022 YDrive Inc. All rights reserved.

#pragma once

#include "CoreMinimal.h"

class UCameraComponent;


/**
 * Class containing interface methods for parsing and generating
 * camera rig ROS JSON files
 */
class FCameraRigRosInterface
{
public:
	FCameraRigRosInterface() {}

	/** Handles importing camera ring from a ROS JSON file */
	FReply OnImportCameraRigClicked();

	/** Handles exporting camera rig into a ROS JSON file */
	bool ExportCameraRig(
		const FString& OutputDir,
		TArray<UCameraComponent*> RigCameras,
		const FIntPoint& SensorSize);

private:
	/** Adds lines describing a single camera to the output array */
	void AddCamera(
		const int CameraId,
		UCameraComponent* Camera,
		const FIntPoint& SensorSize,
		const bool bAddComma,
		TArray<FString>& OutLines);
};
