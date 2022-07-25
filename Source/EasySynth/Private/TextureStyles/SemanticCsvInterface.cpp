// Copyright (c) 2022 YDrive Inc. All rights reserved.

#include "TextureStyles/SemanticCsvInterface.h"

#include "Serialization/Csv/CsvParser.h"

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

	// Check if user is sure he wants semantic classes to be overriden
	if (TextureStyleManager->SemanticClasses().Num() > 1)
	{
		const FText MessageBoxTitle = LOCTEXT("OverrideConfirmationMessageBoxTitle", "Importing semantic classes");
		const FText MessageBoxMessage = LOCTEXT("OverrideConfirmationMessageBoxMessage", "Are you sure that you want to override existing semantic classes?");
		if (FMessageDialog::Open(EAppMsgType::YesNo, MessageBoxMessage, &MessageBoxTitle) == EAppReturnType::No)
		{
			return FReply::Handled();
		}
	}

	TextureStyleManager->RemoveAllSemanticCLasses();

	// Parse the file contents
	const FCsvParser CsvParser(FileContent);
	const FCsvParser::FRows& Rows = CsvParser.GetRows();

	for (int i = 0; i < Rows.Num(); i++)
	{
		const TArray<const TCHAR*>& Row = Rows[i];

		if (Row.Num() != 4)
		{
			const FText MessageBoxTitle = LOCTEXT("InvalidCsvMessageBoxTitle", "Failed to load CSV");
			FMessageDialog::Open(
				EAppMsgType::Ok,
				LOCTEXT("InvalidCsvMessageBoxText", "Expected line format \"name, R, G, B\""),
				&MessageBoxTitle);
			return FReply::Handled();
		}
		UE_LOG(LogEasySynth, Warning, TEXT("%s: %s"), *FString(__FUNCTION__), Row[0])

		const bool bSaveTextureMappingAsset = (i == Rows.Num() - 1);
		TextureStyleManager->NewSemanticClass(Row[0],
			FColor(FCString::Atoi(Row[1]), FCString::Atoi(Row[2]), FCString::Atoi(Row[3])),
			bSaveTextureMappingAsset);
	}

	return FReply::Handled();
}

bool FSemanticCsvInterface::ExportSemanticClasses(const FString& OutputDir, UTextureMappingAsset* TextureMappingAsset)
{
	TArray<FString> Lines;

	for (auto Element : TextureMappingAsset->SemanticClasses)
	{
		const FSemanticClass& Class = Element.Value;
		Lines.Add(FString::Printf(TEXT("%s,%d,%d,%d"), *Class.Name, Class.Color.R, Class.Color.G, Class.Color.B));
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
