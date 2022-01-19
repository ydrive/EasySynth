// Copyright (c) 2022 YDrive Inc. All rights reserved.

#pragma once

#include "CoreMinimal.h"

#include "Engine/DataAsset.h"

#include "WidgetStateAsset.generated.h"


/**
 * An asset that stores the widget options state,
 * set the last time rendering was initiated
*/
UCLASS()
class EASYSYNTH_API UWidgetStateAsset : public UDataAsset
{
	GENERATED_BODY()

public:
	/** Selected level sequence asset */
	UPROPERTY(EditAnywhere, Category = "Level Sequence")
	FSoftObjectPath LevelSequenceAssetPath;

	/** Whether camera poses are selected */
	UPROPERTY(EditAnywhere, Category = "Rendering Targets")
	bool bCameraPosesSelected;

	/** Whether color images are selected */
	UPROPERTY(EditAnywhere, Category = "Rendering Targets")
	bool bColorImagesSelected;

	/** Output format for color images */
	UPROPERTY(EditAnywhere, Category = "Rendering Targets")
	int8 bColorImagesOutputFormat;

	/** Whether depth images are selected */
	UPROPERTY(EditAnywhere, Category = "Rendering Targets")
	bool bDepthImagesSelected;

	/** Output format for depth images */
	UPROPERTY(EditAnywhere, Category = "Rendering Targets")
	int8 bDepthImagesOutputFormat;

	/** Whether normal images are selected */
	UPROPERTY(EditAnywhere, Category = "Rendering Targets")
	bool bNormalImagesSelected;

	/** Output format for normal images */
	UPROPERTY(EditAnywhere, Category = "Rendering Targets")
	int8 bNormalImagesOutputFormat;

	/** Whether optical flow images are selected */
	UPROPERTY(EditAnywhere, Category = "Rendering Targets")
	bool bOpticalFlowImagesSelected;

	/** Output format for optical flow images */
	UPROPERTY(EditAnywhere, Category = "Rendering Targets")
	int8 bOpticalFlowImagesOutputFormat;

	/** Whether semantic images are selected */
	UPROPERTY(EditAnywhere, Category = "Rendering Targets")
	bool bSemanticImagesSelected;

	/** Output format for rendering images */
	UPROPERTY(EditAnywhere, Category = "Rendering Targets")
	int8 bSemanticImagesOutputFormat;

	/** Selected depth threashold range */
	UPROPERTY(EditAnywhere, Category = "Additional parameters")
	float DepthRange;

	/** Selected optical flow scale */
	UPROPERTY(EditAnywhere, Category = "Additional parameters")
	float OpticalFlowScale;

	/** Selected output image resolution */
	UPROPERTY(EditAnywhere, Category = "Additional parameters")
	FIntPoint OutputImageResolution;

	/** Selected output directory */
	UPROPERTY(EditAnywhere, Category = "Additional parameters")
	FString OutputDirectory;
};
