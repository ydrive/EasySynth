// Copyright (c) 2022 YDrive Inc. All rights reserved.

#include "SequenceRenderer.h"

#include "CineCameraComponent.h"
#include "MoviePipelineImageSequenceOutput.h"
#include "MoviePipelineOutputSetting.h"
#include "MoviePipelineQueueSubsystem.h"
#include "MovieRenderPipelineSettings.h"
#include "MoviePipelineDeferredPasses.h"
#include "MoviePipelineAntiAliasingSetting.h"

#include "EXROutput/MoviePipelineEXROutputLocal.h"
#include "PathUtils.h"
#include "RendererTargets/CameraPoseExporter.h"
#include "RendererTargets/RendererTargetSet.h"
#include "RendererTargets/NonSemanticTargetSet.h"
#include "RendererTargets/SemanticTargetSet.h"
#include "TextureStyles/SemanticCsvInterface.h"


const float FRendererTargetOptions::DefaultDepthRangeMetersValue = 100.0f;
const float FRendererTargetOptions::DefaultOpticalFlowScaleValue = 1.0f;

FRendererTargetOptions::FRendererTargetOptions() :
	bExportCameraPoses(false),
	DepthRangeMetersValue(DefaultDepthRangeMetersValue),
	OpticalFlowScaleValue(DefaultOpticalFlowScaleValue)
{
	SelectedTargets.Init(false, TargetType::TARGET_COUNT);
	OutputFormats.Init(EImageFormat::JPEG, TargetType::TARGET_COUNT);
}

bool FRendererTargetOptions::AnyOptionSelected() const
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

void FRendererTargetOptions::GetTargetSets(
	UTextureStyleManager* TextureStyleManager,
	TQueue<TSharedPtr<FRendererTargetSet>>& OutTargetsQueue) const
{
	OutTargetsQueue.Empty();

	OutTargetsQueue.Enqueue(
		MakeShared<FNonSemanticTargetSet>(
			TextureStyleManager,
			OutputFormats[TargetType::COLOR_IMAGE],
			DepthRangeMetersValue,
			OpticalFlowScaleValue,
			SelectedTargets[TargetType::DEPTH_IMAGE],
			SelectedTargets[TargetType::NORMAL_IMAGE],
			SelectedTargets[TargetType::OPTICAL_FLOW_IMAGE]));

	if (SelectedTargets[TargetType::SEMANTIC_IMAGE])
	{
		OutTargetsQueue.Enqueue(
			MakeShared<FSemanticTargetSet>(
				TextureStyleManager,
				OutputFormats[TargetType::SEMANTIC_IMAGE]));
	}
}

USequenceRenderer::USequenceRenderer() :
	EasySynthMoviePipelineConfig(DuplicateObject<UMoviePipelineMasterConfig>(
		LoadObject<UMoviePipelineMasterConfig>(nullptr, *FPathUtils::DefaultMoviePipelineConfigPath()), nullptr)),
	bCurrentlyRendering(false),
	ErrorMessage("")
{
	// Check if the config asset is loaded correctly
	if (EasySynthMoviePipelineConfig == nullptr)
	{
		ErrorMessage = "Could not load the EasySynthMoviePipelineConfig";
		UE_LOG(LogEasySynth, Error, TEXT("%s: %s"), *FString(__FUNCTION__), *ErrorMessage)
		check(EasySynthMoviePipelineConfig)
	}
}

bool USequenceRenderer::RenderSequence(
	ULevelSequence* LevelSequence,
	const FRendererTargetOptions RenderingTargets,
	const FIntPoint OutputImageResolution,
	const FString& OutputDirectory)
{
	UE_LOG(LogEasySynth, Log, TEXT("%s"), *FString(__FUNCTION__))

	if (TextureStyleManager == nullptr)
	{
		UE_LOG(LogEasySynth, Error, TEXT("%s: Texture style manager is null"), *FString(__FUNCTION__))
		check(TextureStyleManager)
	}

	// Check if rendering is already in progress
	if (bCurrentlyRendering)
	{
		ErrorMessage = "Rendering already in progress";
		UE_LOG(LogEasySynth, Warning, TEXT("%s: %s"), *FString(__FUNCTION__), *ErrorMessage)
		return false;
	}

	// Check if LevelSequence is valid
	RenderingSequence = LevelSequence;
	if (RenderingSequence == nullptr)
	{
		ErrorMessage = "Provided level sequence is null";
		UE_LOG(LogEasySynth, Warning, TEXT("%s: %s"), *FString(__FUNCTION__), *ErrorMessage)
		return false;
	}

	// Check if any rendering target is selected
	if (!RenderingTargets.AnyOptionSelected())
	{
		ErrorMessage = "No rendering targets selected";
		UE_LOG(LogEasySynth, Warning, TEXT("%s: %s"), *FString(__FUNCTION__), *ErrorMessage)
		return false;
	}

	// Store parameters
	RendererTargetOptions = RenderingTargets;
	OutputResolution = OutputImageResolution;
	RenderingDirectory = OutputDirectory;

	// Find the sequencer source actor
	FSequencerWrapper SequencerWrapper;
	if (!SequencerWrapper.OpenSequence(LevelSequence))
	{
		ErrorMessage = "Sequencer wrapper opening failed";
		UE_LOG(LogEasySynth, Warning, TEXT("%s: %s"), *FString(__FUNCTION__), *ErrorMessage)
		return false;
	}

	// Assume the same source actor is used for all camera cuts
	UMovieSceneCameraCutSection* CutSection = SequencerWrapper.GetMovieSceneCutSections()[0];

	// Get the sequence source actor
	TArrayView<TWeakObjectPtr<>> SourceObjects = SequencerWrapper.GetSequencer()->FindBoundObjects(
		CutSection->GetCameraBindingID().GetGuid(),
		SequencerWrapper.GetSequencer()->GetFocusedTemplateID());
	if (SourceObjects.Num() == 0)
	{
		ErrorMessage = "No sources assigned to the sequencer";
		UE_LOG(LogEasySynth, Warning, TEXT("%s: %s"), *FString(__FUNCTION__), *ErrorMessage)
		return false;
	}

	// Assume the same source actor is used throughout the camera cut
	CameraRigActor = Cast<AActor>(SourceObjects[0].Get());
	if (CameraRigActor == nullptr)
	{
		ErrorMessage = "Expected an actor as a sequence source";
		UE_LOG(LogEasySynth, Warning, TEXT("%s: %s"), *FString(__FUNCTION__), *ErrorMessage)
		return false;
	}

	// Prepare rig cameras information
	RigCameras.Empty();
	CurrentRigCameraId = -1;

	// Find all camera components inside the source actor
	TArray<UActorComponent*> ActorComponents;
	const bool bIncludeFromChildActors = true;
	CameraRigActor->GetComponents(UCameraComponent::StaticClass(), ActorComponents, bIncludeFromChildActors);

	// If no mesh components are found, ignore the actor
	if (ActorComponents.Num() == 0)
	{
		ErrorMessage = "No cameras found inside the actor";
		UE_LOG(LogEasySynth, Warning, TEXT("%s: %s"), *FString(__FUNCTION__), *ErrorMessage)
		return false;
	}

	// Store pointers to all cameras inside the rig for later selection of the active camera
	for (UActorComponent* ActorComponent : ActorComponents)
	{
		UCameraComponent* CameraComponent = Cast<UCameraComponent>(ActorComponent);
		if (CameraComponent == nullptr)
		{
			ErrorMessage = "Got null camera component";
			UE_LOG(LogEasySynth, Error, TEXT("%s: %s"), *FString(__FUNCTION__), *ErrorMessage)
			return false;
		}

		// Set required camera aspect ratio
		const float AspectRatio = 1.0f * OutputResolution.X / OutputResolution.Y;
		UCineCameraComponent* CineCameraComponent = Cast<UCineCameraComponent>(CameraComponent);
		if (CineCameraComponent != nullptr)
		{
			// Additional steps needed for cine cameras
			// Freeze the sensor width and adjust its height
			auto& Filmback = CineCameraComponent->Filmback;
			Filmback.SensorHeight = Filmback.SensorWidth / AspectRatio;
			Filmback.SensorAspectRatio = Filmback.SensorWidth / Filmback.SensorHeight;
		}
		CameraComponent->SetAspectRatio(AspectRatio);
		CameraComponent->SetConstraintAspectRatio(true);

		RigCameras.Add(CameraComponent);
	}

	// Make sure the camera rig array is sorted by the camera id
	RigCameras.Sort([](UCameraComponent& A, UCameraComponent& B)
	{
		return A.GetReadableName().Compare(B.GetReadableName()) < 0;
	});

	// Export camera rig information
	FCameraRigRosInterface CameraRigRosInterface;
	if (!CameraRigRosInterface.ExportCameraRig(RenderingDirectory, RigCameras, OutputResolution))
	{
		ErrorMessage = "Could not save the camera rig ROS JSON file";
		UE_LOG(LogEasySynth, Error, TEXT("%s: %s"), *FString(__FUNCTION__), *ErrorMessage)
		return false;
	}

	// Export camera rig poses if requested
	if (RendererTargetOptions.ExportCameraPoses())
	{
		FCameraPoseExporter CameraPoseExporter;
		UCameraComponent* NoSpecificCamera = nullptr;
		if (!CameraPoseExporter.ExportCameraPoses(
			RenderingSequence, OutputResolution, RenderingDirectory, NoSpecificCamera))
		{
			ErrorMessage = "Could not export camera rig poses";
			UE_LOG(LogEasySynth, Error, TEXT("%s: %s"), *FString(__FUNCTION__), *ErrorMessage)
			return false;
		}
	}

	// Export semantic class information if semantic rendering is selected
	if (RendererTargetOptions.TargetSelected(FRendererTargetOptions::TargetType::SEMANTIC_IMAGE))
	{
		if (!TextureStyleManager->ExportSemanticClasses(RenderingDirectory))
		{
			ErrorMessage = "Could not save the semantic class CSV file";
			UE_LOG(LogEasySynth, Error, TEXT("%s: %s"), *FString(__FUNCTION__), *ErrorMessage)
			return false;
		}
	}

	OriginalTextureStyle = TextureStyleManager->SelectedTextureStyle();

	UE_LOG(LogEasySynth, Log, TEXT("%s: Rendering..."), *FString(__FUNCTION__))
	bCurrentlyRendering = true;

	FindNextCamera();

	return true;
}

void USequenceRenderer::OnExecutorFinished(UMoviePipelineExecutorBase* InPipelineExecutor, bool bSuccess)
{
	// Revert target specific modifications to the sequence
	if (!CurrentTargetSet->FinalizeSequence(RenderingSequence))
	{
		ErrorMessage = FString::Printf(TEXT("Failed while finalizing the rendering of the %s target set"), *CurrentTargetSet->Name());
		return BroadcastRenderingFinished(false);
	}

	if (!bSuccess)
	{
		ErrorMessage = FString::Printf(TEXT("Failed while rendering the %s target set"), *CurrentTargetSet->Name());
		return BroadcastRenderingFinished(false);
	}

	UE_LOG(LogEasySynth, Log, TEXT("%s: Looking for the next camera"), *FString(__FUNCTION__))

	// Successful rendering, proceed to the next target
	FindNextTarget();
}

void USequenceRenderer::FindNextCamera()
{
	CurrentRigCameraId++;

	// Check if the end is reached
	if (CurrentRigCameraId == RigCameras.Num())
	{
		UE_LOG(LogEasySynth, Log, TEXT("%s: End of camera array reached"), *FString(__FUNCTION__))
		return BroadcastRenderingFinished(true);
	}

	if (CurrentRigCameraId == 0)
	{
		// Remember the transform of the first camera
		OriginalCameraTransform = RigCameras[0]->GetRelativeTransform();
		OriginalCameraFOV = RigCameras[0]->FieldOfView;
	}
	else
	{
		// Transfer the transform of the current camera to the first one that is used for rendering
		RigCameras[0]->SetRelativeTransform(RigCameras[CurrentRigCameraId]->GetRelativeTransform());
		RigCameras[0]->SetFieldOfView(RigCameras[CurrentRigCameraId]->FieldOfView);
	}

	// Export camera poses if requested
	if (RendererTargetOptions.ExportCameraPoses())
	{
		FCameraPoseExporter CameraPoseExporter;
		if (!CameraPoseExporter.ExportCameraPoses(
			RenderingSequence, OutputResolution, RenderingDirectory, RigCameras[CurrentRigCameraId]))
		{
			ErrorMessage = "Could not export camera poses";
			return BroadcastRenderingFinished(false);
		}
	}

	// Prepare the targets queue
	RendererTargetOptions.GetTargetSets(TextureStyleManager, TargetSetsQueue);
	CurrentTargetSet = nullptr;

	UE_LOG(LogEasySynth, Log, TEXT("%s: Rendering camera %d/%d"), *FString(__FUNCTION__), CurrentRigCameraId + 1, RigCameras.Num())

	// Find the next target and start rendering
	FindNextTarget();
}

void USequenceRenderer::FindNextTarget()
{
	// Check if the end is reached
	if (TargetSetsQueue.IsEmpty())
	{
		return FindNextCamera();
	}

	// Select the next requested target
	TargetSetsQueue.Dequeue(CurrentTargetSet);

	// Setup specifics of the current rendering target
	UE_LOG(LogEasySynth, Log, TEXT("%s: Rendering the %s target"), *FString(__FUNCTION__), *CurrentTargetSet->Name())
	if (!CurrentTargetSet->PrepareSequence(RenderingSequence))
	{
		ErrorMessage = FString::Printf(TEXT("Failed while preparing the rendering of the %s target"), *CurrentTargetSet->Name());
		return BroadcastRenderingFinished(false);
	}

	// Start the rendering after a brief pause for texture changes to take effect
	const float DelaySeconds = 2.0f;
	const bool bLoop = false;
	GEditor->GetEditorWorldContext().World()->GetTimerManager().SetTimer(
		RendererPauseTimerHandle,
		this,
		&USequenceRenderer::StartRendering,
		DelaySeconds,
		bLoop);
}

void USequenceRenderer::StartRendering()
{
	// Make sure the sequence is still sound
	if (RenderingSequence == nullptr)
	{
		ErrorMessage = "Provided level sequence is null when starting the recording";
		return BroadcastRenderingFinished(false);
	}

	// Make sure a renderer target is selected
	if (!CurrentTargetSet.IsValid())
	{
		ErrorMessage = "Current renderer target set is null when starting the recording";
		return BroadcastRenderingFinished(false);
	}

	// Get the movie rendering editor subsystem
	UMoviePipelineQueueSubsystem* MoviePipelineQueueSubsystem =
		GEditor->GetEditorSubsystem<UMoviePipelineQueueSubsystem>();
	if (MoviePipelineQueueSubsystem == nullptr)
	{
		ErrorMessage = "Could not get the UMoviePipelineQueueSubsystem";
		return BroadcastRenderingFinished(false);
	}

	// Add received level sequence to the queue as a new job
	if (!PrepareJobQueue(MoviePipelineQueueSubsystem))
	{
		// Propagate the error message set inside the PrepareJobQueue
		return BroadcastRenderingFinished(false);
	}

	// Get the default movie rendering settings
	const UMovieRenderPipelineProjectSettings* ProjectSettings = GetDefault<UMovieRenderPipelineProjectSettings>();
	if (ProjectSettings->DefaultLocalExecutor == nullptr)
	{
		ErrorMessage = "Could not get the UMovieRenderPipelineProjectSettings";
		return BroadcastRenderingFinished(false);
	}

	// Run the rendering
	UMoviePipelineExecutorBase* ActiveExecutor = MoviePipelineQueueSubsystem->RenderQueueWithExecutor(ProjectSettings->DefaultLocalExecutor);
	if (ActiveExecutor == nullptr)
	{
		ErrorMessage = "Could not start the rendering";
		return BroadcastRenderingFinished(false);
	}

	// Assign rendering finished callback
	ActiveExecutor->OnExecutorFinished().AddUObject(this, &USequenceRenderer::OnExecutorFinished);
}

bool USequenceRenderer::PrepareJobQueue(UMoviePipelineQueueSubsystem* MoviePipelineQueueSubsystem)
{
	check(MoviePipelineQueueSubsystem)

	// Update export image format
	UMoviePipelineSetting* JpgSetting = EasySynthMoviePipelineConfig->FindOrAddSettingByClass(
		UMoviePipelineImageSequenceOutput_JPG::StaticClass(), true);
	UMoviePipelineSetting* PngSetting = EasySynthMoviePipelineConfig->FindOrAddSettingByClass(
		UMoviePipelineImageSequenceOutput_PNG::StaticClass(), true);
	UMoviePipelineSetting* ExrSettingBase = EasySynthMoviePipelineConfig->FindOrAddSettingByClass(
		UMoviePipelineImageSequenceOutput_EXRLocal::StaticClass(), true);
	UMoviePipelineImageSequenceOutput_EXRLocal* ExrSetting = Cast<UMoviePipelineImageSequenceOutput_EXRLocal>(ExrSettingBase);

	if (PngSetting == nullptr || ExrSetting == nullptr)
	{
		ErrorMessage = "JPEG, PNG or EXR settings not found";
		return false;
	}
	
	JpgSetting->SetIsEnabled(true);
	PngSetting->SetIsEnabled(true);
	ExrSetting->SetIsEnabled(true);
	// Render each pass to a different EXR file
	ExrSetting->bMultilayer = false;

	UMoviePipelineDeferredPassBase* DeferredRenderSetting = EasySynthMoviePipelineConfig->FindSetting<UMoviePipelineDeferredPassBase>();

	if (DeferredRenderSetting == nullptr)
	{
		ErrorMessage = "DefferedRenderSetting settings not found";
		return false;
	}

	DeferredRenderSetting->AdditionalPostProcessMaterials.Empty();

	UMoviePipelineSetting* AntiAliasingBase = EasySynthMoviePipelineConfig->FindOrAddSettingByClass(
		UMoviePipelineAntiAliasingSetting::StaticClass(), true);
	UMoviePipelineAntiAliasingSetting* AntiAliasing = Cast<UMoviePipelineAntiAliasingSetting>(AntiAliasingBase);

	if (AntiAliasing == nullptr)
	{
		ErrorMessage = "AntiAliasing settings not found";
		return false;
	}

	// Only spatial samples matter in movie rendering pipeline
	AntiAliasing->SpatialSampleCount = 8;
	AntiAliasing->bOverrideAntiAliasing = true;
	AntiAliasing->AntiAliasingMethod = EAntiAliasingMethod::AAM_None;

	for (int i = 0; i < CurrentTargetSet->TargetNames().Num(); i++)
	{
		UMaterial* PostProcessMaterial = LoadObject<UMaterial>(
			nullptr,
			*FPathUtils::PostProcessMaterialPath(CurrentTargetSet->TargetNames()[i]));

		if (PostProcessMaterial == nullptr)
		{
			UE_LOG(LogEasySynth, Error, TEXT("%s: Could not load depth post process material"), *FString(__FUNCTION__))
			return false;
		}

		FMoviePipelinePostProcessPass targetPass;
		targetPass.bEnabled = true;
		targetPass.Material = PostProcessMaterial;
		DeferredRenderSetting->AdditionalPostProcessMaterials.Add(targetPass);
		DeferredRenderSetting->bUse32BitPostProcessMaterials = false;
		DeferredRenderSetting->bDisableMultisampleEffects = false; // true;
	}

	// Update pipeline output settings for the current target
	UMoviePipelineOutputSetting* OutputSetting = EasySynthMoviePipelineConfig->FindSetting<UMoviePipelineOutputSetting>();
	if (OutputSetting == nullptr)
	{
		ErrorMessage = "Could not find the output setting inside the default config";
		return false;
	}

	// Update the image output directory
	OutputSetting->OutputDirectory.Path = FPathUtils::RigCameraDir(RenderingDirectory, RigCameras[CurrentRigCameraId]);
	OutputSetting->OutputResolution = OutputResolution;
	OutputSetting->FileNameFormat = "{sequence_name}.{render_pass}.{frame_number}";

	// Get the queue of sequences to be renderer
	UMoviePipelineQueue* MoviePipelineQueue = MoviePipelineQueueSubsystem->GetQueue();
	if (MoviePipelineQueue == nullptr)
	{
		ErrorMessage = "Could not get the UMoviePipelineQueue";
		return false;
	}
	MoviePipelineQueue->Modify();

	// Clear the current queue
	TArray<UMoviePipelineExecutorJob*> ExistingJobs = MoviePipelineQueue->GetJobs();
	for (UMoviePipelineExecutorJob* MoviePipelineExecutorJob : ExistingJobs)
	{
		MoviePipelineQueue->DeleteJob(MoviePipelineExecutorJob);
	}
	if (MoviePipelineQueue->GetJobs().Num() > 0)
	{
		ErrorMessage = "Job queue not properly cleared";
		return false;
	}

	// Add received level sequence to the queue as a new job
	UMoviePipelineExecutorJob* NewJob = MoviePipelineQueue->AllocateNewJob(UMoviePipelineExecutorJob::StaticClass());
	if (NewJob == nullptr)
	{
		ErrorMessage = "Failed to create new rendering job";
		return false;
	}
	NewJob->Modify();
	NewJob->Map = FSoftObjectPath(GEditor->GetEditorWorldContext().World());
	NewJob->Author = FPlatformProcess::UserName(false);
	NewJob->SetSequence(RenderingSequence);
	NewJob->JobName = NewJob->Sequence.GetAssetName();

	// The SetConfiguration method creates and assigns the copy of the provided config
	NewJob->SetConfiguration(EasySynthMoviePipelineConfig);

	return true;
}

void USequenceRenderer::BroadcastRenderingFinished(const bool bSuccess)
{
	if (!bSuccess)
	{
		UE_LOG(LogEasySynth, Warning, TEXT("%s: %s"), *FString(__FUNCTION__), *ErrorMessage)
	}

	if (RigCameras.Num() > 0)
	{
		// Restore the transform of the original camera
		RigCameras[0]->SetRelativeTransform(OriginalCameraTransform);
		RigCameras[0]->SetFieldOfView(OriginalCameraFOV);
	}

	RigCameras.Empty();
	TargetSetsQueue.Empty();

	// Revert world state to the original one
	TextureStyleManager->CheckoutTextureStyle(OriginalTextureStyle);

	bCurrentlyRendering = false;
	RenderingFinishedEvent.Broadcast(bSuccess);
}
