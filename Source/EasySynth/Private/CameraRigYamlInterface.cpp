// Copyright (c) 2022 YDrive Inc. All rights reserved.

#include "CameraRigYamlInterface.h"

#include "DesktopPlatformModule.h"
#include "Misc/FileHelper.h"
#include "IDesktopPlatform.h"

#include "EasySynth.h"


FReply FCameraRigYamlInterface::OnImportCameraRigClicked()
{
    UE_LOG(LogEasySynth, Log, TEXT("%s"), *FString(__FUNCTION__))

    FString SelectedDir = "";
	void* ParentWindowPtr = FSlateApplication::Get().GetActiveTopLevelWindow()->GetNativeWindow()->GetOSWindowHandle();
	IDesktopPlatform* DesktopPlatform = FDesktopPlatformModule::Get();
	if (DesktopPlatform == nullptr)
	{
		UE_LOG(LogEasySynth, Error, TEXT("%s: Could not get the desktop platform"), *FString(__FUNCTION__))
		return FReply::Handled();
	}

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

	FString FileContent;
	if (!FFileHelper::LoadFileToString(FileContent, *OutFilenames[0]))
	{
		UE_LOG(LogEasySynth, Warning, TEXT("%s: Could not load the selected file"), *FString(__FUNCTION__))
		return FReply::Handled();
	}

	UE_LOG(LogEasySynth, Log, TEXT("%s: %s"), *FString(__FUNCTION__), *FileContent)

    return FReply::Handled();
}
