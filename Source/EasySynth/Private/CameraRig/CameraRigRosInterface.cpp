// Copyright (c) 2022 YDrive Inc. All rights reserved.

#include "CameraRig/CameraRigRosInterface.h"

#include "CineCameraComponent.h"
#include "DesktopPlatformModule.h"
#include "IDesktopPlatform.h"
#include "JsonObjectConverter.h"
#include "Kismet/KismetMathLibrary.h"
#include "Misc/FileHelper.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"

#include "CameraRig/CameraRigData.h"
#include "EasySynth.h"


#define LOCTEXT_NAMESPACE "FCameraRigRosInterface"

FReply FCameraRigRosInterface::OnImportCameraRigClicked()
{
	UE_LOG(LogEasySynth, Log, TEXT("%s"), *FString(__FUNCTION__))

	// Get desktop platform
	void* ParentWindowPtr = FSlateApplication::Get().GetActiveTopLevelWindow()->GetNativeWindow()->GetOSWindowHandle();
	IDesktopPlatform* DesktopPlatform = FDesktopPlatformModule::Get();
	if (DesktopPlatform == nullptr)
	{
		UE_LOG(LogEasySynth, Error, TEXT("%s: Could not get the desktop platform"), *FString(__FUNCTION__))
		return FReply::Handled();
	}

	// Display file open dialog
	TArray<FString> OutFilenames;
	const bool IsFileSelected = DesktopPlatform->OpenFileDialog(
		ParentWindowPtr,
		TEXT("Import camera rig"),
		TEXT(""),
		TEXT(""),
		TEXT("Camera Rig ROS JSON (*.json)|*.json"),
		EFileDialogFlags::None,
		OutFilenames);
	if (!IsFileSelected)
	{
		return FReply::Handled();
	}

	// Helper method for displaying error messages
	auto DisplayError = [](const FText& Message) {
		const FText MessageBoxTitle = LOCTEXT("InvalidJsonMessageBoxTitle", "Failed to load camera rig");
		UE_LOG(LogEasySynth, Warning, TEXT("%s: %s"), *FString(__FUNCTION__), *Message.ToString())
		FMessageDialog::Open(EAppMsgType::Ok, Message, &MessageBoxTitle);
	};


	// Read the selected file
	FString FileContent;
	if (!FFileHelper::LoadFileToString(FileContent, *OutFilenames[0]))
	{
		const FText ErrorMessage = LOCTEXT("CannotReadFileError", "Failed to open ROS JSON file");
		DisplayError(ErrorMessage);
		return FReply::Handled();
	}

	// Parse JSON content into equivalent structure
	FRosJsonContent RosJsonContent;
	if (!FJsonObjectConverter::JsonObjectStringToUStruct(FileContent, &RosJsonContent, 0, 0))
	{
		const FText ErrorMessage = LOCTEXT("FileContentError", "Invalid ROS JSON file content");
		DisplayError(ErrorMessage);
		return FReply::Handled();
	}

	// Aggregated camera rig data
	FCameraRigData CameraRigData;

	for (auto& Element : RosJsonContent.cameras)
	{
		const FString& CameraName = Element.Key;
		const FRosJsonCamera& RosJsonCamera = Element.Value;

		FCameraRigData::FCameraData CameraData;
		CameraData.CameraName = CameraName;

		// Get the sensor size
		if (RosJsonCamera.sensor_size.Num() != 2)
		{
			const FText ErrorMessage = LOCTEXT("InvalidSensorError", "Expected 2 values for the sensor size");
			DisplayError(ErrorMessage);
			return FReply::Handled();
		}
		CameraData.SensorSize.X = RosJsonCamera.sensor_size[0];
		CameraData.SensorSize.Y = RosJsonCamera.sensor_size[1];

		// Ignore the "camera" if its sensor size is 0
		if (CameraData.SensorSize.X * CameraData.SensorSize.Y == 0)
		{
			continue;
		}

		// Get focal length
		if (RosJsonCamera.intrinsics.Num() != 9)
		{
			const FText ErrorMessage = LOCTEXT("InvalidSensorError", "Expected 9 values for camera intrinsics");
			DisplayError(ErrorMessage);
			return FReply::Handled();
		}
		CameraData.FocalLength = RosJsonCamera.intrinsics[0];
		CameraData.PrincipalPointX = RosJsonCamera.intrinsics[2];
		CameraData.PrincipalPointY = RosJsonCamera.intrinsics[5];

		// Get rotation
		if (RosJsonCamera.rotation.Num() != 4)
		{
			const FText ErrorMessage = LOCTEXT("InvalidSensorError", "Expected 4 values for camera rotation");
			DisplayError(ErrorMessage);
			return FReply::Handled();
		}
		CameraData.Transform.SetRotation(FQuat(
			RosJsonCamera.rotation[0],
			RosJsonCamera.rotation[1],
			RosJsonCamera.rotation[2],
			RosJsonCamera.rotation[3]));

		// Get translation
		if (RosJsonCamera.translation.Num() != 3)
		{
			const FText ErrorMessage = LOCTEXT("InvalidSensorError", "Expected 3 values for camera translation");
			DisplayError(ErrorMessage);
			return FReply::Handled();
		}
		CameraData.Transform.SetTranslation(FVector(
			RosJsonCamera.translation[0],
			RosJsonCamera.translation[1],
			RosJsonCamera.translation[2]));

		CameraRigData.Cameras.Add(CameraData);
	}

	// Spawn the camera rig actor
	AActor* CameraRigActor = Cast<AActor>(
		GEditor->AddActor(
			GEditor->GetEditorWorldContext().World()->GetCurrentLevel(),
			AActor::StaticClass(),
			FTransform(
				FRotator::ZeroRotator,
				FVector(0, 0, 0),
				FVector(1, 1, 1))));
	CameraRigActor->SetActorLabel("CameraRigActor");

	// Create the rig actor root component
	USceneComponent* RootComponent = NewObject<USceneComponent>(
		CameraRigActor,
		USceneComponent::GetDefaultSceneRootVariableName(),
		RF_Transactional);
	CameraRigActor->SetRootComponent(RootComponent);
	CameraRigActor->AddInstanceComponent(RootComponent);
	RootComponent->RegisterComponent();

	// Add camera components to the rig actor
	for (int i = 0; i < CameraRigData.Cameras.Num(); i++)
	{
		const FCameraRigData::FCameraData& Camera = CameraRigData.Cameras[i];

		UCineCameraComponent * CameraComponent = NewObject<UCineCameraComponent>(
			CameraRigActor,
			UCineCameraComponent::StaticClass(),
			*Camera.CameraName);
		CameraComponent->AttachToComponent(
			RootComponent,
			FAttachmentTransformRules::KeepWorldTransform);
		CameraRigActor->AddInstanceComponent(CameraComponent);
		CameraComponent->RegisterComponent();

		CameraComponent->SetRelativeTransform(Camera.Transform);

		// Calculate field of view in degrees
		const double FOV = 2 * UKismetMathLibrary::DegAtan2(Camera.SensorSize.X, Camera.FocalLength * 2.0f);
		CameraComponent->SetFieldOfView(FOV);

		// Make camera components smaller so that the rig is easier to visualize
		CameraComponent->SetRelativeScale3D(FVector(0.4f, 0.4f, 0.4f));
	}

	return FReply::Handled();
}

bool FCameraRigRosInterface::ExportCameraRig(
	const FString& OutputDir,
	TArray<UCameraComponent*> RigCameras,
	const FIntPoint& SensorSize)
{
	FRosJsonContent RosJsonContent;

	for (int i = 0; i < RigCameras.Num(); i++)
	{
		// Add each camera
		AddCamera(i, RigCameras[i], SensorSize, RosJsonContent);
	}

	FString JsonString;
	FJsonObjectConverter::UStructToJsonObjectString(RosJsonContent, JsonString);

	// Save the file
	const FString SaveFilePath = FPathUtils::CameraRigFilePath(OutputDir);
	if (!FFileHelper::SaveStringToFile(
		JsonString,
		*SaveFilePath,
		FFileHelper::EEncodingOptions::AutoDetect,
		&IFileManager::Get(),
		EFileWrite::FILEWRITE_None))
	{
		UE_LOG(LogEasySynth, Error, TEXT("%s: Failed while saving the file %s"), *FString(__FUNCTION__), *SaveFilePath)
		return false;
	}

	return true;
}

void FCameraRigRosInterface::AddCamera(
	const int CameraId,
	UCameraComponent* Camera,
	const FIntPoint& SensorSize,
	FRosJsonContent& RosJsonContent)
{
	FRosJsonCamera RosJsonCamera;

	// Add intrinsics
	const double FocalLength = SensorSize.X / UKismetMathLibrary::DegTan(Camera->FieldOfView / 2.0f) / 2.0f;
	const double PrincipalPointX = SensorSize.X / 2.0f;
	const double PrincipalPointY = SensorSize.Y / 2.0f;
	RosJsonCamera.intrinsics.Init(0, 9);
	RosJsonCamera.intrinsics[0] = FocalLength;
	RosJsonCamera.intrinsics[2] = PrincipalPointX;
	RosJsonCamera.intrinsics[4] = FocalLength;
	RosJsonCamera.intrinsics[5] = PrincipalPointY;
	RosJsonCamera.intrinsics[8] = 1.0f;

	// Prepare transform
	FTransform Transform = Camera->GetRelativeTransform();
	// Remove the scaling that makes no impact on camera functionality,
	// but my be used to scale the camera placeholder mesh as user desires
	Transform.SetScale3D(FVector(1.0f, 1.0f, 1.0f));

	// Add translation
	const FVector Translation = Transform.GetTranslation();
	RosJsonCamera.translation.Add(Translation.X);
	RosJsonCamera.translation.Add(Translation.Y);
	RosJsonCamera.translation.Add(Translation.Z);

	// Add rotation
	const FQuat Rotation = Transform.GetRotation();
	RosJsonCamera.rotation.Add(Rotation.X);
	RosJsonCamera.rotation.Add(Rotation.Y);
	RosJsonCamera.rotation.Add(Rotation.Z);
	RosJsonCamera.rotation.Add(Rotation.W);

	// Add sensor size
	RosJsonCamera.sensor_size.Add(SensorSize.X);
	RosJsonCamera.sensor_size.Add(SensorSize.Y);

	RosJsonContent.cameras.Add(FPathUtils::GetCameraName(Camera), RosJsonCamera);
}

#undef LOCTEXT_NAMESPACE
