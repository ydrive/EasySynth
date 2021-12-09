// Copyright (c) YDrive Inc. All rights reserved.
// Licensed under the MIT License.

#include "SequenceRenderer.h"

#include "MoviePipelineImageSequenceOutput.h"
#include "MoviePipelineOutputSetting.h"
#include "MoviePipelineQueueSubsystem.h"
#include "MovieRenderPipelineSettings.h"

#include "EXROutput/MoviePipelineEXROutputLocal.h"
#include "PathUtils.h"
#include "RendererTargets/CameraPoseExporter.h"
#include "RendererTargets/RendererTarget.h"


const float FRendererTargetOptions::DefaultDepthRangeMetersValue = 100.0f;
const float FRendererTargetOptions::DefaultOpticalFlowScaleValue = 1.0f;

FRendererTargetOptions::FRendererTargetOptions() :
	bExportCameraPoses(false),
	DepthRangeMetersValue(DefaultDepthRangeMetersValue),
	OpticalFlowScaleValue(DefaultOpticalFlowScaleValue)
{
	SelectedTargets.Init(false, TargetType::COUNT);
	OutputFormats.Init(EImageFormat::JPEG, TargetType::COUNT);
}

bool FRendererTargetOptions::AnyOptionSelected() const
{
	for (bool TargetSelected : SelectedTargets)
	{
		if (TargetSelected)
		{
			return true;
		}
	}
	return false;
}

void FRendererTargetOptions::GetSelectedTargets(
	UTextureStyleManager* TextureStyleManager,
	TQueue<TSharedPtr<FRendererTarget>>& OutTargetsQueue) const
{
	OutTargetsQueue.Empty();
	for (int i = 0; i < TargetType::COUNT; i++)
	{
		if (SelectedTargets[i])
		{
			TSharedPtr<FRendererTarget> Target = RendererTarget(i, TextureStyleManager);
			if (Target != nullptr)
			{
				OutTargetsQueue.Enqueue(Target);
			}
			else
			{
				UE_LOG(LogEasySynth, Error, TEXT("%s: Target selection mapped to null renderer target"),
					*FString(__FUNCTION__))
				OutTargetsQueue.Empty();
				return;
			}
		}
	}
}

TSharedPtr<FRendererTarget> FRendererTargetOptions::RendererTarget(
	const int TargetType,
	UTextureStyleManager* TextureStyleManager) const
{
	const EImageFormat OutputFormat = OutputFormats[TargetType];
	switch (TargetType)
	{
	case COLOR_IMAGE: return MakeShared<FColorImageTarget>(TextureStyleManager, OutputFormat); break;
	case DEPTH_IMAGE: return MakeShared<FDepthImageTarget>(
		TextureStyleManager, OutputFormat, DepthRangeMetersValue); break;
	case NORMAL_IMAGE: return MakeShared<FNormalImageTarget>(TextureStyleManager, OutputFormat); break;
	case OPTICAL_FLOW_IMAGE: return MakeShared<FOpticalFlowImageTarget>(
		TextureStyleManager, OutputFormat, OpticalFlowScaleValue); break;
	case SEMANTIC_IMAGE: return MakeShared<FSemanticImageTarget>(TextureStyleManager, OutputFormat); break;
	default: return nullptr;
	}
}

USequenceRenderer::USequenceRenderer() :
	EasySynthMoviePipelineConfig(DuplicateObject<UMoviePipelineMasterConfig>( 
		LoadObject<UMoviePipelineMasterConfig>(nullptr, *FPathUtils::DefaultMoviePipelineConfigPath()), nullptr)),
	bCurrentlyRendering(false),
	ErrorMessage("")
{
	// Check if the config asset is loaded correctly
	if (EasySynthMoviePipelineConfig == nullptr)
	{
		ErrorMessage = "Could not load the EasySynthMoviePipelineConfig";
		UE_LOG(LogEasySynth, Error, TEXT("%s: %s"), *FString(__FUNCTION__), *ErrorMessage)
		check(EasySynthMoviePipelineConfig)
	}
}

bool USequenceRenderer::RenderSequence(
	ULevelSequence* LevelSequence,
	const FRendererTargetOptions RenderingTargets,
	const FIntPoint OutputImageResolution,
	const FString& OutputDirectory)
{
	UE_LOG(LogEasySynth, Log, TEXT("%s"), *FString(__FUNCTION__))

	if (TextureStyleManager == nullptr)
	{
		UE_LOG(LogEasySynth, Error, TEXT("%s: Texture style manager is null"), *FString(__FUNCTION__))
		check(TextureStyleManager)
	}

	// Check if rendering is already in progress
	if (bCurrentlyRendering)
	{
		ErrorMessage = "Rendering already in progress";
		UE_LOG(LogEasySynth, Warning, TEXT("%s: %s"), *FString(__FUNCTION__), *ErrorMessage)
		return false;
	}

	// Check if LevelSequence is valid
	RenderingSequence = LevelSequence;
	if (RenderingSequence == nullptr)
	{
		ErrorMessage = "Provided level sequence is null";
		UE_LOG(LogEasySynth, Warning, TEXT("%s: %s"), *FString(__FUNCTION__), *ErrorMessage)
		return false;
	}

	// Check if any rendering target is selected
	if (!RenderingTargets.AnyOptionSelected())
	{
		ErrorMessage = "No rendering targets selected";
		UE_LOG(LogEasySynth, Warning, TEXT("%s: %s"), *FString(__FUNCTION__), *ErrorMessage)
		return false;
	}

	// Store parameters
	OutputResolution = OutputImageResolution;
	RenderingDirectory = OutputDirectory;

	// Export camera poses if requested
	if (RenderingTargets.ExportCameraPoses())
	{
		FCameraPoseExporter CameraPoseExporter;
		if (!CameraPoseExporter.ExportCameraPoses(RenderingSequence, OutputResolution, RenderingDirectory))
		{
			ErrorMessage = "Could not export camera poses";
			UE_LOG(LogEasySynth, Warning, TEXT("%s: %s"), *FString(__FUNCTION__), *ErrorMessage)
			return false;
		}
	}

	// Prepare the targets queue
	RenderingTargets.GetSelectedTargets(TextureStyleManager, TargetsQueue);
	OriginalTextureStyle = TextureStyleManager->SelectedTextureStyle();
	CurrentTarget = nullptr;

	UE_LOG(LogEasySynth, Log, TEXT("%s: Rendering..."), *FString(__FUNCTION__))
	bCurrentlyRendering = true;

	// Find the next target and start rendering
	FindNextTarget();

	return true;
}

void USequenceRenderer::OnExecutorFinished(UMoviePipelineExecutorBase* InPipelineExecutor, bool bSuccess)
{
	// Revert target specific modifications to the sequence
	if (!CurrentTarget->FinalizeSequence(RenderingSequence))
	{
		ErrorMessage = FString::Printf(TEXT("Failed while finalizing the rendering of the %s target"), *CurrentTarget->Name());
		return BroadcastRenderingFinished(false);
	}

	if (!bSuccess)
	{
		ErrorMessage = FString::Printf(TEXT("Failed while rendering the %s target"), *CurrentTarget->Name());
		return BroadcastRenderingFinished(false);
	}

	// Successful rendering, proceed to the next target
	FindNextTarget();
}

void USequenceRenderer::FindNextTarget()
{
	// Check if the end is reached
	if (TargetsQueue.IsEmpty())
	{
		return BroadcastRenderingFinished(true);
	}

	// Select the next requested target
	TargetsQueue.Dequeue(CurrentTarget);

	// Setup specifics of the current rendering target
	UE_LOG(LogEasySynth, Log, TEXT("%s: Rendering the %s target"), *FString(__FUNCTION__), *CurrentTarget->Name())
	if (!CurrentTarget->PrepareSequence(RenderingSequence))
	{
		ErrorMessage = FString::Printf(TEXT("Failed while preparing the rendering of the %s target"), *CurrentTarget->Name());
		return BroadcastRenderingFinished(false);
	}

	// Start the rendering after a brief pause
	const float DelaySeconds = 2.0f;
	const bool bLoop = false;
	GEditor->GetEditorWorldContext().World()->GetTimerManager().SetTimer(
		RendererPauseTimerHandle,
		this,
		&USequenceRenderer::StartRendering,
		DelaySeconds,
		bLoop);
}

void USequenceRenderer::StartRendering()
{
	// Make sure the sequence is still sound
	if (RenderingSequence == nullptr)
	{
		ErrorMessage = "Provided level sequence is null when starting the recording";
		return BroadcastRenderingFinished(false);
	}

	// Make sure a renderer target is selected
	if (!CurrentTarget.IsValid())
	{
		ErrorMessage = "Current renderer target null when starting the recording";
		return BroadcastRenderingFinished(false);
	}

	// Get the movie rendering editor subsystem
	UMoviePipelineQueueSubsystem* MoviePipelineQueueSubsystem =
		GEditor->GetEditorSubsystem<UMoviePipelineQueueSubsystem>();
	if (MoviePipelineQueueSubsystem == nullptr)
	{
		ErrorMessage = "Could not get the UMoviePipelineQueueSubsystem";
		return BroadcastRenderingFinished(false);
	}

	// Add received level sequence to the queue as a new job
	if (!PrepareJobQueue(MoviePipelineQueueSubsystem))
	{
		// Propagate the error message set inside the PrepareJobQueue
		return BroadcastRenderingFinished(false);
	}

	// Get the default movie rendering settings
	const UMovieRenderPipelineProjectSettings* ProjectSettings = GetDefault<UMovieRenderPipelineProjectSettings>();
	if (ProjectSettings->DefaultLocalExecutor == nullptr)
	{
		ErrorMessage = "Could not get the UMovieRenderPipelineProjectSettings";
		return BroadcastRenderingFinished(false);
	}

	// Run the rendering
	UMoviePipelineExecutorBase* ActiveExecutor =
		MoviePipelineQueueSubsystem->RenderQueueWithExecutor(ProjectSettings->DefaultLocalExecutor);
	if (ActiveExecutor == nullptr)
	{
		ErrorMessage = "Could not start the rendering";
		return BroadcastRenderingFinished(false);
	}

	// Assign rendering finished callback
	ActiveExecutor->OnExecutorFinished().AddUObject(this, &USequenceRenderer::OnExecutorFinished);
}

bool USequenceRenderer::PrepareJobQueue(UMoviePipelineQueueSubsystem* MoviePipelineQueueSubsystem)
{
	check(MoviePipelineQueueSubsystem)

	// Update export image format
	UMoviePipelineSetting* JpegSetting = EasySynthMoviePipelineConfig->FindOrAddSettingByClass(
		UMoviePipelineImageSequenceOutput_JPG::StaticClass(), true);
	UMoviePipelineSetting* PngSetting = EasySynthMoviePipelineConfig->FindSettingByClass(
		UMoviePipelineImageSequenceOutput_PNG::StaticClass(), true);
	UMoviePipelineSetting* ExrSetting = EasySynthMoviePipelineConfig->FindOrAddSettingByClass(
		UMoviePipelineImageSequenceOutput_EXRLocal::StaticClass(), true);
	if (JpegSetting == nullptr || PngSetting == nullptr || ExrSetting == nullptr)
	{
		ErrorMessage = "JPEG, PNG or EXR settings not found";
		return false;
	}
	JpegSetting->SetIsEnabled(CurrentTarget->ImageFormat == EImageFormat::JPEG);
	PngSetting->SetIsEnabled(CurrentTarget->ImageFormat == EImageFormat::PNG);
	ExrSetting->SetIsEnabled(CurrentTarget->ImageFormat == EImageFormat::EXR);

	// Update pipeline output settings for the current target
	UMoviePipelineOutputSetting* OutputSetting =
		EasySynthMoviePipelineConfig->FindSetting<UMoviePipelineOutputSetting>();
	if (OutputSetting == nullptr)
	{
		ErrorMessage = "Could not find the output setting inside the default config";
		return false;
	}
	// Update the image output directory
	OutputSetting->OutputDirectory.Path = FPaths::Combine(RenderingDirectory, CurrentTarget->Name());
	OutputSetting->OutputResolution = OutputResolution;

	// Get the queue of sequences to be renderer
	UMoviePipelineQueue* MoviePipelineQueue = MoviePipelineQueueSubsystem->GetQueue();
	if (MoviePipelineQueue == nullptr)
	{
		ErrorMessage = "Could not get the UMoviePipelineQueue";
		return false;
	}
	MoviePipelineQueue->Modify();

	// Clear the current queue
	TArray<UMoviePipelineExecutorJob*> ExistingJobs = MoviePipelineQueue->GetJobs();
	for (UMoviePipelineExecutorJob* MoviePipelineExecutorJob : ExistingJobs)
	{
		MoviePipelineQueue->DeleteJob(MoviePipelineExecutorJob);
	}
	if (MoviePipelineQueue->GetJobs().Num() > 0)
	{
		ErrorMessage = "Job queue not properly cleared";
		return false;
	}

	// Add received level sequence to the queue as a new job
	UMoviePipelineExecutorJob* NewJob = MoviePipelineQueue->AllocateNewJob(UMoviePipelineExecutorJob::StaticClass());
	if (NewJob == nullptr)
	{
		ErrorMessage = "Failed to create new rendering job";
		return false;
	}
	NewJob->Modify();
	NewJob->Map = FSoftObjectPath(GEditor->GetEditorWorldContext().World());
	NewJob->Author = FPlatformProcess::UserName(false);
	NewJob->SetSequence(RenderingSequence);
	NewJob->JobName = NewJob->Sequence.GetAssetName();

	// The SetConfiguration method creates and assigns the copy of the provided config
	NewJob->SetConfiguration(EasySynthMoviePipelineConfig);

	return true;
}

void USequenceRenderer::BroadcastRenderingFinished(const bool bSuccess)
{
	if (!bSuccess)
	{
		UE_LOG(LogEasySynth, Warning, TEXT("%s: %s"), *FString(__FUNCTION__), *ErrorMessage)
	}

	// Revert world state to the original one
	TextureStyleManager->CheckoutTextureStyle(OriginalTextureStyle);

	bCurrentlyRendering = false;
	RenderingFinishedEvent.Broadcast(bSuccess);
}
