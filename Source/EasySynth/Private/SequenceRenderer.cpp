// Copyright Ydrive 2021

#include "SequenceRenderer.h"


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
	if (LevelSequence == nullptr)
	{
		ErrorMessage = "Provided level sequence is null";
		UE_LOG(LogEasySynth, Log, TEXT("%s: %s"), *FString(__FUNCTION__), *ErrorMessage)
		return false;
	}

	UE_LOG(LogEasySynth, Log, TEXT("%s: Rendering..."), *FString(__FUNCTION__))
	return true;
}
