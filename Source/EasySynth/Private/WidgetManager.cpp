// Copyright (c) YDrive Inc. All rights reserved.
// Licensed under the MIT License.

#include "WidgetManager.h"

#include "LevelSequence.h"
#include "PropertyCustomizationHelpers.h"
#include "Widgets/Docking/SDockTab.h"
#include "Widgets/Input/SDirectoryPicker.h"
#include "Widgets/Input/SSpinBox.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Text/STextBlock.h"


const FString FWidgetManager::TextureStyleColorName(TEXT("Original color textures"));
const FString FWidgetManager::TextureStyleSemanticName(TEXT("Semantic color textures"));

const FText FWidgetManager::StartRenderingErrorMessageBoxTitle = FText::FromString(TEXT("Could not start rendering"));
const FText FWidgetManager::RenderingErrorMessageBoxTitle = FText::FromString(TEXT("Rendering failed"));
const FText FWidgetManager::SuccessfulRenderingMessageBoxTitle = FText::FromString(TEXT("Successful rendering"));

FWidgetManager::FWidgetManager()
{
	// Create the texture style manager and add it to the root to avoid garbage collection
	// No need to ever release it, as the FWidgetManager lives as long as the plugin inside the editor
	TextureStyleManager = NewObject<UTextureStyleManager>();
	check(TextureStyleManager);
	TextureStyleManager->AddToRoot();
	// Add some dummy sematic classes
	TextureStyleManager->NewSemanticClass("Drivable", FColor(255, 0, 0, 255), false);
	TextureStyleManager->NewSemanticClass("Marking", FColor(0, 255, 0, 255), false);
	TextureStyleManager->NewSemanticClass("Sidewalk", FColor(0, 0, 255, 255), false);

	// Create the sequence renderer and add it to the root to avoid garbage collection
	// No need to ever release it, as the FWidgetManager lives as long as the plugin inside the editor
	SequenceRenderer = NewObject<USequenceRenderer>();
	check(SequenceRenderer)
	SequenceRenderer->AddToRoot();
	// Register the rendering finished callback
	SequenceRenderer->OnRenderingFinished().AddRaw(this, &FWidgetManager::OnRenderingFinished);
	SequenceRenderer->SetTextureStyleManager(TextureStyleManager);

	// Prepare content of the texture style checkout combo box
	TextureStyleNames.Add(MakeShared<FString>(TextureStyleColorName));
	TextureStyleNames.Add(MakeShared<FString>(TextureStyleSemanticName));

	// Define the default output directory
	// TODO: remember the last one used
	OutputDirectory = FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("RenderingOutput"));
}

TSharedRef<SDockTab> FWidgetManager::OnSpawnPluginTab(const FSpawnTabArgs& SpawnTabArgs)
{
	// Bind events now that the editor has finished starting up
	TextureStyleManager->BindEvents();

	return SNew(SDockTab)
		.TabRole(ETabRole::NomadTab)
		[
			SNew(SScrollBox)
			+SScrollBox::Slot()
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
			[
				SNew(STextBlock)
				.Text(FText::FromString("Pick sequencer"))
			]
			+SScrollBox::Slot()
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
			[
				SNew(STextBlock)
				.Text(FText::FromString("Chose targets to be rendered"))
			]
			+SScrollBox::Slot()
			[
				SNew(SCheckBox)
				.OnCheckStateChanged_Raw(
					this, &FWidgetManager::OnRenderTargetsChanged, FRendererTargetOptions::COLOR_IMAGE)
				[
					SNew(STextBlock)
					.Text(FText::FromString("Color images"))
				]
			]
			+SScrollBox::Slot()
			[
				SNew(SCheckBox)
				.OnCheckStateChanged_Raw(
					this, &FWidgetManager::OnRenderTargetsChanged, FRendererTargetOptions::DEPTH_IMAGE)
				[
					SNew(STextBlock)
					.Text(FText::FromString("Depth images"))
				]
			]
			+SScrollBox::Slot()
			[
				SNew(SCheckBox)
				.OnCheckStateChanged_Raw(
					this, &FWidgetManager::OnRenderTargetsChanged, FRendererTargetOptions::NORMAL_IMAGE)
				[
					SNew(STextBlock)
					.Text(FText::FromString("Normal images"))
				]
			]
			+SScrollBox::Slot()
			[
				SNew(SCheckBox)
				.OnCheckStateChanged_Raw(
					this, &FWidgetManager::OnRenderTargetsChanged, FRendererTargetOptions::SEMANTIC_IMAGE)
				[
					SNew(STextBlock)
					.Text(FText::FromString("Semantic images"))
				]
			]
			+SScrollBox::Slot()
			[
				SNew(STextBlock)
				.Text(FText::FromString("Depth range [m]"))
			]
			+SScrollBox::Slot()
			[
				SNew(SSpinBox<float>)
				.Value_Raw(this, &FWidgetManager::GetDepthRangeValue)
				.OnValueChanged_Raw(this, &FWidgetManager::OnDepthRangeValueChanged)
				.MinValue(0.01f)
				.MaxValue(10000.0f)
			]
			+SScrollBox::Slot()
			[
				SNew(STextBlock)
				.Text(FText::FromString("Ouput directory"))
			]
			+SScrollBox::Slot()
			[
				SNew(SDirectoryPicker)
				.Directory(OutputDirectory)
				.OnDirectoryChanged_Raw(this, &FWidgetManager::OnOutputDirectoryChanged)
			]
			+SScrollBox::Slot()
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
		TextureStyleManager->ApplySemanticClass(*StringItem);
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

void FWidgetManager::OnRenderTargetsChanged(ECheckBoxState NewState, FRendererTargetOptions::TargetType TargetType)
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
	if (!SequenceRenderer->RenderSequence(LevelSequence, SequenceRendererTargets, OutputDirectory))
	{
		FMessageDialog::Open(
			EAppMsgType::Ok,
			FText::FromString(*SequenceRenderer->GetErrorMessage()),
			&StartRenderingErrorMessageBoxTitle);
	}
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
