// Copyright (c) YDrive Inc. All rights reserved.
// Licensed under the MIT License.

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

	/** Whether EXR is selected for color images */
	UPROPERTY(EditAnywhere, Category = "Rendering Targets")
	bool bColorImagesExrSelected;

	/** Whether depth images are selected */
	UPROPERTY(EditAnywhere, Category = "Rendering Targets")
	bool bDepthImagesSelected;

	/** Whether EXR is selected for depth images */
	UPROPERTY(EditAnywhere, Category = "Rendering Targets")
	bool bDepthImagesExrSelected;

	/** Whether normal images are selected */
	UPROPERTY(EditAnywhere, Category = "Rendering Targets")
	bool bNormalImagesSelected;

	/** Whether EXR is selected for normal images */
	UPROPERTY(EditAnywhere, Category = "Rendering Targets")
	bool bNormalImagesExrSelected;

	/** Whether semantic images are selected */
	UPROPERTY(EditAnywhere, Category = "Rendering Targets")
	bool bSemanticImagesSelected;

	/** Whether EXR is selected for rendering images */
	UPROPERTY(EditAnywhere, Category = "Rendering Targets")
	bool bSemanticImagesExrSelected;

	/** Selected depth threashold range */
	UPROPERTY(EditAnywhere, Category = "Additional parameters")
	float DepthRange;

	/** Selected output image resolution */
	UPROPERTY(EditAnywhere, Category = "Additional parameters")
	FIntPoint OutputImageResolution;

	/** Selected output directory */
	UPROPERTY(EditAnywhere, Category = "Additional parameters")
	FString OutputDirectory;
};
