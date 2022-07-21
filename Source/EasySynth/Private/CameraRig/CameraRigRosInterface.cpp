// Copyright (c) 2022 YDrive Inc. All rights reserved.

#include "CameraRig/CameraRigRosInterface.h"

#include "CineCameraComponent.h"
#include "DesktopPlatformModule.h"
#include "Misc/FileHelper.h"
#include "IDesktopPlatform.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"

#include "CameraRig/CameraRigUtils.h"
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

	// Read the selected file
	FString FileContent;
	if (!FFileHelper::LoadFileToString(FileContent, *OutFilenames[0]))
	{
		// ...
		UE_LOG(LogEasySynth, Error, TEXT("%s: 1"), *FString(__FUNCTION__))
		return FReply::Handled();
	}

	// Parse the file contents
	TSharedPtr<FJsonObject> JsonObject;
	TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(FileContent);
	if (!FJsonSerializer::Deserialize(Reader, JsonObject) || !JsonObject.IsValid())
	{
		// ...
		UE_LOG(LogEasySynth, Error, TEXT("%s: 2"), *FString(__FUNCTION__))
		return FReply::Handled();
	}

	// Get the cameras object
	const FString CamerasField = "cameras";
	const TSharedPtr<FJsonObject>* CamerasJsonObject;
	if (!JsonObject->TryGetObjectField(CamerasField, CamerasJsonObject))
	{
		// ...
		UE_LOG(LogEasySynth, Error, TEXT("%s: 3"), *FString(__FUNCTION__))
		return FReply::Handled();
	}

	// Aggregated camera rig data
	FCameraRigData CameraRigData;

	// Iterate through the array of objects and parse each camera
	TArray<FString> CameraNames;
	(*CamerasJsonObject)->Values.GetKeys(CameraNames);
	for (const FString& CameraName : CameraNames)
	{
		FCameraRigData::FCameraData CameraData;
		CameraData.CameraName = CameraName;

		const TSharedPtr<FJsonObject>* CameraJsonObject;
		if (!(*CamerasJsonObject)->TryGetObjectField(CameraName, CameraJsonObject))
		{
			// ...
			UE_LOG(LogEasySynth, Error, TEXT("%s: 4"), *FString(__FUNCTION__))
			return FReply::Handled();
		}

		// Get sensor size
		const FString SensorSizeField = "sensor_size";
		const TArray<TSharedPtr<FJsonValue>>* SensorSize;
		if (!(*CameraJsonObject)->TryGetArrayField(SensorSizeField, SensorSize))
		{
			// ...
			UE_LOG(LogEasySynth, Error, TEXT("%s: 5"), *FString(__FUNCTION__))
			return FReply::Handled();
		}
		if (SensorSize->Num() != 2)
		{
			// ...
			UE_LOG(LogEasySynth, Error, TEXT("%s: 5-"), *FString(__FUNCTION__))
			return FReply::Handled();
		}
		CameraData.SensorWidth = (*SensorSize)[0]->AsNumber();
		CameraData.SensorHeight = (*SensorSize)[1]->AsNumber();

		// Ignore the "camera" if its sensor size is 0
		if (CameraData.SensorWidth * CameraData.SensorHeight == 0)
		{
			continue;
		}

		// Get the coordinate system
		const FString CoordinateSystemField = "coord_sys";
		FString CoordinateSystem;
		if (!(*CameraJsonObject)->TryGetStringField(CoordinateSystemField, CoordinateSystem))
		{
			// ...
			UE_LOG(LogEasySynth, Error, TEXT("%s: 6"), *FString(__FUNCTION__))
			return FReply::Handled();
		}
		if (CoordinateSystem != "RDF")
		{
			// ...
			UE_LOG(LogEasySynth, Error, TEXT("%s: Expected RDF coordinate system, got %s"), *FString(__FUNCTION__), *CoordinateSystem)
			return FReply::Handled();
		}

		// Get focal length
		const FString CameraMatrixField = "calibrated_intrinsics";
		const TArray<TSharedPtr<FJsonValue>>* CameraMatrix;
		if (!(*CameraJsonObject)->TryGetArrayField(CameraMatrixField, CameraMatrix))
		{
			// ...
			UE_LOG(LogEasySynth, Error, TEXT("%s: 6"), *FString(__FUNCTION__))
			return FReply::Handled();
		}
		if (CameraMatrix->Num() != 9)
		{
			// ...
			UE_LOG(LogEasySynth, Error, TEXT("%s: 6-"), *FString(__FUNCTION__))
			return FReply::Handled();
		}
		CameraData.FocalLength = (*CameraMatrix)[0]->AsNumber();

		// Get rotation
		const FString RotationField = "rotation";
		const TArray<TSharedPtr<FJsonValue>>* Rotation;
		if (!(*CameraJsonObject)->TryGetArrayField(RotationField, Rotation))
		{
			// ...
			UE_LOG(LogEasySynth, Error, TEXT("%s: 7"), *FString(__FUNCTION__))
			return FReply::Handled();
		}
		if (Rotation->Num() != 3)
		{
			// ...
			UE_LOG(LogEasySynth, Error, TEXT("%s: 7-"), *FString(__FUNCTION__))
			return FReply::Handled();
		}
		const TArray<TSharedPtr<FJsonValue>> Rotation1 = (*Rotation)[0]->AsArray();
		const TArray<TSharedPtr<FJsonValue>> Rotation2 = (*Rotation)[1]->AsArray();
		const TArray<TSharedPtr<FJsonValue>> Rotation3 = (*Rotation)[2]->AsArray();
		FMatrix RotationMatrix = FMatrix(
			FVector(Rotation1[0]->AsNumber(), Rotation1[1]->AsNumber(), Rotation1[2]->AsNumber()),
			FVector(Rotation2[0]->AsNumber(), Rotation2[1]->AsNumber(), Rotation2[2]->AsNumber()),
			FVector(Rotation3[0]->AsNumber(), Rotation3[1]->AsNumber(), Rotation3[2]->AsNumber()),
			FVector(0.0, 0.0, 0.0));
		FQuat ExternalQuat = FQuat(RotationMatrix);
		TArray<double> ExternalRotation;
		ExternalRotation.Add(ExternalQuat.X);
		ExternalRotation.Add(ExternalQuat.Y);
		ExternalRotation.Add(ExternalQuat.Z);
		ExternalRotation.Add(ExternalQuat.W);

		// Get translation
		const FString TranslationField = "translation";
		const TArray<TSharedPtr<FJsonValue>>* Translation;
		if (!(*CameraJsonObject)->TryGetArrayField(TranslationField, Translation))
		{
			// ...
			UE_LOG(LogEasySynth, Error, TEXT("%s: 8"), *FString(__FUNCTION__))
			return FReply::Handled();
		}
		if (Translation->Num() != 3)
		{
			// ...
			UE_LOG(LogEasySynth, Error, TEXT("%s: 8-"), *FString(__FUNCTION__))
			return FReply::Handled();
		}
		TArray<double> ExternalTranslation;
		ExternalTranslation.Add((*Translation)[0]->AsNumber());
		ExternalTranslation.Add((*Translation)[1]->AsNumber());
		ExternalTranslation.Add((*Translation)[2]->AsNumber());

		// Apply needed transformations to the loaded translation and location
		const bool bDoInverse = false;
		CameraData.Transform = FCoordinateSystemConverter::ExternalToUE(
			ExternalTranslation,
			ExternalRotation,
			bDoInverse);

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
			*Camera.CameraName
			);
		CameraComponent->AttachToComponent(
			RootComponent,
			FAttachmentTransformRules::KeepWorldTransform);
		CameraRigActor->AddInstanceComponent(CameraComponent);
		CameraComponent->RegisterComponent();

		CameraComponent->SetRelativeTransform(Camera.Transform);

		// Make camera components smaller so that the rig is easier to visualize
		CameraComponent->SetRelativeScale3D(FVector(0.4f, 0.4f, 0.4f));
	}

    return FReply::Handled();
}

bool FCameraRigRosInterface::ExportCameraRig(const FString& OutputDir, TArray<UCameraComponent*> RigCameras)
{
	TArray<FString> Lines;
	Lines.Add("%YAML:1.0");
	Lines.Add("---");

	for (int i = 0; i < RigCameras.Num(); i++)
	{
		// Add each camera
		AddCamera(i, RigCameras[i], Lines);
	}

	// Save the file
	const FString SaveFilePath = FPathUtils::CameraRigFilePath(OutputDir);
	if (!FFileHelper::SaveStringArrayToFile(
		Lines,
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

void FCameraRigRosInterface::AddCamera(const int CameraId, UCameraComponent* Camera, TArray<FString>& OutLines)
{
	// Add camera matrix section
	TArray<double> CameraMatrixValues;
	CameraMatrixValues.Add(768.);
	CameraMatrixValues.Add(0.);
	CameraMatrixValues.Add(1.0201599731445312e+03);
	CameraMatrixValues.Add(0.);
	CameraMatrixValues.Add(768.);
    CameraMatrixValues.Add(6.4039001464843750e+02);
	CameraMatrixValues.Add(0.);
	CameraMatrixValues.Add(0.);
	CameraMatrixValues.Add(1);
	AddMatrix(FYamlFileStructure::CameraMatrixName(CameraId), 3, 3, CameraMatrixValues, OutLines);

	// Add distortion coefficients section
	TArray<double> DistortionCoeffValues;
	DistortionCoeffValues.Init(0, 14);
	AddMatrix(FYamlFileStructure::DistortionCoeffName(CameraId), 14, 1, DistortionCoeffValues, OutLines);

	// Covert between coordinate systems
	FTransform Transform = Camera->GetRelativeTransform();
	// Remove the scaling that makes no impact on camera functionality,
	// but my be used to scale the camera placeholder mesh as user desires
	Transform.SetScale3D(FVector(1.0f, 1.0f, 1.0f));
	TArray<double> TVecValues;
	TArray<double> QVecValues;
	const bool bDoInverse = true;
	FCoordinateSystemConverter::UEToExternal(Transform, TVecValues, QVecValues, bDoInverse);

	// Add t_vec and q_vec sections
	AddMatrix(FYamlFileStructure::TVecName(CameraId), 3, 1, TVecValues, OutLines);
	AddMatrix(FYamlFileStructure::QVecName(CameraId), 4, 1, QVecValues, OutLines);
}

#undef LOCTEXT_NAMESPACE
