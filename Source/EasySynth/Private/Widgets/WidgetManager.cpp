// Copyright (c) YDrive Inc. All rights reserved.
// Licensed under the MIT License.

#include "Widgets/WidgetManager.h"

#include "LevelSequence.h"
#include "PropertyCustomizationHelpers.h"
#include "Widgets/Docking/SDockTab.h"
#include "Widgets/Input/SDirectoryPicker.h"
#include "Widgets/Input/SSpinBox.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Text/STextBlock.h"

#include "Widgets/WidgetStateAsset.h"


const FString FWidgetManager::TextureStyleColorName(TEXT("Original color textures"));
const FString FWidgetManager::TextureStyleSemanticName(TEXT("Semantic color textures"));
const FIntPoint FWidgetManager::DefaultOutputImageResolution(1920, 1080);

const FText FWidgetManager::StartRenderingErrorMessageBoxTitle = FText::FromString(TEXT("Could not start rendering"));
const FText FWidgetManager::RenderingErrorMessageBoxTitle = FText::FromString(TEXT("Rendering failed"));
const FText FWidgetManager::SuccessfulRenderingMessageBoxTitle = FText::FromString(TEXT("Successful rendering"));

FWidgetManager::FWidgetManager() :
	OutputImageResolution(DefaultOutputImageResolution),
	OutputDirectory(FPathUtils::DefaultRenderingOutputPath())
{
	// Create the texture style manager and add it to the root to avoid garbage collection
	TextureStyleManager = NewObject<UTextureStyleManager>();
	check(TextureStyleManager);
	TextureStyleManager->AddToRoot();
	// Add some dummy sematic classes
	TextureStyleManager->NewSemanticClass(TEXT("Drivable"), FColor(255, 0, 0, 255), false);
	TextureStyleManager->NewSemanticClass(TEXT("Marking"), FColor(0, 255, 0, 255), false);
	TextureStyleManager->NewSemanticClass(TEXT("Sidewalk"), FColor(0, 0, 255, 255), false);

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

	// Initialize SemanticClassesWidgetManager
	SemanticsWidget.SetTextureStyleManager(TextureStyleManager);
}

TSharedRef<SDockTab> FWidgetManager::OnSpawnPluginTab(const FSpawnTabArgs& SpawnTabArgs)
{
	// Bind events now that the editor has finished starting up
	TextureStyleManager->BindEvents();

	// Load saved optsion states now, also to make sure editor is ready
	LoadWidgetOptionStates();

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
				.OnClicked_Raw(&SemanticsWidget, &FSemanticClassesWidgetManager::OnManageSemanticClassesClicked)
				.Content()
				[
					SNew(STextBlock)
					.Text(FText::FromString("Manage Semantic Classes"))
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
				.OnComboBoxOpening_Raw(this, &FWidgetManager::OnSemanticClassComboBoxOpened)
				.Content()
				[
					SNew(STextBlock).Text(FText::FromString(TEXT("Pick a semantic class")))
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
					SNew(STextBlock).Text(FText::FromString(TEXT("Pick a mesh texture style")))
				]
			]
			+SScrollBox::Slot()
			.Padding(2)
			[
				SNew(STextBlock)
				.Text(FText::FromString("Pick sequencer"))
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
				.Text(FText::FromString("Chose targets to be rendered"))
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
					.Text(FText::FromString("Camera poses"))
				]
			]
			+SScrollBox::Slot()
			.Padding(2)
			[
				SNew(SCheckBox)
				.IsChecked_Raw(this, &FWidgetManager::RenderTargetsCheckedState, FRendererTargetOptions::COLOR_IMAGE)
				.OnCheckStateChanged_Raw(
					this, &FWidgetManager::OnRenderTargetsChanged, FRendererTargetOptions::COLOR_IMAGE)
				[
					SNew(STextBlock)
					.Text(FText::FromString("Color images"))
				]
			]
			+SScrollBox::Slot()
			.Padding(2)
			[
				SNew(SCheckBox)
				.IsChecked_Raw(this, &FWidgetManager::RenderTargetsCheckedState, FRendererTargetOptions::DEPTH_IMAGE)
				.OnCheckStateChanged_Raw(
					this, &FWidgetManager::OnRenderTargetsChanged, FRendererTargetOptions::DEPTH_IMAGE)
				[
					SNew(STextBlock)
					.Text(FText::FromString("Depth images"))
				]
			]
			+SScrollBox::Slot()
			.Padding(2)
			[
				SNew(SCheckBox)
				.IsChecked_Raw(this, &FWidgetManager::RenderTargetsCheckedState, FRendererTargetOptions::NORMAL_IMAGE)
				.OnCheckStateChanged_Raw(
					this, &FWidgetManager::OnRenderTargetsChanged, FRendererTargetOptions::NORMAL_IMAGE)
				[
					SNew(STextBlock)
					.Text(FText::FromString("Normal images"))
				]
			]
			+SScrollBox::Slot()
			.Padding(2)
			[
				SNew(SCheckBox)
				.IsChecked_Raw(this, &FWidgetManager::RenderTargetsCheckedState, FRendererTargetOptions::SEMANTIC_IMAGE)
				.OnCheckStateChanged_Raw(
					this, &FWidgetManager::OnRenderTargetsChanged, FRendererTargetOptions::SEMANTIC_IMAGE)
				[
					SNew(STextBlock)
					.Text(FText::FromString("Semantic images"))
				]
			]
			+SScrollBox::Slot()
			.Padding(2)
			[
				SNew(STextBlock)
				.Text(FText::FromString("Output image width [px]"))
			]
			+SScrollBox::Slot()
			.Padding(2)
			[
				SNew(SSpinBox<int32>)
				.Value_Lambda([this](){ return OutputImageResolution.X; })
				.OnValueChanged_Lambda([this](const int32 NewValue){ OutputImageResolution.X = NewValue / 2 * 2; })
				.MinValue(1)
				.MaxValue(10000)
			]
			+SScrollBox::Slot()
			.Padding(2)
			[
				SNew(STextBlock)
				.Text(FText::FromString("Output image height [px]"))
			]
			+SScrollBox::Slot()
			.Padding(2)
			[
				SNew(SSpinBox<int32>)
				.Value_Lambda([this](){ return OutputImageResolution.Y; })
				.OnValueChanged_Lambda([this](const int32 NewValue){ OutputImageResolution.Y = NewValue / 2 * 2; })
				.MinValue(1)
				.MaxValue(10000)
			]
			// TODO: Displat aspect ratio
			+SScrollBox::Slot()
			.Padding(2)
			[
				SNew(STextBlock)
				.Text(FText::FromString("Depth range [m]"))
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
				.Text(FText::FromString("Ouput directory"))
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
					.Text(FText::FromString("Render Images"))
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
	}
}

void FWidgetManager::OnSemanticClassComboBoxOpened()
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
		FMessageDialog::Open(
			EAppMsgType::Ok,
			FText::FromString(*SequenceRenderer->GetErrorMessage()),
			&StartRenderingErrorMessageBoxTitle);
	}

	// Save the current widget options
	SaveWidgetOptionStates();

	return FReply::Handled();
}

void FWidgetManager::OnRenderingFinished(bool bSuccess)
{
	if (bSuccess)
	{
		FMessageDialog::Open(
			EAppMsgType::Ok,
			FText::FromString(TEXT("Rendering finished successfully")),
			&SuccessfulRenderingMessageBoxTitle);
	}
	else
	{
		FMessageDialog::Open(
			EAppMsgType::Ok,
			FText::FromString(*SequenceRenderer->GetErrorMessage()),
			&RenderingErrorMessageBoxTitle);
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

		// Register the plugin directroy with the editor
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
	SequenceRendererTargets.SetSelectedTarget(FRendererTargetOptions::SEMANTIC_IMAGE, WidgetStateAsset->bSematicImagesSelected);
	OutputImageResolution = WidgetStateAsset->OutputImageResolution;
	SequenceRendererTargets.SetDepthRangeMeters(WidgetStateAsset->DepthRange);
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
	WidgetStateAsset->bSematicImagesSelected = SequenceRendererTargets.TargetSelected(FRendererTargetOptions::SEMANTIC_IMAGE);
	WidgetStateAsset->OutputImageResolution = OutputImageResolution;
	WidgetStateAsset->DepthRange = SequenceRendererTargets.DepthRangeMeters();
	WidgetStateAsset->OutputDirectory = OutputDirectory;

	// Save the asset
	const bool bOnlyIfIsDirty = false;
	UEditorAssetLibrary::SaveLoadedAsset(WidgetStateAsset, bOnlyIfIsDirty);
}
