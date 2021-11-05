// Copyright Ydrive 2021

#pragma once

#include "CoreMinimal.h"

class ULevelSequence;
class UMoviePipelineExecutorBase;
class UMoviePipelineQueueSubsystem;


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
	void SetSelectedTarget(int TargetType, bool Selected) { SelectedTargets[TargetType] = Selected; }

	/** Check if a rendering target is selected */
	bool TargetSelected(int TargetType) const { return SelectedTargets[TargetType]; }

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

	/** Returns a readable name of a target type */
	static FString TargetName(int TargetType)
	{
		switch (TargetType)
		{
		case COLOR_IMAGE: return "color image"; break;
		case DEPTH_IMAGE: return "depth image"; break;
		case NORMAL_IMAGE: return "normal image"; break;
		case SEMANTIC_IMAGE: return "semantic image"; break;
		default: return "";
		}
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

	/** Handles finding and running the next rendering target */
	bool StartRenderingNextTarget();

	/** Keeps a pointer to the UMoviePipelineQueueSubsystem */
	UMoviePipelineQueueSubsystem* MoviePipelineQueueSubsystem;

	/** Marks if rendering is currently in process */
	bool bCurrentlyRendering;

	/** SequenceRenderer copy of the requested rendering targets */
	FSequenceRendererTargets RequestedSequenceRendererTargets;

	/** FSequenceRendererTargets::TargetType */
	int CurrentTarget;

	/** Stores the latest error message */
	FString ErrorMessage;

	/** Movie pipeline configuration asset path */
	static const FString EasySynthMoviePipelineConfigPath;
};
