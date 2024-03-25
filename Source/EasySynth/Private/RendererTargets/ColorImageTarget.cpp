// Copyright (c) 2022 YDrive Inc. All rights reserved.

#include "RendererTargets/ColorImageTarget.h"

#include "Camera/CameraComponent.h"

#include "LevelSequence.h"
#include "TextureStyles/TextureStyleManager.h"


bool FColorImageTarget::PrepareSequence(ULevelSequence* LevelSequence)
{
	// Update texture style inside the level
	TextureStyleManager->CheckoutTextureStyle(ETextureStyle::COLOR);

	// Get all camera components bound to the level sequence
	TArray<UCameraComponent*> Cameras = GetCameras(LevelSequence);
	if (Cameras.Num() == 0)
	{
		UE_LOG(LogEasySynth, Warning, TEXT("%s: No cameras bound to the level sequence found"), *FString(__FUNCTION__))
		return false;
	}

	// Prepare the camera post process material
	UMaterial* PostProcessMaterial = LoadPostProcessMaterial();
	if (PostProcessMaterial == nullptr)
	{
		UE_LOG(LogEasySynth, Error, TEXT("%s: Could not load color post process material"), *FString(__FUNCTION__))
		return false;
	}

	for (UCameraComponent* Camera : Cameras)
	{
		if (Camera == nullptr)
		{
			UE_LOG(LogEasySynth, Error, TEXT("%s: Found camera is null"), *FString(__FUNCTION__))
			return false;
		}

		Camera->PostProcessSettings.WeightedBlendables.Array.Empty();
		if (CustomPPMaterial == nullptr)
		{
			// If no custom post process material is provided, use the default one
			Camera->PostProcessSettings.WeightedBlendables.Array.Add(FWeightedBlendable(1.0f, PostProcessMaterial));
		}
		else
		{
			// If a non-null custom post process material is provided, use it instead of the default one
			Camera->PostProcessSettings.WeightedBlendables.Array.Add(FWeightedBlendable(1.0f, CustomPPMaterial));
		}
	}

	return true;
}

bool FColorImageTarget::FinalizeSequence(ULevelSequence* LevelSequence)
{
	return ClearCameraPostProcess(LevelSequence);
}
