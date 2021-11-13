// Copyright Ydrive 2021

#include "RendererTargets/DepthImageTarget.h"

#include "Camera/CameraComponent.h"
#include "Materials/MaterialInstanceDynamic.h"

#include "LevelSequence.h"


const FString FDepthImageTarget::DepthRangeMetersParameter("DepthRangeMeters");

bool FDepthImageTarget::PrepareSequence(ULevelSequence* LevelSequence)
{
	// Get all camera components bound to the level sequence
	TArray<UCameraComponent*> Cameras = GetCameras(LevelSequence);
	if (Cameras.Num() == 0)
	{
		UE_LOG(LogEasySynth, Warning, TEXT("%s: No cameras bound to the level sequence found"), *FString(__FUNCTION__));
		return false;
	}

	// Prepare the camera post process material
	UMaterial* PostProcessMaterial = LoadPostProcessMatrial();
	if (PostProcessMaterial == nullptr)
	{
		UE_LOG(LogEasySynth, Error, TEXT("%s: Could not load depth post process material"), *FString(__FUNCTION__));
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
	PostProcessMaterialInstance->SetScalarParameterValue(*DepthRangeMetersParameter, DepthRange);

	for (UCameraComponent* Camera : Cameras)
	{
		if (Camera == nullptr)
		{
			UE_LOG(LogEasySynth, Error, TEXT("%s: Found camera is null"), *FString(__FUNCTION__));
			return false;
		}
		Camera->PostProcessSettings.WeightedBlendables.Array.Empty();
		Camera->PostProcessSettings.WeightedBlendables.Array.Add(FWeightedBlendable(1.0f, PostProcessMaterialInstance));
	}

	return true;
}

bool FDepthImageTarget::FinalizeSequence(ULevelSequence* LevelSequence)
{
	// Get all camera components bound to the level sequence
	TArray<UCameraComponent*> Cameras = GetCameras(LevelSequence);
	if (Cameras.Num() == 0)
	{
		UE_LOG(LogEasySynth, Warning, TEXT("%s: No cameras bound to the level sequence found"), *FString(__FUNCTION__));
		return false;
	}

	// Clear post process materials
	for (UCameraComponent* Camera : Cameras)
	{
		if (Camera == nullptr)
		{
			UE_LOG(LogEasySynth, Error, TEXT("%s: Found camera is null"), *FString(__FUNCTION__));
			return false;
		}
		Camera->PostProcessSettings.WeightedBlendables.Array.Empty();
	}

	return true;
}
