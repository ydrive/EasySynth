// Copyright (c) 2024 YDrive Inc. All rights reserved.

#include "RendererTargets/CustomPPMaterialTarget.h"

#include "Camera/CameraComponent.h"
#include "LevelSequence.h"

#include "EasySynth.h"
#include "TextureStyles/TextureStyleManager.h"


bool FCustomPPMaterialTarget::PrepareSequence(ULevelSequence* LevelSequence)
{
	// Update texture style inside the level
	TextureStyleManager->CheckoutTextureStyle(ETextureStyle::COLOR);

	// Make sure the custom post process material is not null
	if (CustomPPMaterial == nullptr)
	{
		UE_LOG(LogEasySynth, Error, TEXT("%s: Custom post process material is null"), *FString(__FUNCTION__))
		return false;
	}

	// Get all camera components bound to the level sequence
	TArray<UCameraComponent*> Cameras = GetCameras(LevelSequence);
	if (Cameras.Num() == 0)
	{
		UE_LOG(LogEasySynth, Warning, TEXT("%s: No cameras bound to the level sequence found"), *FString(__FUNCTION__))
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
		Camera->PostProcessSettings.WeightedBlendables.Array.Add(FWeightedBlendable(1.0f, CustomPPMaterial));
	}

	return true;
}

bool FCustomPPMaterialTarget::FinalizeSequence(ULevelSequence* LevelSequence)
{
	return ClearCameraPostProcess(LevelSequence);
}
