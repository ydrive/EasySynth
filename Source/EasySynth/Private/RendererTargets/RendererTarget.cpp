// Copyright (c) 2022 YDrive Inc. All rights reserved.

#include "RendererTargets/RendererTarget.h"

#include "ILevelSequenceEditorToolkit.h"
#include "ISequencer.h"
#include "LevelSequence.h"
#include "MovieScene.h"
#include "Sections/MovieSceneCameraCutSection.h"
#include "Subsystems/AssetEditorSubsystem.h"

#include "SequencerWrapper.h"


TArray<UCameraComponent*> FRendererTarget::GetCameras(ULevelSequence* LevelSequence)
{
	TArray<UCameraComponent*> Cameras;

	FSequencerWrapper SequencerWrapper;
	if (!SequencerWrapper.OpenSequence(LevelSequence))
	{
		UE_LOG(LogEasySynth, Error, TEXT("%s: Sequencer wrapper opening failed"), *FString(__FUNCTION__))
		return Cameras;
	}

	// Get the camera from each section
	TArray<UMovieSceneCameraCutSection*>& CutSections = SequencerWrapper.GetMovieSceneCutSections();
	for (auto CutSection : CutSections)
	{
		// Get the camera componenet
		UCameraComponent* Camera = CutSection->GetFirstCamera(
			*SequencerWrapper.GetSequencer(),
			SequencerWrapper.GetSequencer()->GetFocusedTemplateID());
		if (Camera == nullptr)
		{
			UE_LOG(LogEasySynth, Error, TEXT("%s: Cut section camera component is null"), *FString(__FUNCTION__))
			Cameras.Empty();
			return Cameras;
		}
		Cameras.Add(Camera);
	}

	return Cameras;
}

bool FRendererTarget::ClearCameraPostProcess(ULevelSequence* LevelSequence)
{
	// Get all camera components bound to the level sequence
	TArray<UCameraComponent*> Cameras = GetCameras(LevelSequence);
	if (Cameras.Num() == 0)
	{
		UE_LOG(LogEasySynth, Warning, TEXT("%s: No cameras bound to the level sequence found"), *FString(__FUNCTION__))
		return false;
	}

	// Clear post process materials
	for (UCameraComponent* Camera : Cameras)
	{
		if (Camera == nullptr)
		{
			UE_LOG(LogEasySynth, Error, TEXT("%s: Found camera is null"), *FString(__FUNCTION__))
			return false;
		}
		Camera->PostProcessSettings.WeightedBlendables.Array.Empty();
	}

	return true;
}
