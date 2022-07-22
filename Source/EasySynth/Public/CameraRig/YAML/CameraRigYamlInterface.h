// Copyright (c) 2022 YDrive Inc. All rights reserved.

#pragma once

#include "CoreMinimal.h"

class UCameraComponent;


/**
 * Class containing interface methods for parsing and generating
 * camera rig yaml files
 */
class FCameraRigYamlInterface
{
public:
	FCameraRigYamlInterface() {}

	/** Handles importing camera ring from a yaml file */
	FReply OnImportCameraRigClicked();

	/** Handles exporting camera rig into a yaml file */
	bool ExportCameraRig(const FString& OutputDir, TArray<UCameraComponent*> RigCameras);

private:
	/** Adds lines describing a single camera to the output array */
	void AddCamera(const int CameraId, UCameraComponent* Camera, TArray<FString>& OutLines);

	/** Adds lines describing a single OpenCV matrix to the output array */
	void AddMatrix(
		const FString& MatrixName,
		const int Rows,
		const int Cols,
		const TArray<double>& Values,
		TArray<FString>& OutLines);
};
