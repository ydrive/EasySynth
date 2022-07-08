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

	AActor* CameraRigActor = Cast<AActor>(
		GEditor->AddActor(
			GEditor->GetEditorWorldContext().World()->GetCurrentLevel(),
			AActor::StaticClass(),
			FTransform(
				FRotator::ZeroRotator,
				FVector(0, 0, 0),
				FVector(1, 1, 1))));
	CameraRigActor->SetActorLabel("CameraRigActor");

	USceneComponent* RootComponent = NewObject<USceneComponent>(
		CameraRigActor,
		USceneComponent::GetDefaultSceneRootVariableName(),
		RF_Transactional);
	CameraRigActor->SetRootComponent(RootComponent);
	CameraRigActor->AddInstanceComponent(RootComponent);
	RootComponent->RegisterComponent();

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
	}

	// Spawn the camera rig actor

    return FReply::Handled();
}

#undef LOCTEXT_NAMESPACE
