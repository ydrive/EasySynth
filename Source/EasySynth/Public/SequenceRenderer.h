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


/**
 * Class Tracking which renderer targets are requested to be rendered
*/
class FRendererTargetOptions
{
public:
	/** The enum containing all supported rendering targets */
	enum TargetType { COLOR_IMAGE, DEPTH_IMAGE, NORMAL_IMAGE, SEMANTIC_IMAGE, COUNT };

	FRendererTargetOptions();

	/** Select a rendering target */
	void SetSelectedTarget(int TargetType, bool Selected) { SelectedTargets[TargetType] = Selected; }

	/** Check if a rendering target is selected */
	bool TargetSelected(int TargetType) const { return SelectedTargets[TargetType]; }

	/** Checks if any of the available options is selected */
	bool AnyOptionSelected() const;

	/** Populate provided queue with selected renderer targets */
	void GetSelectedTargets(TQueue<TSharedPtr<FRendererTarget>>& OutTargetsQueue) const;

	/** The clipping range when rendering the depth target */
	float DepthRangeMetersValue;

private:
	/** Get the renderer target object from the target type id */
	TSharedPtr<FRendererTarget> RendererTarget(const int TargetType) const;

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
		const FRendererTargetOptions RenderingTargets,
		const FString& OutputDirectory);

	/** Checks if the rendering is currently in progress */
	bool IsRendering() const { return bCurrentlyRendering; }

	/** Returns the latest error message */
	const FString& GetErrorMessage() const { return ErrorMessage; }

	/** Delegate type used to broadcast the rendering finished event */
	DECLARE_EVENT_OneParam(USequenceRenderer, FRenderingFinishedEvent, bool);

	/** Returns a reference to the event for others to bind */
	FRenderingFinishedEvent& OnRenderingFinished() { return RenderingFinishedEvent; }

private:
	/** Movie rendering finished handle */
	void OnExecutorFinished(UMoviePipelineExecutorBase* InPipelineExecutor, bool bSuccess);

	/** Handles finding the next target to be rendered */
	void FindNextTarget();

	/** Runs the rendering of the currently selected target */
	void StartRendering();

	/** Finalizes rendering and broadcasts the event */
	void BroadcastRenderingFinished(const bool bSuccess);

	/** Rendering finished event dispatcher */
	FRenderingFinishedEvent RenderingFinishedEvent;

	/** Default movie pipeline config file provided with the plugin content */
	UPROPERTY()
	UMoviePipelineMasterConfig* EasySynthMoviePipelineConfig;

	/** Points to the user-created level sequence */
	UPROPERTY()
	ULevelSequence* RenderingSequence;

	/** Queue of targets to be rendered */
	TQueue<TSharedPtr<FRendererTarget>> TargetsQueue;

	/** Target currently being rendered */
	TSharedPtr<FRendererTarget> CurrentTarget;

	/** Currently selected output directory */
	FString RenderingDirectory;

	/** Marks if rendering is currently in process */
	bool bCurrentlyRendering;

	/** Handle for a timer needed to make a brief pause between targets */
	FTimerHandle RendererPauseTimerHandle;

	/** Stores the latest error message */
	FString ErrorMessage;

	/** Movie pipeline configuration asset path */
	static const FString EasySynthMoviePipelineConfigPath;
};
