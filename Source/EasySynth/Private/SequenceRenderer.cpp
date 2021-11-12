// Copyright Ydrive 2021

#include "SequenceRenderer.h"

#include "MoviePipelineOutputSetting.h"
#include "MoviePipelineQueueSubsystem.h"
#include "MovieRenderPipelineSettings.h"

#include "RendererTargets/RendererTarget.h"


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

void FRendererTargetOptions::GetSelectedTargets(TQueue<TSharedPtr<FRendererTarget>>& OutTargetsQueue) const
{
	OutTargetsQueue.Empty();
	for (int i = 0; i < TargetType::COUNT; i++)
	{
		if (SelectedTargets[i])
		{
			TSharedPtr<FRendererTarget> Target = RendererTarget(i);
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

TSharedPtr<FRendererTarget> FRendererTargetOptions::RendererTarget(const int TargetType)
{
	switch (TargetType)
	{
	case COLOR_IMAGE: return MakeShared<FColorImageTarget>(); break;
	case DEPTH_IMAGE: return MakeShared<FDepthImageTarget>(); break;
	case NORMAL_IMAGE: return MakeShared<FNormalImageTarget>(); break;
	case SEMANTIC_IMAGE: return MakeShared<FSemanticImageTarget>(); break;
	default: return nullptr;
	}
}


const FString USequenceRenderer::EasySynthMoviePipelineConfigPath("/EasySynth/EasySynthMoviePipelineConfig");

USequenceRenderer::USequenceRenderer() :
	EasySynthMoviePipelineConfig(LoadObject<UMoviePipelineMasterConfig>(nullptr, *EasySynthMoviePipelineConfigPath)),
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
	const FString& OutputDirectory)
{
	UE_LOG(LogEasySynth, Log, TEXT("%s"), *FString(__FUNCTION__))

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

	// Prepare the targets queue
	RenderingTargets.GetSelectedTargets(TargetsQueue);
	CurrentTarget = nullptr;

	// Store the output directory
	RenderingDirectory = OutputDirectory;

	UE_LOG(LogEasySynth, Log, TEXT("%s: Rendering..."), *FString(__FUNCTION__))
	bCurrentlyRendering = true;

	// Find the next target and start rendering
	FindNextTarget();

	return true;
}

void USequenceRenderer::OnExecutorFinished(UMoviePipelineExecutorBase* InPipelineExecutor, bool bSuccess)
{
	// Revert target specific modifications to the sequence
	// TODO: Check if successful
	CurrentTarget->FinalizeSequence(RenderingSequence);

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

	// TODO: Setup specifics of the current rendering target
	UE_LOG(LogEasySynth, Log, TEXT("%s: Rendering the %s target"), *FString(__FUNCTION__), *CurrentTarget->Name())
	// TODO: Check if successful
	CurrentTarget->PrepareSequence(RenderingSequence);

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

	// Update pipeline output settings for the current target
	UMoviePipelineOutputSetting* OutputSetting =
		EasySynthMoviePipelineConfig->FindSetting<UMoviePipelineOutputSetting>();
	if (OutputSetting == nullptr)
	{
		ErrorMessage = "Could not find the output setting inside the default config";
		return BroadcastRenderingFinished(false);
	}
	// Update the image output directory
	OutputSetting->OutputDirectory.Path = FPaths::Combine(RenderingDirectory, CurrentTarget->Name());

	// Get the movie rendering editor subsystem
	UMoviePipelineQueueSubsystem* MoviePipelineQueueSubsystem =
		GEditor->GetEditorSubsystem<UMoviePipelineQueueSubsystem>();
	if (MoviePipelineQueueSubsystem == nullptr)
	{
		ErrorMessage = "Could not get the UMoviePipelineQueueSubsystem";
		return BroadcastRenderingFinished(false);
	}

	// Get the queue of sequences to be renderer
	UMoviePipelineQueue* MoviePipelineQueue = MoviePipelineQueueSubsystem->GetQueue();
	if (MoviePipelineQueue == nullptr)
	{
		ErrorMessage = "Could not get the UMoviePipelineQueue";
		return BroadcastRenderingFinished(false);
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
		return BroadcastRenderingFinished(false);
	}

	// Add received level sequence to the queue as a new job
	UMoviePipelineExecutorJob* NewJob = MoviePipelineQueue->AllocateNewJob(UMoviePipelineExecutorJob::StaticClass());
	if (NewJob == nullptr)
	{
		ErrorMessage = "Failed to create new rendering job";
		return BroadcastRenderingFinished(false);
	}
	NewJob->Modify();
	NewJob->Map = FSoftObjectPath(GEditor->GetEditorWorldContext().World());
	NewJob->Author = FPlatformProcess::UserName(false);
	NewJob->SetSequence(RenderingSequence);
	NewJob->JobName = NewJob->Sequence.GetAssetName();
	// The SetConfiguration method creates and assigns the copy of the provided config
	NewJob->SetConfiguration(EasySynthMoviePipelineConfig);

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
	// TODO: Bind other events, such as rendering canceled
}

void USequenceRenderer::BroadcastRenderingFinished(const bool bSuccess)
{
	if (!bSuccess)
	{
		UE_LOG(LogEasySynth, Warning, TEXT("%s: %s"), *FString(__FUNCTION__), *ErrorMessage)
	}
	// TODO: Revert world state to the original one (remove semantic textures)
	bCurrentlyRendering = false;
	RenderingFinishedEvent.Broadcast(bSuccess);
}
