// Copyright Ydrive 2021

#include "SequenceRenderer.h"

#include "MoviePipelineQueueSubsystem.h"
#include "MovieRenderPipelineSettings.h"


const FString FSequenceRenderer::EasySynthMoviePipelineConfigPath("/EasySynth/EasySynthMoviePipelineConfig");

FSequenceRenderer::FSequenceRenderer() :
	bCurrentlyRendering(false),
	ErrorMessage("")
{}

bool FSequenceRenderer::RenderSequence(ULevelSequence* LevelSequence, FSequenceRendererTargets RenderingTargets)
{
	UE_LOG(LogEasySynth, Log, TEXT("%s"), *FString(__FUNCTION__))

	// Load the asset representing needed movie pipeline configuration
	UMoviePipelineMasterConfig* EasySynthMoviePipelineConfig =
		LoadObject<UMoviePipelineMasterConfig>(nullptr, *EasySynthMoviePipelineConfigPath);
	if (EasySynthMoviePipelineConfig == nullptr)
	{
		ErrorMessage = "Could not load the EasySynthMoviePipelineConfig";
		UE_LOG(LogEasySynth, Error, TEXT("%s: %s"), *FString(__FUNCTION__), *ErrorMessage)
		return false;
	}

	// Check if rendering is already in progress
	if (bCurrentlyRendering)
	{
		ErrorMessage = "Rendering already in progress";
		UE_LOG(LogEasySynth, Log, TEXT("%s: %s"), *FString(__FUNCTION__), *ErrorMessage)
		return false;
	}

	// Check if LevelSequence is valid
	if (LevelSequence == nullptr)
	{
		ErrorMessage = "Provided level sequence is null";
		UE_LOG(LogEasySynth, Log, TEXT("%s: %s"), *FString(__FUNCTION__), *ErrorMessage)
		return false;
	}

	// Check if any rendering target is selected
	RequestedSequenceRendererTargets = RenderingTargets;
	if (!RequestedSequenceRendererTargets.AnyOptionSelected())
	{
		ErrorMessage = "No rendering targets selected";
		UE_LOG(LogEasySynth, Log, TEXT("%s: %s"), *FString(__FUNCTION__), *ErrorMessage)
		return false;
	}

	// Get the movie rendering editor subsystem
	MoviePipelineQueueSubsystem = GEditor->GetEditorSubsystem<UMoviePipelineQueueSubsystem>();
	if (MoviePipelineQueueSubsystem == nullptr)
	{
		ErrorMessage = "Could not get the UMoviePipelineQueueSubsystem";
		UE_LOG(LogEasySynth, Log, TEXT("%s: %s"), *FString(__FUNCTION__), *ErrorMessage)
		return false;
	}

	// Get the queue of sequences to be renderer
	UMoviePipelineQueue* MoviePipelineQueue = MoviePipelineQueueSubsystem->GetQueue();
	if (MoviePipelineQueue == nullptr)
	{
		ErrorMessage = "Could not get the UMoviePipelineQueue";
		UE_LOG(LogEasySynth, Log, TEXT("%s: %s"), *FString(__FUNCTION__), *ErrorMessage)
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
		UE_LOG(LogEasySynth, Log, TEXT("%s: %s"), *FString(__FUNCTION__), *ErrorMessage)
		return false;
	}

	// Add received level sequence to the queue as a new job
	UMoviePipelineExecutorJob* NewJob = MoviePipelineQueue->AllocateNewJob(UMoviePipelineExecutorJob::StaticClass());
	if (NewJob == nullptr)
	{
		ErrorMessage = "Failed to create new rendering job";
		UE_LOG(LogEasySynth, Log, TEXT("%s: %s"), *FString(__FUNCTION__), *ErrorMessage)
		return false;
	}
	NewJob->Modify();
	NewJob->Map = FSoftObjectPath(GEditor->GetEditorWorldContext().World());
	NewJob->Author = FPlatformProcess::UserName(false);
	NewJob->SetSequence(LevelSequence);
	NewJob->JobName = NewJob->Sequence.GetAssetName();
	NewJob->SetConfiguration(EasySynthMoviePipelineConfig);

	// Starting the next target rendering will increase the CurrentTarget from -1 to 0
	CurrentTarget = -1;

	if (!StartRenderingNextTarget())
	{
		// Propagate the error set inside the StartRenderingNextTarget
		return false;
	}

	UE_LOG(LogEasySynth, Log, TEXT("%s: Rendering..."), *FString(__FUNCTION__))
	bCurrentlyRendering = true;

	return true;
}

void FSequenceRenderer::OnExecutorFinished(UMoviePipelineExecutorBase* InPipelineExecutor, bool bSuccess)
{
	if (!bSuccess)
	{
		ErrorMessage = FString::Printf(
			TEXT("Failed while rendering the %s target"),
			*FSequenceRendererTargets::TargetName(CurrentTarget));
		UE_LOG(LogEasySynth, Log, TEXT("%s: %s"), *FString(__FUNCTION__), *ErrorMessage)
		bCurrentlyRendering = false;
		// TODO: Notify widget of the unsuccessful rendering
		return;
	}

	// This will handle all checks related to starting the next rendering job
	StartRenderingNextTarget();
}

bool FSequenceRenderer::StartRenderingNextTarget()
{
	// Find the next requested target
	while (++CurrentTarget != FSequenceRendererTargets::COUNT &&
		!RequestedSequenceRendererTargets.TargetSelected(CurrentTarget)) {}

	// Check if the end is reached
	if (CurrentTarget == FSequenceRendererTargets::COUNT)
	{
		bCurrentlyRendering = false;
		// TODO: Notify widget of the successful rendering
		return true;
	}

	// TODO: Setup specifics of the current rendering target
	UE_LOG(LogEasySynth, Log, TEXT("%s: Rendering the %s target"),
		*FString(__FUNCTION__), *FSequenceRendererTargets::TargetName(CurrentTarget))

	// Get the default movie rendering settings
	const UMovieRenderPipelineProjectSettings* ProjectSettings = GetDefault<UMovieRenderPipelineProjectSettings>();
	if (ProjectSettings->DefaultLocalExecutor == nullptr)
	{
		ErrorMessage = "Could not get the UMovieRenderPipelineProjectSettings";
		UE_LOG(LogEasySynth, Log, TEXT("%s: %s"), *FString(__FUNCTION__), *ErrorMessage)
		return false;
	}

	// Run the rendering
	UMoviePipelineExecutorBase* ActiveExecutor =
		MoviePipelineQueueSubsystem->RenderQueueWithExecutor(ProjectSettings->DefaultLocalExecutor);
	if (ActiveExecutor == nullptr)
	{
		ErrorMessage = "Could not start the rendering";
		UE_LOG(LogEasySynth, Log, TEXT("%s: %s"), *FString(__FUNCTION__), *ErrorMessage)
		bCurrentlyRendering = false;
		// TODO: Notify widget of the unsuccessful rendering
		return false;
	}

	// Assign rendering finished callback
	ActiveExecutor->OnExecutorFinished().AddRaw(this, &FSequenceRenderer::OnExecutorFinished);

	return true;
}
