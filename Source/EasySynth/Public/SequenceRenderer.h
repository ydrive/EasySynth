// Copyright Ydrive 2021

#pragma once

#include "CoreMinimal.h"

class ULevelSequence;
class UMoviePipelineExecutorBase;


/**
 * Class Tracking which renderer targets are requested to be rendered
*/
struct FSequenceRendererTargets
{
	FSequenceRendererTargets() :
		bColorImages(false)
	{}

	/** Checks if any of the available options is selected */
	bool AnyOptionSelected() const
	{
		return bColorImages;
	}

	/** Is the default color image rendering requested */
	bool bColorImages;
};


/**
 * Class that runs sequence rendering
*/
class FSequenceRenderer
{
public:
	/** Initialize the SequenceRenderer */
	FSequenceRenderer();

	/** Runs sequence rendering, returns false if rendering could not start */
	bool RenderSequence(ULevelSequence* LevelSequence, FSequenceRendererTargets RenderingTargets);

	/** Returns the latest error message */
	const FString& GetErrorMessage() const { return ErrorMessage; }

private:
	/** Movie rendering finished handle */
	void OnExecutorFinished(UMoviePipelineExecutorBase* InPipelineExecutor, bool bSuccess);

	/** Marks if rendering is currently in process */
	bool bCurrentlyRendering;

	/** Stores the latest error message */
	FString ErrorMessage;

	/** Movie pipeline configuration asset path */
	static const FString EasySynthMoviePipelineConfigPath;
};
