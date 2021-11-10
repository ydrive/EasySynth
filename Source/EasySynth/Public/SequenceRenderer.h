// Copyright Ydrive 2021

#pragma once

#include "CoreMinimal.h"

#include "RendererTargets/RendererTarget.h"
#include "RendererTargets/ColorImageTarget.h"
#include "RendererTargets/DepthImageTarget.h"
#include "RendererTargets/NormalImageTarget.h"
#include "RendererTargets/SemanticImageTarget.h"

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
class FRendererTargetOptions
{
public:
	/** The enum containing all supported rendering targets */
	enum TargetType { COLOR_IMAGE, DEPTH_IMAGE, NORMAL_IMAGE, SEMANTIC_IMAGE, COUNT };

	FRendererTargetOptions() { SelectedTargets.Init(false, TargetType::COUNT); }

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

	static TSharedPtr<FRendererTarget> RendererTarget(const int TargetType)
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
		FRendererTargetOptions RenderingTargets,
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
	FRendererTargetOptions RequestedSequenceRendererTargets;

	/** Currently selected output directory */
	FString RenderingDirectory;

	/** Marks if rendering is currently in process */
	bool bCurrentlyRendering;

	/** Indicates the current target as an index of the FRendererTargetOptions::TargetType enum */
	int CurrentTarget;

	/** Handle for a timer needed to make a brief pause between targets */
	FTimerHandle RendererPauseTimerHandle;

	/** Stores the latest error message */
	FString ErrorMessage;

	/** Movie pipeline configuration asset path */
	static const FString EasySynthMoviePipelineConfigPath;
};
