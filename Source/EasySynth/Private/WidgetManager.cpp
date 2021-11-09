// Copyright Ydrive 2021

#include "WidgetManager.h"

#include "LevelSequence.h"
#include "PropertyCustomizationHelpers.h"
#include "Widgets/Docking/SDockTab.h"
#include "Widgets/Input/SDirectoryPicker.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Text/STextBlock.h"


const FText FWidgetManager::StartRenderingErrorMessageBoxTitle = FText::FromString(TEXT("Could not start rendering"));

FWidgetManager::FWidgetManager()
{
	// Create the sequence renderer and add it to the root to avoid garbage collection
	SequenceRenderer = NewObject<USequenceRenderer>();
	check(SequenceRenderer)
	SequenceRenderer->AddToRoot();

	// Define the default output directory
	// TODO: remember the last one used
	OutputDirectory = FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("RenderingOutput"));
}

TSharedRef<SDockTab> FWidgetManager::OnSpawnPluginTab(const FSpawnTabArgs& SpawnTabArgs)
{
	return SNew(SDockTab)
		.TabRole(ETabRole::NomadTab)
		[
			SNew(SScrollBox)
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
				// TODO: Disable if settings are not valid
				.OnClicked_Raw(this, &FWidgetManager::OnRenderImagesClicked)
				.Content()
				[
					SNew(STextBlock)
					.Text(FText::FromString("Render Images"))
				]
			]
		];
}

void FWidgetManager::OnSequencerSelected(const FAssetData& AssetData)
{
	LevelSequenceAssetData = AssetData;
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

void FWidgetManager::OnOutputDirectoryChanged(const FString& Directory)
{
	OutputDirectory = Directory;
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
