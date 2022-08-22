// Copyright (c) 2022 YDrive Inc. All rights reserved.

#pragma once

#include "CoreMinimal.h"

#include "CameraRigRosInterface.generated.h"

class UCameraComponent;


/**
 * Structure representing the exact structure of single camera object inside of camera rig JSON file.
 * Member names are lover case, as they need to exactly match the JSON file content.
 */
USTRUCT()
struct FRosJsonCamera
{
	GENERATED_USTRUCT_BODY()

	/** Array representing camera intrinsics matrix, with the length of 9 */
	UPROPERTY()
	TArray<double> intrinsics;

	/** Array representing camera rotation quaternion, with the length of 4 */
	UPROPERTY()
	TArray<double> rotation;

	/** Array representing camera translation vector, with the length of 3 */
	UPROPERTY()
	TArray<double> translation;

	/** Array containing two numbers representing image width and height */
	UPROPERTY()
	TArray<double> sensor_size;
};


/**
 * Structure representing the exact structure of camera rig JSON files.
 * Member names are lover case, as they need to exactly match the JSON file content.
 */
USTRUCT()
struct FRosJsonContent
{
	GENERATED_USTRUCT_BODY()

	/**
	 * Map representing set of cameras inside a rig
	 */
	UPROPERTY()
	TMap<FString, FRosJsonCamera> cameras;
};


/**
 * Class containing interface methods for parsing and generating
 * camera rig ROS JSON files
 */
class FCameraRigRosInterface
{
public:
	FCameraRigRosInterface() {}

	/** Imports camera rig from a ROS JSON file */
	FReply OnImportCameraRigClicked();

	/** Exports camera rig into a ROS JSON file */
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
		FRosJsonContent& RosJsonContent);
};
