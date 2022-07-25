// Copyright (c) 2022 YDrive Inc. All rights reserved.

#include "SequencerWrapper.h"

#include "ILevelSequenceEditorToolkit.h"
#include "ISequencer.h"
#include "LevelSequence.h"
#include "MovieScene.h"
#include "Sections/MovieSceneCameraCutSection.h"
#include "Subsystems/AssetEditorSubsystem.h"


bool FSequencerWrapper::OpenSequence(ULevelSequence* LevelSequence)
{
	// Get the root level sequence movie scene
	MovieScene = LevelSequence->GetMovieScene();
	if (MovieScene == nullptr)
	{
		UE_LOG(LogEasySynth, Error, TEXT("%s: Could not get the movie scene from the level sequence"),
			*FString(__FUNCTION__))
		return false;
	}

	// Get the camera track from the movie scene
	CameraCutTrack = MovieScene->GetCameraCutTrack();
	if (CameraCutTrack == nullptr)
	{
		UE_LOG(LogEasySynth, Error, TEXT("%s: Could not get the camera cut track from the movie scene"),
			*FString(__FUNCTION__))
		return false;
	}

	GetMovieSceneCutSections();

	// Open sequencer editor for the level sequence asset
	TArray<UObject*> Assets;
	Assets.Add(LevelSequence);
	if (!GEditor->GetEditorSubsystem<UAssetEditorSubsystem>()->OpenEditorForAssets(Assets))
	{
		UE_LOG(LogEasySynth, Error, TEXT("%s: Could not open the level sequence editor"), *FString(__FUNCTION__))
		return false;
	}

	// Get the opened LevelSequenceEditor
	IAssetEditorInstance* AssetEditor =
		GEditor->GetEditorSubsystem<UAssetEditorSubsystem>()->FindEditorForAsset(LevelSequence, false);
	if (AssetEditor == nullptr)
	{
		UE_LOG(LogEasySynth, Error, TEXT("%s: Could not find the asset editor"), *FString(__FUNCTION__))
		return false;
	}

	// Confirm we received the right editor
	ILevelSequenceEditorToolkit* LevelSequenceEditor = static_cast<ILevelSequenceEditorToolkit*>(AssetEditor);
	if (LevelSequenceEditor == nullptr)
	{
		UE_LOG(LogEasySynth, Error, TEXT("%s: Could not find the level sequence editor"), *FString(__FUNCTION__))
		return false;
	}

	// Get the Sequencer
	WeakSequencer = LevelSequenceEditor ? LevelSequenceEditor->GetSequencer() : nullptr;
	if (!WeakSequencer.IsValid())
	{
		UE_LOG(LogEasySynth, Error, TEXT("%s: Could not get the sequencer"), *FString(__FUNCTION__))
		return false;
	}

	return true;
}

TArray<UMovieSceneCameraCutSection*>& FSequencerWrapper::GetMovieSceneCutSections()
{
	MovieSceneCutSections.Empty();

	// Get all sections of the camera track
	TArray<UMovieSceneSection*> MovieSceneSections = CameraCutTrack->GetAllSections();
	if (MovieSceneSections.Num() == 0)
	{
		UE_LOG(LogEasySynth, Warning, TEXT("%s: No sections inside the camera cut track"), *FString(__FUNCTION__))
		return MovieSceneCutSections;
	}

	// Convert camera track sections to cut sections
	for (UMovieSceneSection* MovieSceneSection : MovieSceneSections)
	{
		auto CutSection = Cast<UMovieSceneCameraCutSection>(MovieSceneSection);
		if (CutSection == nullptr)
		{
			UE_LOG(LogEasySynth, Error, TEXT("%s: Could not convert MovieSceneSection into a CutSection"),
				*FString(__FUNCTION__));
			return MovieSceneCutSections;
		}
		MovieSceneCutSections.Add(CutSection);
	}

	return MovieSceneCutSections;
}
