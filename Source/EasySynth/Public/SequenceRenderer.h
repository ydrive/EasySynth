// Copyright Ydrive 2021

#pragma once

#include "CoreMinimal.h"

#include "SequenceRenderer.generated.h"

class ULevelSequence;
class UMoviePipelineExecutorBase;
class UMoviePipelineMasterConfig;
class UMoviePipelineQueueSubsystem;


/** Delegate type used to broadcast the rendering finished event */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FDelegateRenderingFinished, bool, bSuccess);


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
		case COLOR_IMAGE: return TEXT("ColorImage"); break;
		case DEPTH_IMAGE: return TEXT("DepthImage"); break;
		case NORMAL_IMAGE: return TEXT("NormalImage"); break;
		case SEMANTIC_IMAGE: return TEXT("SemanticImage"); break;
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
UCLASS()
class USequenceRenderer : public UObject
{
	GENERATED_BODY()

public:
	/** Initialize the SequenceRenderer */
	USequenceRenderer();

	/** Runs sequence rendering, returns false if rendering could not start */
	bool RenderSequence(
		ULevelSequence* LevelSequence,
		FSequenceRendererTargets RenderingTargets,
		const FString& OutputDirectory);

	/** Returns the latest error message */
	const FString& GetErrorMessage() const { return ErrorMessage; }

private:
	/** Movie rendering finished handle */
	void OnExecutorFinished(UMoviePipelineExecutorBase* InPipelineExecutor, bool bSuccess);

	/** Handles finding the next target to be rendered */
	void FindNextTarget();

	/** Runs the rendering of the currently selected target */
	void StartRendering();

	/** Finalizes rendering and broadcasts the event */
	void BroadcastRenderingFinished(bool bSuccess);

	/** Rendering finished event dispatcher */
	FDelegateRenderingFinished DelegateRenderingFinished;

	/** Default movie pipeline config file provided with the plugin content */
	UPROPERTY()
	UMoviePipelineMasterConfig* EasySynthMoviePipelineConfig;

	/** Points to the user-created level sequence */
	UPROPERTY()
	ULevelSequence* RenderingSequence;

	/** SequenceRenderer copy of the requested rendering targets */
	FSequenceRendererTargets RequestedSequenceRendererTargets;

	/** Currently selected output directory */
	FString RenderingDirectory;

	/** Marks if rendering is currently in process */
	bool bCurrentlyRendering;

	/** Indicates the current target as an index of the FSequenceRendererTargets::TargetType enum */
	int CurrentTarget;

	/** Handle for a timer needed to make a brief pause between targets */
	FTimerHandle RendererPauseTimerHandle;

	/** Stores the latest error message */
	FString ErrorMessage;

	/** Movie pipeline configuration asset path */
	static const FString EasySynthMoviePipelineConfigPath;
};
