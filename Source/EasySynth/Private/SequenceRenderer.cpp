// Copyright Ydrive 2021

#include "SequenceRenderer.h"

#include "MoviePipelineQueueSubsystem.h"
#include "MovieRenderPipelineSettings.h"


const FString FSequenceRenderer::EasySynthMoviePipelineConfigPath("/EasySynth/EasySynthMoviePipelineConfig");

FSequenceRenderer::FSequenceRenderer() :
	bCurrentlyRendering(false),
	ErrorMessage("")
{}

bool FSequenceRenderer::RenderSequence(ULevelSequence* LevelSequence)
{
	UE_LOG(LogEasySynth, Log, TEXT("%s"), *FString(__FUNCTION__))

	// Check if rendering is already in progress
	if (bCurrentlyRendering)
	{
		ErrorMessage = "Rendering already in progress";
		UE_LOG(LogEasySynth, Log, TEXT("%s: %s"), *FString(__FUNCTION__), *ErrorMessage)
		return false;
	}

	// Check if LevelSequence is valid
	// if (LevelSequence == nullptr)
	// {
	// 	ErrorMessage = "Provided level sequence is null";
	// 	UE_LOG(LogEasySynth, Log, TEXT("%s: %s"), *FString(__FUNCTION__), *ErrorMessage)
	// 	return false;
	// }

	// Get the movie rendering editor subsystem
	UMoviePipelineQueueSubsystem* MoviePipelineQueueSubsystem =
		GEditor->GetEditorSubsystem<UMoviePipelineQueueSubsystem>();
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

	// Load the asset representing needed movie pipeline configuration
	UMoviePipelineMasterConfig* EasySynthMoviePipelineConfig =
		LoadObject<UMoviePipelineMasterConfig>(nullptr, *EasySynthMoviePipelineConfigPath);
	if (EasySynthMoviePipelineConfig == nullptr)
	{
		ErrorMessage = "Could not load the EasySynthMoviePipelineConfig";
		UE_LOG(LogEasySynth, Error, TEXT("%s: %s"), *FString(__FUNCTION__), *ErrorMessage)
		return false;
	}

	// Iterate the queue
	for (UMoviePipelineExecutorJob* MoviePipelineExecutorJob : MoviePipelineQueue->GetJobs())
	{
		// Get job configuration
		if (MoviePipelineExecutorJob == nullptr)
		{
			ErrorMessage = "A job inside the queue is null";
			UE_LOG(LogEasySynth, Log, TEXT("%s: %s"), *FString(__FUNCTION__), *ErrorMessage)
			return false;
		}

		UMoviePipelineMasterConfig* MoviePipelineMasterConfig = MoviePipelineExecutorJob->GetConfiguration();
		UE_LOG(LogEasySynth, Log, TEXT("%s: Old configuration %d"), *FString(__FUNCTION__), MoviePipelineMasterConfig)

		MoviePipelineExecutorJob->SetConfiguration(EasySynthMoviePipelineConfig);

		MoviePipelineMasterConfig = MoviePipelineExecutorJob->GetConfiguration();
		UE_LOG(LogEasySynth, Log, TEXT("%s: New configuration %d"), *FString(__FUNCTION__), MoviePipelineMasterConfig)
	}

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
		return false;
	}

	// Assign rendering finished callback
	ActiveExecutor->OnExecutorFinished().AddRaw(this, &FSequenceRenderer::OnExecutorFinished);

	UE_LOG(LogEasySynth, Log, TEXT("%s: Rendering..."), *FString(__FUNCTION__))
	bCurrentlyRendering = true;

	return true;
}

void FSequenceRenderer::OnExecutorFinished(UMoviePipelineExecutorBase* InPipelineExecutor, bool bSuccess)
{
	UE_LOG(LogEasySynth, Log, TEXT("%s: Success: %d"), *FString(__FUNCTION__), bSuccess)
	bCurrentlyRendering = false;
}
