// Copyright (c) 2022 YDrive Inc. All rights reserved.

#pragma once

#include "CoreMinimal.h"

#include "RendererTargets/RendererTarget.h"
#include "RendererTargets/ColorImageTarget.h"
#include "RendererTargets/DepthImageTarget.h"
#include "RendererTargets/NormalImageTarget.h"
#include "RendererTargets/OpticalFlowImageTarget.h"
#include "RendererTargets/SemanticImageTarget.h"
#include "TextureStyles/TextureStyleManager.h"

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
	enum TargetType { COLOR_IMAGE, DEPTH_IMAGE, NORMAL_IMAGE, OPTICAL_FLOW_IMAGE, SEMANTIC_IMAGE, COUNT };

	FRendererTargetOptions();

	/** Select a rendering target */
	void SetSelectedTarget(const int TargetType, const bool Selected) { SelectedTargets[TargetType] = Selected; }

	/** Check if a rendering target is selected */
	bool TargetSelected(const int TargetType) const { return SelectedTargets[TargetType]; }

	/** Checks if any of the available options is selected */
	bool AnyOptionSelected() const;

	/** Set output format for the target */
	void SetOutputFormat(const int TargetType, const EImageFormat Selected) { OutputFormats[TargetType] = Selected; }

	/** Get selected output format for the target */
	EImageFormat OutputFormat(const int TargetType) const { return OutputFormats[TargetType]; }

	/** Updates should camera poses be exported */
	void SetExportCameraPoses(const bool bValue) { bExportCameraPoses = bValue; }

	/** Return should camera poses be exported */
	bool ExportCameraPoses() const { return bExportCameraPoses; }

	/** DepthRangeMetersValue getter */
	void SetDepthRangeMeters(const float DepthRangeMeters) { DepthRangeMetersValue = DepthRangeMeters; }

	/** DepthRangeMetersValue setter */
	float DepthRangeMeters() const { return DepthRangeMetersValue; }

	/** OpticalFlowScaleValue getter */
	void SetOpticalFlowScale(const float OpticalFlowScale) { OpticalFlowScaleValue = OpticalFlowScale; }

	/** OpticalFlowScaleValue setter */
	float OpticalFlowScale() const { return OpticalFlowScaleValue; }

	/** Populate provided queue with selected renderer targets */
	void GetSelectedTargets(
		UTextureStyleManager* TextureStyleManager,
		TQueue<TSharedPtr<FRendererTarget>>& OutTargetsQueue) const;

private:
	/** Get the renderer target object from the target type id */
	TSharedPtr<FRendererTarget> RendererTarget(const int TargetType, UTextureStyleManager* TextureStyleManager) const;

	/** Is the default color image rendering requested */
	TArray<bool> SelectedTargets;

	/** Selected output formats for each target */
	TArray<EImageFormat> OutputFormats;

	/** Whether to export camera poses */
	bool bExportCameraPoses;

	/**
	 * The clipping range when rendering the depth target
	 * Larger values provide the longer range, but also the lower granularity
	*/
	float DepthRangeMetersValue;

	/**
	 * Multiplying coefficient for optical flow
	 * Larger values increase color intensity, but also increase the chance of clipping
	*/
	float OpticalFlowScaleValue;

	/** Default value for the depth range */
	static const float DefaultDepthRangeMetersValue;

	/** Default value for the optical flow scale */
	static const float DefaultOpticalFlowScaleValue;
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

	/** Sets TextureStyleManager */
	void SetTextureStyleManager(UTextureStyleManager* Value) { TextureStyleManager = Value; }

	/** Runs sequence rendering, returns false if rendering could not start */
	bool RenderSequence(
		ULevelSequence* LevelSequence,
		const FRendererTargetOptions RenderingTargets,
		const FIntPoint OutputImageResolution,
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

	/** Handles finding the next rig camera to be used for rendering all requested targets */
	void FindNextCamera();

	/** Handles finding the next target to be rendered by the current camera */
	void FindNextTarget();

	/** Runs the rendering of the currently selected target */
	void StartRendering();

	/** Clears the existing job queue and adds a fresh job */
	bool PrepareJobQueue(UMoviePipelineQueueSubsystem* MoviePipelineQueueSubsystem);

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

	/** Keeps current rendering options */
	FRendererTargetOptions RendererTargetOptions;

	/** TextureStyleManager needed to be finalize the rendering */
	UTextureStyleManager* TextureStyleManager;

	/** Used to revert to this style after finishing the rendering */
	ETextureStyle OriginalTextureStyle;

	/** Points to the actor that serves as a camera source for the sequencer */
	UPROPERTY()
	AActor* CameraRigActor;

	/** Keeps all CameraRigActor's camera components during rendering to avoid getting them each time */
	UPROPERTY()
	TArray<UCameraComponent*> RigCameras;

	/** Keeps the original camera transform so it can be restored at the end */
	FTransform OriginalCameraTransform;

	/** Keeps the original camera field of view so it can be restored at the end */
	double OriginalCameraFOV;

	/** Keeps the currently selected rig camera */
	int CurrentRigCameraId;

	/** Queue of targets to be rendered */
	TQueue<TSharedPtr<FRendererTarget>> TargetsQueue;

	/** Target currently being rendered */
	TSharedPtr<FRendererTarget> CurrentTarget;

	/** Output image resolution */
	FIntPoint OutputResolution;

	/** Currently selected output directory */
	FString RenderingDirectory;

	/** Marks if rendering is currently in process */
	bool bCurrentlyRendering;

	/** Handle for a timer needed to make a brief pause between targets */
	FTimerHandle RendererPauseTimerHandle;

	/** Stores the latest error message */
	FString ErrorMessage;
};
