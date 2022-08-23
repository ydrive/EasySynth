// Copyright (c) 2022 YDrive Inc. All rights reserved.

#include "RendererTargets/CameraPoseExporter.h"

#include "Camera/CameraComponent.h"
#include "EntitySystem/Interrogation/MovieSceneInterrogationLinker.h"
#include "EntitySystem/MovieSceneEntitySystemTypes.h"
#include "ILevelSequenceEditorToolkit.h"
#include "ISequencer.h"
#include "Kismet/KismetMathLibrary.h"
#include "LevelSequence.h"
#include "Misc/FileHelper.h"
#include "MovieScene.h"
#include "MovieSceneObjectBindingID.h"
#include "Sections/MovieSceneCameraCutSection.h"
#include "Subsystems/AssetEditorSubsystem.h"
#include "Tracks/MovieScene3DTransformTrack.h"


bool FCameraPoseExporter::ExportCameraPoses(
	ULevelSequence* LevelSequence,
	const FIntPoint OutputImageResolution,
	const FString& OutputDir,
	UCameraComponent* CameraComponent)
{
	// Open the received level sequence inside the sequencer wrapper
	if (!SequencerWrapper.OpenSequence(LevelSequence))
	{
		UE_LOG(LogEasySynth, Error, TEXT("%s: Sequencer wrapper opening failed"), *FString(__FUNCTION__))
		return false;
	}

	OutputResolution = OutputImageResolution;

	// Extract the camera pose transforms
	const bool bAccumulateCameraOffset = (CameraComponent != nullptr);
	if (!ExtractCameraTransforms(bAccumulateCameraOffset))
	{
		UE_LOG(LogEasySynth, Error, TEXT("%s: Camera pose extraction failed"), *FString(__FUNCTION__))
		return false;
	}

	// Store to file
	FString SaveFilePath;
	if (CameraComponent == nullptr)
	{
		SaveFilePath = FPathUtils::CameraRigPosesFilePath(OutputDir);
	}
	else
	{
		SaveFilePath = FPathUtils::CameraPosesFilePath(OutputDir, CameraComponent);
	}
	if (!SavePosesToCSV(SaveFilePath))
	{
		UE_LOG(LogEasySynth, Error, TEXT("%s: Failed while saving camera poses to the file"), *FString(__FUNCTION__))
		return false;
	}

	return true;
}

bool FCameraPoseExporter::ExtractCameraTransforms(const bool bAccumulateCameraOffset)
{
	// Get level sequence fps
	const FFrameRate DisplayRate = SequencerWrapper.GetMovieScene()->GetDisplayRate();
	const double FrameTime = 1.0f / DisplayRate.AsDecimal();
	double AccumulatedFrameTime = 0.0f;

	// Get level sequence ticks per second
	// Engine likes to update much more often than the video frame rate,
	// so this is needed to calculate engine ticks that correspond to frames
	const FFrameRate TickResolutions = SequencerWrapper.GetMovieScene()->GetTickResolution();

	// Calculate ticks per frame
	const int TicksPerFrame = TickResolutions.AsDecimal() / DisplayRate.AsDecimal();

	// Get the camera poses from each cut section
	TArray<UMovieSceneCameraCutSection*>& CutSections = SequencerWrapper.GetMovieSceneCutSections();
	for (auto CutSection : CutSections)
	{
		// Get the camera component
		UCameraComponent* Camera = CutSection->GetFirstCamera(
			*SequencerWrapper.GetSequencer(),
			SequencerWrapper.GetSequencer()->GetFocusedTemplateID());
		if (Camera == nullptr)
		{
			UE_LOG(LogEasySynth, Error, TEXT("%s: Cut section camera component is null"), *FString(__FUNCTION__))
			return false;
		}

		// Get the current cut section camera binding id
		const FMovieSceneObjectBindingID& CameraBindingID = CutSection->GetCameraBindingID();

		// Find the track inside the level sequence that corresponds to the
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

			for (FTransform& Transform : TempTransforms)
			{
				if (bAccumulateCameraOffset)
				{
					Transform.Accumulate(Camera->GetRelativeTransform());
				}

				AccumulatedFrameTime += FrameTime;
				Timestamps.Add(AccumulatedFrameTime);
			}

			CameraTransforms.Append(TempTransforms);
		}
	}

	return true;
}

bool FCameraPoseExporter::SavePosesToCSV(const FString& FilePath)
{
	// Create the file content
	TArray<FString> Lines;
	Lines.Add("id,tx,ty,tz,qx,qy,qz,qw,t");

	for (int i = 0; i < CameraTransforms.Num(); i++)
	{
		// Remove the scaling that makes no impact on camera functionality,
		// but my be used to scale the camera placeholder mesh as user desires
		CameraTransforms[i].SetScale3D(FVector(1.0f, 1.0f, 1.0f));
		const FVector Translation = CameraTransforms[i].GetTranslation();
		const FQuat Rotation = CameraTransforms[i].GetRotation();

		Lines.Add(FString::Printf(TEXT("%d,%f,%f,%f,%f,%f,%f,%f,%f"),
			i,
			Translation.X, Translation.Y, Translation.Z,
			Rotation.X, Rotation.Y, Rotation.Z, Rotation.W,
			Timestamps[i]));
	}

	// Save the file
	if (!FFileHelper::SaveStringArrayToFile(
		Lines,
		*FilePath,
		FFileHelper::EEncodingOptions::AutoDetect,
		&IFileManager::Get(),
		EFileWrite::FILEWRITE_None))
	{
		UE_LOG(LogEasySynth, Error, TEXT("%s: Failed while saving the file %s"), *FString(__FUNCTION__), *FilePath)
		return false;
	}

	return true;
}
