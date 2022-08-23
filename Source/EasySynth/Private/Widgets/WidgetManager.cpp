// Copyright (c) 2022 YDrive Inc. All rights reserved.

#include "Widgets/WidgetManager.h"

#include "LevelSequence.h"
#include "PropertyCustomizationHelpers.h"
#include "Widgets/Docking/SDockTab.h"
#include "Widgets/Input/SDirectoryPicker.h"
#include "Widgets/Input/SSpinBox.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Layout/SSeparator.h"
#include "Widgets/Text/STextBlock.h"

#include "Widgets/WidgetStateAsset.h"


const FString FWidgetManager::TextureStyleColorName(TEXT("Original color textures"));
const FString FWidgetManager::TextureStyleSemanticName(TEXT("Semantic color textures"));
const FString FWidgetManager::JpegFormatName(TEXT("jpeg"));
const FString FWidgetManager::PngFormatName(TEXT("png"));
const FString FWidgetManager::ExrFormatName(TEXT("exr"));
const FIntPoint FWidgetManager::DefaultOutputImageResolution(1920, 1080);

#define LOCTEXT_NAMESPACE "FWidgetManager"

FWidgetManager::FWidgetManager() :
	OutputImageResolution(DefaultOutputImageResolution),
	OutputDirectory(FPathUtils::DefaultRenderingOutputPath())
{
	// Create the texture style manager and add it to the root to avoid garbage collection
	TextureStyleManager = NewObject<UTextureStyleManager>();
	check(TextureStyleManager);
	TextureStyleManager->AddToRoot();
	// Register the semantic classes updated callback
	TextureStyleManager->OnSemanticClassesUpdated().AddRaw(this, &FWidgetManager::OnSemanticClassesUpdated);

	// Create the sequence renderer and add it to the root to avoid garbage collection
	SequenceRenderer = NewObject<USequenceRenderer>();
	check(SequenceRenderer)
	SequenceRenderer->AddToRoot();
	// Register the rendering finished callback
	SequenceRenderer->OnRenderingFinished().AddRaw(this, &FWidgetManager::OnRenderingFinished);
	SequenceRenderer->SetTextureStyleManager(TextureStyleManager);

	// No need to ever release the TextureStyleManager and the SequenceRenderer,
	// as the FWidgetManager lives as long as the plugin inside the editor

	// Prepare content of the texture style checkout combo box
	TextureStyleNames.Add(MakeShared<FString>(TextureStyleColorName));
	TextureStyleNames.Add(MakeShared<FString>(TextureStyleSemanticName));

	// Prepare content of the outut image format combo box
	OutputFormatNames.Add(MakeShared<FString>(JpegFormatName));
	OutputFormatNames.Add(MakeShared<FString>(PngFormatName));
	OutputFormatNames.Add(MakeShared<FString>(ExrFormatName));

	// Initialize SemanticClassesWidgetManager
	SemanticsWidget.SetTextureStyleManager(TextureStyleManager);
}

TSharedRef<SDockTab> FWidgetManager::OnSpawnPluginTab(const FSpawnTabArgs& SpawnTabArgs)
{
	// Bind events now that the editor has finished starting up
	TextureStyleManager->BindEvents();

	// Load saved option states now, also to make sure editor is ready
	LoadWidgetOptionStates();

	// Update combo box semantic class names
	OnSemanticClassesUpdated();

	// Dynamically generate renderer target checkboxes
	TSharedRef<SScrollBox> TargetsScrollBoxes = SNew(SScrollBox);
	TMap<FRendererTargetOptions::TargetType, FText> TargetCheckBoxNames;
	TargetCheckBoxNames.Add(FRendererTargetOptions::COLOR_IMAGE, LOCTEXT("ColorImagesCheckBoxText", "Color images"));
	TargetCheckBoxNames.Add(FRendererTargetOptions::DEPTH_IMAGE, LOCTEXT("DepthImagesCheckBoxText", "Depth images"));
	TargetCheckBoxNames.Add(FRendererTargetOptions::NORMAL_IMAGE, LOCTEXT("NormalImagesCheckBoxText", "Normal images"));
	TargetCheckBoxNames.Add(FRendererTargetOptions::OPTICAL_FLOW_IMAGE, LOCTEXT("OpticalFlowImagesCheckBoxText", "Optical flow images"));
	TargetCheckBoxNames.Add(FRendererTargetOptions::SEMANTIC_IMAGE, LOCTEXT("SemanticImagesCheckBoxText", "Semantic images"));
	for (auto Element : TargetCheckBoxNames)
	{
		const FRendererTargetOptions::TargetType TargetType = Element.Key;
		const FText CheckBoxText = Element.Value;
		TargetsScrollBoxes->AddSlot()
			.Padding(2)
			[
				SNew(SHorizontalBox)
				+SHorizontalBox::Slot()
				[
					SNew(SCheckBox)
					.IsChecked_Raw(this, &FWidgetManager::RenderTargetsCheckedState, TargetType)
					.OnCheckStateChanged_Raw(this, &FWidgetManager::OnRenderTargetsChanged, TargetType)
					[
						SNew(STextBlock)
						.Text(CheckBoxText)
					]
				]
				+SHorizontalBox::Slot()
				[
					SNew(SComboBox<TSharedPtr<FString>>)
					.OptionsSource(&OutputFormatNames)
					.ContentPadding(2)
					.OnGenerateWidget_Lambda(
						[](TSharedPtr<FString> StringItem)
						{ return SNew(STextBlock).Text(FText::FromString(*StringItem)); })
					.OnSelectionChanged_Raw(this, &FWidgetManager::OnOutputFormatSelectionChanged, TargetType)
					[
						SNew(STextBlock)
						.Text_Raw(this, &FWidgetManager::SelectedOutputFormat, TargetType)
					]
				]
			];
	}

	// Generate the UI
	return SNew(SDockTab)
		.TabRole(ETabRole::NomadTab)
		.ContentPadding(2)
		[
			SNew(SScrollBox)
			+SScrollBox::Slot()
			.Padding(2)
			[
				SNew(SButton)
				.OnClicked_Raw(
					&SemanticCsvInterface,
					&FSemanticCsvInterface::OnImportSemanticClassesClicked,
					TextureStyleManager)
				.Content()
				[
					SNew(STextBlock)
					.Text(LOCTEXT("ImportSemanticClassesButtonText", "Import semantic classes CSV file"))
				]
			]
			+SScrollBox::Slot()
			.Padding(2)
			[
				SNew(SButton)
				.OnClicked_Raw(&CameraRigRosInterface, &FCameraRigRosInterface::OnImportCameraRigClicked)
				.Content()
				[
					SNew(STextBlock)
					.Text(LOCTEXT("ImportCameraRigButtonText", "Import camera rig ROS JSON file"))
				]
			]
			+SScrollBox::Slot()
			.Padding(0, 2, 0, 2)
			[
				SNew(SSeparator)
			]
			+SScrollBox::Slot()
			.Padding(2)
			[
				SNew(SButton)
				.OnClicked_Raw(&SemanticsWidget, &FSemanticClassesWidgetManager::OnManageSemanticClassesClicked)
				.Content()
				[
					SNew(STextBlock)
					.Text(LOCTEXT("ManageSemanticClassesButtonText", "Manage Semantic Classes"))
				]
			]
			+SScrollBox::Slot()
			.Padding(2)
			[
				SAssignNew(SemanticClassComboBox, SComboBox<TSharedPtr<FString>>)
				.OptionsSource(&SemanticClassNames)
				.ContentPadding(2)
				.OnGenerateWidget_Lambda(
					[](TSharedPtr<FString> StringItem)
					{ return SNew(STextBlock).Text(FText::FromString(*StringItem)); })
				.OnSelectionChanged_Raw(this, &FWidgetManager::OnSemanticClassComboBoxSelectionChanged)
				.Content()
				[
					SNew(STextBlock)
					.Text(LOCTEXT("PickSemanticClassComboBoxText", "Pick a semantic class"))
				]
			]
			+SScrollBox::Slot()
			.Padding(2)
			[
				SNew(SComboBox<TSharedPtr<FString>>)
				.OptionsSource(&TextureStyleNames)
				.ContentPadding(2)
				.OnGenerateWidget_Lambda(
					[](TSharedPtr<FString> StringItem)
					{ return SNew(STextBlock).Text(FText::FromString(*StringItem)); })
				.OnSelectionChanged_Raw(this, &FWidgetManager::OnTextureStyleComboBoxSelectionChanged)
				.Content()
				[
					SNew(STextBlock)
					.Text(LOCTEXT("PickMeshTextureStyleComboBoxText", "Pick a mesh texture style"))
				]
			]
			+SScrollBox::Slot()
			.Padding(2)
			[
				SNew(STextBlock)
				.Text(LOCTEXT("PickSequencerSectionTitle", "Pick sequencer"))
			]
			+SScrollBox::Slot()
			.Padding(2)
			[
				SNew(SObjectPropertyEntryBox)
				.AllowedClass(ULevelSequence::StaticClass())
				.ObjectPath_Raw(this, &FWidgetManager::GetSequencerPath)
				.OnObjectChanged_Raw(this, &FWidgetManager::OnSequencerSelected)
				.AllowClear(true)
				.DisplayUseSelected(true)
				.DisplayBrowse(true)
			]
			+SScrollBox::Slot()
			.Padding(2)
			[
				SNew(STextBlock)
				.Text(LOCTEXT("ChoseTargetsSectionTitle", "Chose targets to be rendered"))
			]
			+SScrollBox::Slot()
			.Padding(2)
			[
				SNew(SCheckBox)
				.IsChecked_Lambda(
					[this]()
					{
						const bool bChecked = SequenceRendererTargets.ExportCameraPoses();
						return bChecked ? ECheckBoxState::Checked : ECheckBoxState::Unchecked;
					})
				.OnCheckStateChanged_Lambda(
					[this](ECheckBoxState NewState)
					{ SequenceRendererTargets.SetExportCameraPoses(NewState == ECheckBoxState::Checked); })
				[
					SNew(STextBlock)
					.Text(LOCTEXT("CameraPosesCheckBoxText", "Camera poses"))
				]
			]
			+SScrollBox::Slot()
			[
				TargetsScrollBoxes
			]
			+SScrollBox::Slot()
			.Padding(2)
			[
				SNew(STextBlock)
				.Text(LOCTEXT("OutputWidthText", "Output image width [px]"))
			]
			+SScrollBox::Slot()
			.Padding(2)
			[
				SNew(SSpinBox<int32>)
				.Value_Lambda([this](){ return OutputImageResolution.X; })
				.OnValueChanged_Lambda([this](const int32 NewValue){ OutputImageResolution.X = NewValue / 2 * 2; })
				.MinValue(100)
				.MaxValue(1920 * 2)
			]
			+SScrollBox::Slot()
			.Padding(2)
			[
				SNew(STextBlock)
				.Text(LOCTEXT("OutputHeightText", "Output image height [px]"))
			]
			+SScrollBox::Slot()
			.Padding(2)
			[
				SNew(SSpinBox<int32>)
				.Value_Lambda([this](){ return OutputImageResolution.Y; })
				.OnValueChanged_Lambda([this](const int32 NewValue){ OutputImageResolution.Y = NewValue / 2 * 2; })
				.MinValue(100)
				.MaxValue(1080 * 2)
			]
			+SScrollBox::Slot()
			.Padding(2)
			[
				SNew(STextBlock)
				.Text_Lambda(
					[this]()
					{
						return FText::Format(
							LOCTEXT("OutputAspectRatioText", "Output aspect ratio: {0}"),
							FText::AsNumber(1.0f * OutputImageResolution.X / OutputImageResolution.Y));
					})
			]
			+SScrollBox::Slot()
			.Padding(2)
			[
				SNew(STextBlock)
				.Text(LOCTEXT("DepthRangeText", "Depth range [m]"))
			]
			+SScrollBox::Slot()
			.Padding(2)
			[
				SNew(SSpinBox<float>)
				.Value_Lambda([this](){ return SequenceRendererTargets.DepthRangeMeters(); })
				.OnValueChanged_Lambda(
					[this](const float NewValue){ SequenceRendererTargets.SetDepthRangeMeters(NewValue); })
				.MinValue(0.01f)
				.MaxValue(10000.0f)
			]
			+SScrollBox::Slot()
			.Padding(2)
			[
				SNew(STextBlock)
				.Text(LOCTEXT("OpticalFlowScaleText", "Optical flow scale coefficient"))
			]
			+SScrollBox::Slot()
			.Padding(2)
			[
				SNew(SSpinBox<float>)
				.Value_Lambda([this](){ return SequenceRendererTargets.OpticalFlowScale(); })
				.OnValueChanged_Lambda(
					[this](const float NewValue){ SequenceRendererTargets.SetOpticalFlowScale(NewValue); })
				.MinValue(1.0f)
				.MaxValue(100.0f)
			]
			+SScrollBox::Slot()
			.Padding(2)
			[
				SNew(STextBlock)
				.Text(LOCTEXT("OuputDirectoryText", "Ouput directory"))
			]
			+SScrollBox::Slot()
			.Padding(2)
			[
				SNew(SDirectoryPicker)
				.Directory(OutputDirectory)
				.OnDirectoryChanged_Raw(this, &FWidgetManager::OnOutputDirectoryChanged)
			]
			+SScrollBox::Slot()
			.Padding(2)
			[
				SNew(SButton)
				.IsEnabled_Raw(this, &FWidgetManager::GetIsRenderImagesEnabled)
				.OnClicked_Raw(this, &FWidgetManager::OnRenderImagesClicked)
				.Content()
				[
					SNew(STextBlock)
					.Text(LOCTEXT("RenderImagesButtonText", "Render Images"))
				]
			]
		];
}

void FWidgetManager::OnSemanticClassComboBoxSelectionChanged(
	TSharedPtr<FString> StringItem,
	ESelectInfo::Type SelectInfo)
{
	if (StringItem.IsValid())
	{
		UE_LOG(LogEasySynth, Log, TEXT("%s: Semantic class selected: %s"), *FString(__FUNCTION__), **StringItem)
		TextureStyleManager->ApplySemanticClassToSelectedActors(*StringItem);
		SemanticClassComboBox->ClearSelection();
	}
}

void FWidgetManager::OnTextureStyleComboBoxSelectionChanged(
	TSharedPtr<FString> StringItem,
	ESelectInfo::Type SelectInfo)
{
	if (StringItem.IsValid())
	{
		UE_LOG(LogEasySynth, Log, TEXT("%s: Texture style selected: %s"), *FString(__FUNCTION__), **StringItem)
		if (*StringItem == TextureStyleColorName)
		{
			TextureStyleManager->CheckoutTextureStyle(ETextureStyle::COLOR);
		}
		else if (*StringItem == TextureStyleSemanticName)
		{
			TextureStyleManager->CheckoutTextureStyle(ETextureStyle::SEMANTIC);
		}
		else
		{
			UE_LOG(LogEasySynth, Error, TEXT("%s: Got unexpected texture style: %s"),
				*FString(__FUNCTION__), **StringItem);
		}
	}
}

FString FWidgetManager::GetSequencerPath() const
{
	if (LevelSequenceAssetData.IsValid())
	{
		return LevelSequenceAssetData.ObjectPath.ToString();
	}
	return "";
}

ECheckBoxState FWidgetManager::RenderTargetsCheckedState(const FRendererTargetOptions::TargetType TargetType) const
{
	const bool bChecked = SequenceRendererTargets.TargetSelected(TargetType);
	return bChecked ? ECheckBoxState::Checked : ECheckBoxState::Unchecked;
}

void FWidgetManager::OnRenderTargetsChanged(
	ECheckBoxState NewState,
	const FRendererTargetOptions::TargetType TargetType)
{
	SequenceRendererTargets.SetSelectedTarget(TargetType, (NewState == ECheckBoxState::Checked));
}

void FWidgetManager::OnOutputFormatSelectionChanged(
	TSharedPtr<FString> StringItem,
	ESelectInfo::Type SelectInfo,
	const FRendererTargetOptions::TargetType TargetType)
{
	if (*StringItem == JpegFormatName)
	{
		SequenceRendererTargets.SetOutputFormat(TargetType, EImageFormat::JPEG);
	}
	else if (*StringItem == PngFormatName)
	{
		SequenceRendererTargets.SetOutputFormat(TargetType, EImageFormat::PNG);
	}
	else if (*StringItem == ExrFormatName)
	{
		SequenceRendererTargets.SetOutputFormat(TargetType, EImageFormat::EXR);
	}
	else
	{
		UE_LOG(LogEasySynth, Error, TEXT("%s: Invalid output format selection '%s'"),
			*FString(__FUNCTION__), **StringItem);
	}
}

FText FWidgetManager::SelectedOutputFormat(const FRendererTargetOptions::TargetType TargetType) const
{
	EImageFormat OutputFormat = SequenceRendererTargets.OutputFormat(TargetType);

	if (OutputFormat == EImageFormat::JPEG)
	{
		return FText::FromString(JpegFormatName);
	}
	else if (OutputFormat == EImageFormat::PNG)
	{
		return FText::FromString(PngFormatName);
	}
	else if (OutputFormat == EImageFormat::EXR)
	{
		return FText::FromString(ExrFormatName);
	}
	else
	{
		UE_LOG(LogEasySynth, Error, TEXT("%s: Invalid target type '%d'"),
			*FString(__FUNCTION__), TargetType);
		return FText::GetEmpty();
	}
}

bool FWidgetManager::GetIsRenderImagesEnabled() const
{
	return
		LevelSequenceAssetData.GetAsset() != nullptr &&
		SequenceRendererTargets.AnyOptionSelected() &&
		SequenceRenderer != nullptr && !SequenceRenderer->IsRendering();
}

FReply FWidgetManager::OnRenderImagesClicked()
{
	ULevelSequence* LevelSequence = Cast<ULevelSequence>(LevelSequenceAssetData.GetAsset());
	// Make a copy of the SequenceRendererTargets to avoid
	// them being changed through the UI during rendering
	if (!SequenceRenderer->RenderSequence(
		LevelSequence,
		SequenceRendererTargets,
		OutputImageResolution,
		OutputDirectory))
	{
		const FText MessageBoxTitle = LOCTEXT("StartRenderingErrorMessageBoxTitle", "Could not start rendering");
		FMessageDialog::Open(
			EAppMsgType::Ok,
			FText::FromString(SequenceRenderer->GetErrorMessage()),
			&MessageBoxTitle);
	}

	// Save the current widget options
	SaveWidgetOptionStates();

	return FReply::Handled();
}

void FWidgetManager::OnSemanticClassesUpdated()
{
	// Refresh the list of semantic classes
	SemanticClassNames.Reset();
	TArray<FString> ClassNames = TextureStyleManager->SemanticClassNames();
	for (const FString& ClassName : ClassNames)
	{
		SemanticClassNames.Add(MakeShared<FString>(ClassName));
	}

	// Refresh the combo box
	if (SemanticClassComboBox.IsValid())
	{
		SemanticClassComboBox->RefreshOptions();
	}
	else
	{
		UE_LOG(LogEasySynth, Error, TEXT("%s: Semantic class picker is invalid, could not refresh"),
			*FString(__FUNCTION__));
	}
}

void FWidgetManager::OnRenderingFinished(bool bSuccess)
{
	if (bSuccess)
	{
		const FText MessageBoxTitle = LOCTEXT("SuccessfulRenderingMessageBoxTitle", "Successful rendering");
		FMessageDialog::Open(
			EAppMsgType::Ok,
			LOCTEXT("SuccessfulRenderingMessageBoxText", "Rendering finished successfully"),
			&MessageBoxTitle);
	}
	else
	{
		const FText MessageBoxTitle = LOCTEXT("RenderingErrorMessageBoxTitle", "Rendering failed");
		FMessageDialog::Open(
			EAppMsgType::Ok,
			FText::FromString(SequenceRenderer->GetErrorMessage()),
			&MessageBoxTitle);
	}
}

void FWidgetManager::LoadWidgetOptionStates()
{
	// Try to load
	UWidgetStateAsset* WidgetStateAsset =
		LoadObject<UWidgetStateAsset>(nullptr, *FPathUtils::WidgetStateAssetPath());

	// Initialize with defaults if not found
	if (WidgetStateAsset == nullptr)
	{
		UE_LOG(LogEasySynth, Log, TEXT("%s: Texture mapping asset not found, creating a new one"),
			*FString(__FUNCTION__));

		// Register the plugin directory with the editor
		FAssetRegistryModule& AssetRegistryModule =
			FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
		AssetRegistryModule.Get().AddPath(FPathUtils::ProjectPluginContentDir());

		// Create and populate the asset
		UPackage *WidgetStatePackage = CreatePackage(*FPathUtils::WidgetStateAssetPath());
		check(WidgetStatePackage)
		WidgetStateAsset = NewObject<UWidgetStateAsset>(
			WidgetStatePackage,
			UWidgetStateAsset::StaticClass(),
			*FPathUtils::WidgetStateAssetName,
			EObjectFlags::RF_Public | EObjectFlags::RF_Standalone);
		check(WidgetStateAsset)

		// Set defaults and save
		SaveWidgetOptionStates(WidgetStateAsset);
	}

	// Initialize the widget members using loaded options
	LevelSequenceAssetData = FAssetData(WidgetStateAsset->LevelSequenceAssetPath.TryLoad());
	SequenceRendererTargets.SetExportCameraPoses(WidgetStateAsset->bCameraPosesSelected);
	SequenceRendererTargets.SetSelectedTarget(FRendererTargetOptions::COLOR_IMAGE, WidgetStateAsset->bColorImagesSelected);
	SequenceRendererTargets.SetSelectedTarget(FRendererTargetOptions::DEPTH_IMAGE, WidgetStateAsset->bDepthImagesSelected);
	SequenceRendererTargets.SetSelectedTarget(FRendererTargetOptions::NORMAL_IMAGE, WidgetStateAsset->bNormalImagesSelected);
	SequenceRendererTargets.SetSelectedTarget(FRendererTargetOptions::OPTICAL_FLOW_IMAGE, WidgetStateAsset->bOpticalFlowImagesSelected);
	SequenceRendererTargets.SetSelectedTarget(FRendererTargetOptions::SEMANTIC_IMAGE, WidgetStateAsset->bSemanticImagesSelected);
	SequenceRendererTargets.SetOutputFormat(
		FRendererTargetOptions::COLOR_IMAGE,
		static_cast<EImageFormat>(WidgetStateAsset->bColorImagesOutputFormat));
	SequenceRendererTargets.SetOutputFormat(
		FRendererTargetOptions::DEPTH_IMAGE,
		static_cast<EImageFormat>(WidgetStateAsset->bDepthImagesOutputFormat));
	SequenceRendererTargets.SetOutputFormat(
		FRendererTargetOptions::NORMAL_IMAGE,
		static_cast<EImageFormat>(WidgetStateAsset->bNormalImagesOutputFormat));
	SequenceRendererTargets.SetOutputFormat(
		FRendererTargetOptions::OPTICAL_FLOW_IMAGE,
		static_cast<EImageFormat>(WidgetStateAsset->bOpticalFlowImagesOutputFormat));
	SequenceRendererTargets.SetOutputFormat(
		FRendererTargetOptions::SEMANTIC_IMAGE,
		static_cast<EImageFormat>(WidgetStateAsset->bSemanticImagesOutputFormat));
	OutputImageResolution = WidgetStateAsset->OutputImageResolution;
	SequenceRendererTargets.SetDepthRangeMeters(WidgetStateAsset->DepthRange);
	SequenceRendererTargets.SetOpticalFlowScale(WidgetStateAsset->OpticalFlowScale);
	OutputDirectory = WidgetStateAsset->OutputDirectory;
}

void FWidgetManager::SaveWidgetOptionStates(UWidgetStateAsset* WidgetStateAsset)
{
	// Get the asset if not provided
	if (WidgetStateAsset == nullptr)
	{
		WidgetStateAsset = LoadObject<UWidgetStateAsset>(nullptr, *FPathUtils::WidgetStateAssetPath());
		if (WidgetStateAsset == nullptr)
		{
			UE_LOG(LogEasySynth, Error, TEXT("%s: Widget state asset expected but not found, cannot save the widget state"),
				*FString(__FUNCTION__));
			return;
		}
	}

	// Update asset values
	WidgetStateAsset->LevelSequenceAssetPath = LevelSequenceAssetData.ToSoftObjectPath();
	WidgetStateAsset->bCameraPosesSelected = SequenceRendererTargets.ExportCameraPoses();
	WidgetStateAsset->bColorImagesSelected = SequenceRendererTargets.TargetSelected(FRendererTargetOptions::COLOR_IMAGE);
	WidgetStateAsset->bDepthImagesSelected = SequenceRendererTargets.TargetSelected(FRendererTargetOptions::DEPTH_IMAGE);
	WidgetStateAsset->bNormalImagesSelected = SequenceRendererTargets.TargetSelected(FRendererTargetOptions::NORMAL_IMAGE);
	WidgetStateAsset->bOpticalFlowImagesSelected = SequenceRendererTargets.TargetSelected(FRendererTargetOptions::OPTICAL_FLOW_IMAGE);
	WidgetStateAsset->bSemanticImagesSelected = SequenceRendererTargets.TargetSelected(FRendererTargetOptions::SEMANTIC_IMAGE);
	WidgetStateAsset->bColorImagesOutputFormat = static_cast<int8>(
		SequenceRendererTargets.OutputFormat(FRendererTargetOptions::COLOR_IMAGE));
	WidgetStateAsset->bDepthImagesOutputFormat = static_cast<int8>(
		SequenceRendererTargets.OutputFormat(FRendererTargetOptions::DEPTH_IMAGE));
	WidgetStateAsset->bNormalImagesOutputFormat = static_cast<int8>(
		SequenceRendererTargets.OutputFormat(FRendererTargetOptions::NORMAL_IMAGE));
	WidgetStateAsset->bOpticalFlowImagesOutputFormat = static_cast<int8>(
		SequenceRendererTargets.OutputFormat(FRendererTargetOptions::OPTICAL_FLOW_IMAGE));
	WidgetStateAsset->bSemanticImagesOutputFormat = static_cast<int8>(
		SequenceRendererTargets.OutputFormat(FRendererTargetOptions::SEMANTIC_IMAGE));
	WidgetStateAsset->OutputImageResolution = OutputImageResolution;
	WidgetStateAsset->DepthRange = SequenceRendererTargets.DepthRangeMeters();
	WidgetStateAsset->OpticalFlowScale = SequenceRendererTargets.OpticalFlowScale();
	WidgetStateAsset->OutputDirectory = OutputDirectory;

	// Save the asset
	const bool bOnlyIfIsDirty = false;
	UEditorAssetLibrary::SaveLoadedAsset(WidgetStateAsset, bOnlyIfIsDirty);
}

#undef LOCTEXT_NAMESPACE
