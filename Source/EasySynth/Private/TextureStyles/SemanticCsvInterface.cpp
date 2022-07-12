// Copyright (c) 2022 YDrive Inc. All rights reserved.

#include "TextureStyles/SemanticCsvInterface.h"

#include "TextureStyles/TextureMappingAsset.h"
#include "TextureStyles/TextureStyleManager.h"


#define LOCTEXT_NAMESPACE "FSemanticCsvInterface"

FReply FSemanticCsvInterface::OnImportSemanticClassesClicked(UTextureStyleManager* TextureStyleManager)
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
		TEXT("Import semantic classes"),
		TEXT(""),
		TEXT(""),
		TEXT("Semantic Classes CSV (*.csv)|*.csv"),
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


    return FReply::Handled();
}

bool FSemanticCsvInterface::ExportSemanticClasses(const FString& OutputDir, UTextureMappingAsset* TextureMappingAsset)
{
	TArray<FString> Lines;

	for (auto Iter : TextureMappingAsset->SemanticClasses)
	{
		const FSemanticClass& Class = Iter.Value;
		Lines.Add(FString::Printf(TEXT("%s, %d, %d, %d"), *Class.Name, Class.Color.R, Class.Color.G, Class.Color.B));
	}

	// Save the file
	const FString SaveFilePath = FPathUtils::SemanticClassesFilePath(OutputDir);
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

#undef LOCTEXT_NAMESPACE
