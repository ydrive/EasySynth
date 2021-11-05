// Copyright Ydrive 2021

#pragma once

#include "CoreMinimal.h"

class ULevelSequence;
class UMoviePipelineExecutorBase;


/**
 * Class Tracking which renderer targets are requested to be rendered
*/
class FSequenceRendererTargets
{
public:
	/** The enum containing all supported rendering targets */
	enum TargetType { COLOR_IMAGE, DEPTH_IMAGE, NORMAL_IMAGE, SEMANTIC_IMAGE, COUNT };

	FSequenceRendererTargets() { SelectedTargets.Init(false, TargetType::COUNT); }

	/** Select a rendering target */
	void SetSelectedTarget(TargetType Target, bool Selected) { SelectedTargets[Target] = Selected; }

	/** Check if a rendering target is selected */
	bool TargetSelected(TargetType Target) { return SelectedTargets[Target]; }

	/** Checks if any of the available options is selected */
	bool AnyOptionSelected() const
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

private:
	/** Is the default color image rendering requested */
	TArray<bool> SelectedTargets;
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
