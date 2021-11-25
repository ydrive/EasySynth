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

	// FPS
	FFrameRate DisplayRate = MovieScene->GetDisplayRate();
	// TicksPS
	FFrameRate TickResolutions = MovieScene->GetTickResolution();
	// TPF
	const int TicksPerFrame = TickResolutions.AsDecimal() / DisplayRate.AsDecimal();

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

		UE_LOG(LogEasySynth, Error, TEXT("%s: %d %d"), *FString(__FUNCTION__),
			CutSection->GetTrueRange().GetLowerBoundValue().Value, CutSection->GetTrueRange().GetUpperBoundValue().Value)

        for (
			FFrameNumber FrameNumber = CutSection->GetTrueRange().GetLowerBoundValue();
			FrameNumber < CutSection->GetTrueRange().GetUpperBoundValue();
            FrameNumber += TicksPerFrame)
        {
            Interrogator.Reset();
            TGuardValue<UE::MovieScene::FEntityManager*> DebugVizGuard(
                UE::MovieScene::GEntityManagerForDebuggingVisualizers, &Interrogator.GetLinker()->EntityManager);
		    Interrogator.ImportTrack(CameraTransformTrack, UE::MovieScene::FInterrogationChannel::Default());

            // Add interrogations
            if (Interrogator.AddInterrogation(FrameNumber) == INDEX_NONE)
            {
                UE_LOG(LogEasySynth, Error, TEXT("%s: Adding interrogation failed"), *FString(__FUNCTION__))
                return false;
            }
		    Interrogator.Update();

            TArray<FTransform> TempTransforms;
            Interrogator.QueryWorldSpaceTransforms(UE::MovieScene::FInterrogationChannel::Default(), TempTransforms);

            if (TempTransforms.Num() == 0)
            {
                UE_LOG(LogEasySynth, Error, TEXT("%s: No camera transforms found"), *FString(__FUNCTION__))
                return false;
            }

		    CameraTransforms.Append(TempTransforms);
        }
		const int Num = CameraTransforms.Num() - 1;
		UE_LOG(LogEasySynth, Log, TEXT("%s: %d between %d %d Transform found %f %f %f - %f %f %f"), *FString(__FUNCTION__),
			Num + 1, CutSection->GetTrueRange().GetLowerBoundValue().Value, CutSection->GetTrueRange().GetUpperBoundValue().Value,
			CameraTransforms[0].GetLocation()[0], CameraTransforms[0].GetLocation()[1], CameraTransforms[0].GetLocation()[2],
			CameraTransforms[Num].GetLocation()[0], CameraTransforms[Num].GetLocation()[1], CameraTransforms[Num].GetLocation()[2])
	}

	// Store to file
    for (int i = 0; i < CameraTransforms.Num(); i++)
	{
        const FTransform& Transform = CameraTransforms[i];
		UE_LOG(LogEasySynth, Log, TEXT("%s: Transform found: %d  %f %f %f"), *FString(__FUNCTION__),
			i, Transform.GetLocation()[0], Transform.GetLocation()[1], Transform.GetLocation()[2])
	}

	return true;
}
