// Copyright (c) 2022 YDrive Inc. All rights reserved.

#include "RendererTargets/DepthImageTarget.h"

#include "Camera/CameraComponent.h"
#include "Materials/MaterialInstanceDynamic.h"

#include "LevelSequence.h"
#include "TextureStyles/TextureStyleManager.h"


const FString FDepthImageTarget::DepthRangeMetersParameter("DepthRangeMeters");

bool FDepthImageTarget::PrepareSequence(ULevelSequence* LevelSequence)
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
		UE_LOG(LogEasySynth, Error, TEXT("%s: Could not load depth post process material"), *FString(__FUNCTION__))
		return false;
	}

	// Create the material instance and set the range parameter
	UMaterialInstanceDynamic* PostProcessMaterialInstance =
		UMaterialInstanceDynamic::Create(PostProcessMaterial, nullptr);
	if (PostProcessMaterialInstance == nullptr)
	{
		UE_LOG(LogTemp, Error, TEXT("%s: Could not create the material instance dynamic"), *FString(__FUNCTION__))
		return false;
	}
	PostProcessMaterialInstance->SetScalarParameterValue(*DepthRangeMetersParameter, DepthRangeMeters);

	for (UCameraComponent* Camera : Cameras)
	{
		if (Camera == nullptr)
		{
			UE_LOG(LogEasySynth, Error, TEXT("%s: Found camera is null"), *FString(__FUNCTION__))
			return false;
		}
		Camera->PostProcessSettings.WeightedBlendables.Array.Empty();
		Camera->PostProcessSettings.WeightedBlendables.Array.Add(FWeightedBlendable(1.0f, PostProcessMaterialInstance));
	}

	return true;
}

bool FDepthImageTarget::FinalizeSequence(ULevelSequence* LevelSequence)
{
	return ClearCameraPostProcess(LevelSequence);
}
