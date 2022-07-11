// Copyright (c) 2022 YDrive Inc. All rights reserved.

#include "CameraRig/CameraRigYamlInterface.h"

#include "CineCameraComponent.h"
#include "DesktopPlatformModule.h"
#include "Misc/FileHelper.h"
#include "IDesktopPlatform.h"

#include "CameraRig/CameraRigData.h"
#include "CameraRig/CameraRigYamlParser.h"
#include "EasySynth.h"


#define LOCTEXT_NAMESPACE "FCameraRigYamlInterface"

FReply FCameraRigYamlInterface::OnImportCameraRigClicked()
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
		TEXT("Camera Rig YAML (*.yaml)|*.yaml"),
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
		UE_LOG(LogEasySynth, Warning, TEXT("%s: Could not load the selected file"), *FString(__FUNCTION__))
		return FReply::Handled();
	}

	// Parse the file contents
	FCameraRigData CameraRigData;
	FCameraRigYamlParser CameraRigYamlParser;
	if (!CameraRigYamlParser.Parse(FileContent, CameraRigData))
	{
		const FText MessageBoxTitle = LOCTEXT("YamlParsingErrorMessageBoxTitle", "Could not parse the yaml file");
		UE_LOG(LogEasySynth, Warning, TEXT("%s: %s"), *FString(__FUNCTION__), *(MessageBoxTitle.ToString()))
		FMessageDialog::Open(
			EAppMsgType::Ok,
			FText::FromString(CameraRigYamlParser.GetErrorMessage()),
			&MessageBoxTitle);
		return FReply::Handled();
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
			*FString::Printf(TEXT("CameraComponent_%d"), i));
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

bool FCameraRigYamlInterface::ExportCameraRig(const FString& OutputDir, TArray<UCameraComponent*> RigCameras)
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

void FCameraRigYamlInterface::AddCamera(const int CameraId, UCameraComponent* Camera, TArray<FString>& OutLines)
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

void FCameraRigYamlInterface::AddMatrix(
	const FString& MatrixName,
	const int Rows,
	const int Cols,
	const TArray<double>& Values,
	TArray<FString>& OutLines)
{
	OutLines.Add(FYamlFileStructure::NameLine(MatrixName));
	OutLines.Add(FYamlFileStructure::RowsLine(Rows));
	OutLines.Add(FYamlFileStructure::ColsLine(Cols));
	OutLines.Add(FYamlFileStructure::DTLine());

	// Create the values line
	FString ValuesLine = FString::Printf(TEXT("%s %f"), *FYamlFileStructure::DataLineStart(), Values[0]);
	for (int i = 1; i < Values.Num(); i++)
	{
		ValuesLine = FString::Printf(TEXT("%s, %f"), *ValuesLine, Values[i]);
	}
	ValuesLine = FString::Printf(TEXT("%s %s"), *ValuesLine, *FYamlFileStructure::DataLineEnd());
	OutLines.Add(ValuesLine);
}

#undef LOCTEXT_NAMESPACE
