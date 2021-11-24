// Copyright (c) YDrive Inc. All rights reserved.
// Licensed under the MIT License.

#include "RendererTargets/CameraPoseExporter.h"

#include "EntitySystem/Interrogation/MovieSceneInterrogationLinker.h"
#include "EntitySystem/MovieSceneEntitySystemTypes.h"
#include "ILevelSequenceEditorToolkit.h"
#include "ISequencer.h"
#include "LevelSequence.h"
#include "MovieScene.h"
#include "MovieSceneObjectBindingID.h"
#include "Sections/MovieSceneCameraCutSection.h"
#include "Subsystems/AssetEditorSubsystem.h"
#include "Tracks/MovieScene3DTransformTrack.h"


bool FCameraPoseExporter::ExportCameraPoses(ULevelSequence* LevelSequence)
{
    UE_LOG(LogEasySynth, Log, TEXT("%s"), *FString(__FUNCTION__))

	UMovieScene* MovieScene = LevelSequence->GetMovieScene();
	if (MovieScene == nullptr)
	{
		UE_LOG(LogEasySynth, Error, TEXT("%s: Could not get the movie scene from the level sequence"),
			*FString(__FUNCTION__))
		return false;
	}

	UMovieSceneTrack* CameraCutTrack = MovieScene->GetCameraCutTrack();
	if (CameraCutTrack == nullptr)
	{
		UE_LOG(LogEasySynth, Error, TEXT("%s: Could not get the camera cut track from the movie scene"),
			*FString(__FUNCTION__))
		return false;
	}

	TArray<UMovieSceneSection*> MovieSceneSections = CameraCutTrack->GetAllSections();
	if (MovieSceneSections.Num() == 0)
	{
		UE_LOG(LogEasySynth, Warning, TEXT("%s: No sections inside the camera cut track"), *FString(__FUNCTION__))
		return false;
	}

	// Open sequencer editor for the level sequence asset
	TArray<UObject*> Assets;
	Assets.Add(LevelSequence);
	if (!GEditor->GetEditorSubsystem<UAssetEditorSubsystem>()->OpenEditorForAssets(Assets))
	{
		UE_LOG(LogEasySynth, Error, TEXT("%s: Could not open the level sequence editor"), *FString(__FUNCTION__))
		return false;
	}

	// Get the LevelSequenceEditor
	IAssetEditorInstance* AssetEditor =
		GEditor->GetEditorSubsystem<UAssetEditorSubsystem>()->FindEditorForAsset(LevelSequence, false);
	if (AssetEditor == nullptr)
	{
		UE_LOG(LogEasySynth, Error, TEXT("%s: Could not find the asset editor"), *FString(__FUNCTION__))
		return false;
	}
	ILevelSequenceEditorToolkit* LevelSequenceEditor = static_cast<ILevelSequenceEditorToolkit*>(AssetEditor);
	if (LevelSequenceEditor == nullptr)
	{
		UE_LOG(LogEasySynth, Error, TEXT("%s: Could not find the level sequence editor"), *FString(__FUNCTION__))
		return false;
	}

	// Get the Sequencer
	TWeakPtr<ISequencer> WeakSequencer = LevelSequenceEditor ? LevelSequenceEditor->GetSequencer() : nullptr;
	if (!WeakSequencer.IsValid())
	{
		UE_LOG(LogEasySynth, Error, TEXT("%s: Could not get the sequencer"), *FString(__FUNCTION__))
		return false;
	}

	// Get the camera from each section
	TArray<FTransform> CameraTransforms;
	for (UMovieSceneSection* MovieSceneSection : MovieSceneSections)
	{
		// Convert UMovieSceneSection to UMovieSceneCameraCutSection
		auto CutSection = Cast<UMovieSceneCameraCutSection>(MovieSceneSection);
		if (CutSection == nullptr)
		{
			UE_LOG(LogEasySynth, Error, TEXT("%s: Could not convert MovieSceneSection into a CutSection"),
				*FString(__FUNCTION__));
			return false;
		}

		const FMovieSceneObjectBindingID& CameraBindingID = CutSection->GetCameraBindingID();

		// Get the camera componenet
		UCameraComponent* Camera =
			CutSection->GetFirstCamera(*WeakSequencer.Pin(), WeakSequencer.Pin()->GetFocusedTemplateID());
		if (Camera == nullptr)
		{
			UE_LOG(LogEasySynth, Error, TEXT("%s: Cut section camera component is null"), *FString(__FUNCTION__))
			return false;
		}

		// Find the transform track for our bound camera.
		UMovieScene3DTransformTrack* CameraTransformTrack = nullptr;
		for (const FMovieSceneBinding& Binding : MovieScene->GetBindings())
		{
			if (Binding.GetObjectGuid() == CameraBindingID.GetGuid())
			{
				for (UMovieSceneTrack* Track : Binding.GetTracks())
				{
					CameraTransformTrack = Cast<UMovieScene3DTransformTrack>(Track);
					if (CameraTransformTrack)
					{
						break;
					}
				}
			}
		}
		if (CameraTransformTrack == nullptr)
		{
			UE_LOG(LogEasySynth, Error, TEXT("%s: Could not find camera transform track"), *FString(__FUNCTION__))
			return false;
		}

		// Get camera positions
		UE::MovieScene::FSystemInterrogator Interrogator;

		TGuardValue<UE::MovieScene::FEntityManager*> DebugVizGuard(
            UE::MovieScene::GEntityManagerForDebuggingVisualizers, &Interrogator.GetLinker()->EntityManager);

		Interrogator.ImportTrack(CameraTransformTrack, UE::MovieScene::FInterrogationChannel::Default());

		Interrogator.AddInterrogation(CutSection->GetTrueRange().GetLowerBoundValue());

		Interrogator.Update();

		TArray<FTransform> TempTransforms;
		Interrogator.QueryWorldSpaceTransforms(UE::MovieScene::FInterrogationChannel::Default(), TempTransforms);

		if (TempTransforms.Num() == 0)
		{
			UE_LOG(LogEasySynth, Error, TEXT("%s: No camera transforms found"), *FString(__FUNCTION__))
			return false;
		}

		UE_LOG(LogEasySynth, Log, TEXT("%s: Transform found %f %f %f"), *FString(__FUNCTION__),
			TempTransforms[0].GetLocation()[0], TempTransforms[0].GetLocation()[1], TempTransforms[0].GetLocation()[2])

		CameraTransforms.Append(TempTransforms);
	}

	// Store to file
	for (const FTransform& Transform : CameraTransforms)
	{
		// TODO:
	}

	return true;
}
