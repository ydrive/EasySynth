// Copyright Ydrive 2021

#include "SequenceRenderer.h"

#include "MoviePipelineQueueSubsystem.h"
#include "MovieRenderPipelineSettings.h"


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
	UMoviePipelineQueueSubsystem* Subsystem = GEditor->GetEditorSubsystem<UMoviePipelineQueueSubsystem>();
	if (Subsystem == nullptr)
	{
		ErrorMessage = "Could not get the UMoviePipelineQueueSubsystem";
		UE_LOG(LogEasySynth, Log, TEXT("%s: %s"), *FString(__FUNCTION__), *ErrorMessage)
		return false;
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
		Subsystem->RenderQueueWithExecutor(ProjectSettings->DefaultLocalExecutor);
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
