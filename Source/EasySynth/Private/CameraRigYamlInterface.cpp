// Copyright (c) 2022 YDrive Inc. All rights reserved.

#include "CameraRigYamlInterface.h"

#include "EasySynth.h"


FReply FCameraRigYamlInterface::OnImportCameraRigClicked()
{
    UE_LOG(LogEasySynth, Log, TEXT("%s"), *FString(__FUNCTION__))

    // FString SelectedDir = "";
	// void* ParentWindowPtr = FSlateApplication::Get().GetActiveTopLevelWindow()->GetNativeWindow()->GetOSWindowHandle();
	// IDesktopPlatform* DesktopPlatform = FDesktopPlatformModule::Get();
	// if (DesktopPlatform)
	// {
	// 	// TODO: Default path
	// 	bool IsDirSelected = DesktopPlatform->OpenDirectoryDialog(
	// 		ParentWindowPtr,
	// 		"Import YDrive Workspace",
	// 		OpenWorkspacePath_,
	// 		SelectedDir);

	// 	if (IsDirSelected)
	// 	{
	// 		SelectedDir = FLoaderUtils::AddTailingSlash(SelectedDir);
	// 		const int ParentDirIndex =
	// 			SelectedDir.Find(TEXT("/"), ESearchCase::CaseSensitive, ESearchDir::FromEnd, SelectedDir.Len() - 2);
	// 		OpenWorkspacePath_ = SelectedDir.Mid(0, ParentDirIndex + 1);
	// 	}
	// }

	// if (SelectedDir.Len() == 0)
	// {
	// 	return;
	// }

    return FReply::Handled();
}
