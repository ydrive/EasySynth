// Copyright (c) YDrive Inc. All rights reserved.
// Licensed under the MIT License.

#include "RendererTargets/CameraPoseExporter.h"

#include "EntitySystem/Interrogation/MovieSceneInterrogationLinker.h"
#include "EntitySystem/MovieSceneEntitySystemTypes.h"
#include "ILevelSequenceEditorToolkit.h"
#include "ISequencer.h"
#include "LevelSequence.h"
#include "Misc/FileHelper.h"
#include "MovieScene.h"
#include "MovieSceneObjectBindingID.h"
#include "Sections/MovieSceneCameraCutSection.h"
#include "Subsystems/AssetEditorSubsystem.h"
#include "Tracks/MovieScene3DTransformTrack.h"


bool FCameraPoseExporter::ExportCameraPoses(ULevelSequence* LevelSequence, const FString& OutputDir)
{
	// Open the received level sequence inside the sequencer wrapper
	if (!SequencerWrapper.OpenSequence(LevelSequence))
	{
		UE_LOG(LogEasySynth, Error, TEXT("%s: Sequencer wrapper opening failed"), *FString(__FUNCTION__))
		return false;
	}

	// Extract the camera pose transforms
	if (!ExtractCameraTransforms())
	{
		UE_LOG(LogEasySynth, Error, TEXT("%s: Camera pose extaction failed"), *FString(__FUNCTION__))
		return false;
	}

	// Store to file
	if (!SavePosesToFile(OutputDir))
	{
		UE_LOG(LogEasySynth, Error, TEXT("%s: Failed while saving camera poses to the file"), *FString(__FUNCTION__))
		return false;
	}

	return true;
}

bool FCameraPoseExporter::ExtractCameraTransforms()
{
	// Get level sequence fps
	const FFrameRate DisplayRate = SequencerWrapper.GetMovieScene()->GetDisplayRate();

	// Get level sequence ticks per second
	// Engine likes to update much more often than the video frame rate,
	// so this is needed to calculate engine ticks that corespond to frames
	const FFrameRate TickResolutions = SequencerWrapper.GetMovieScene()->GetTickResolution();

	// Calculate ticks per frame
	const int TicksPerFrame = TickResolutions.AsDecimal() / DisplayRate.AsDecimal();

	// Get the camera poses from each cut section
	TArray<UMovieSceneCameraCutSection*>& CutSections = SequencerWrapper.GetMovieSceneCutSections();
	for (auto CutSection : CutSections)
	{
		// Get the current cut section camera binding id
		const FMovieSceneObjectBindingID& CameraBindingID = CutSection->GetCameraBindingID();

		// Get the camera componenet
		UCameraComponent* Camera = CutSection->GetFirstCamera(
			*SequencerWrapper.GetSequencer(),
			SequencerWrapper.GetSequencer()->GetFocusedTemplateID());
		if (Camera == nullptr)
		{
			UE_LOG(LogEasySynth, Error, TEXT("%s: Cut section camera component is null"), *FString(__FUNCTION__))
			return false;
		}

		// Find the track inside the level sequence that coresponds to the
		// pose transformation of the camera
		UMovieScene3DTransformTrack* CameraTransformTrack = nullptr;
		for (const FMovieSceneBinding& Binding : SequencerWrapper.GetMovieScene()->GetBindings())
		{
			if (Binding.GetObjectGuid() == CameraBindingID.GetGuid())
			{
				for (UMovieSceneTrack* Track : Binding.GetTracks())
				{
					CameraTransformTrack = Cast<UMovieScene3DTransformTrack>(Track);
					if (CameraTransformTrack != nullptr)
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

		// Interrogator object that queries the transformation track for camera poses
		UE::MovieScene::FSystemInterrogator Interrogator;

		// Inclusive lower bound of the movie scene ticks that belong to this cut section
		FFrameNumber StartTickNumber = CutSection->GetTrueRange().GetLowerBoundValue();
		// Exclusive upper bound of the movie scene ticks that belong to this cut section
		FFrameNumber EndTickNumber = CutSection->GetTrueRange().GetUpperBoundValue();
        for (FFrameNumber TickNumber = StartTickNumber; TickNumber < EndTickNumber; TickNumber += TicksPerFrame)
        {
			// Reinitialize the interrogator for each frame
            Interrogator.Reset();
            TGuardValue<UE::MovieScene::FEntityManager*> DebugVizGuard(
                UE::MovieScene::GEntityManagerForDebuggingVisualizers, &Interrogator.GetLinker()->EntityManager);
		    Interrogator.ImportTrack(CameraTransformTrack, UE::MovieScene::FInterrogationChannel::Default());

            // Add frame interrogation
            if (Interrogator.AddInterrogation(TickNumber) == INDEX_NONE)
            {
                UE_LOG(LogEasySynth, Error, TEXT("%s: Adding interrogation failed"), *FString(__FUNCTION__))
                return false;
            }
		    Interrogator.Update();

			// Get the camera pose transform for the frame
			// Engine crashes in case multiple interrogations are added at once
            TArray<FTransform> TempTransforms;
            Interrogator.QueryWorldSpaceTransforms(UE::MovieScene::FInterrogationChannel::Default(), TempTransforms);
            if (TempTransforms.Num() == 0)
            {
                UE_LOG(LogEasySynth, Error, TEXT("%s: No camera transforms found"), *FString(__FUNCTION__))
                return false;
            }

		    CameraTransforms.Append(TempTransforms);
        }
	}

	return true;
}

bool FCameraPoseExporter::SavePosesToFile(const FString& OutputDir)
{
	// Create the file content
	TArray<FString> Lines;
	Lines.Add("id, tx, ty, tz, qw, qx, qy, qz");

    for (int i = 0; i < CameraTransforms.Num(); i++)
	{
        const FTransform& Transform = CameraTransforms[i];
		const FVector& Location = Transform.GetLocation();
		const FQuat& Rotation = Transform.GetRotation();
		// A constant for converting centimeters to meters
		const float Conv = 1.0e-2f;
		Lines.Add(FString::Printf(TEXT("%d, %f, %f, %f, %f, %f, %f, %f"),
			i,
			Location.X * Conv, Location.Y * Conv, Location.Z * Conv,
			Rotation.W, Rotation.X, Rotation.Y, Rotation.Z));
	}

	// Save the file
	const FString SaveFilePath = FPathUtils::CameraPosesFilePath(OutputDir);
	if (!FFileHelper::SaveStringArrayToFile(
		Lines,
		*SaveFilePath,
		FFileHelper::EEncodingOptions::AutoDetect,
		&IFileManager::Get(),
		EFileWrite::FILEWRITE_None))
	{
		UE_LOG(LogEasySynth, Error, TEXT("%s: Failed while saving the file %s"), *FString(__FUNCTION__), *SaveFilePath)
        return false;
	}

	return true;
}
