// Copyright Ydrive 2021

#include "WidgetManager.h"

#include "LevelSequence.h"
#include "PropertyCustomizationHelpers.h"
#include "Widgets/Docking/SDockTab.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Text/STextBlock.h"


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
				SNew(SButton)
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

FReply FWidgetManager::OnRenderImagesClicked()
{
	UE_LOG(LogEasySynth, Log, TEXT("%s"), *FString(__FUNCTION__))
	ULevelSequence* LevelSequence = Cast<ULevelSequence>(LevelSequenceAssetData.GetAsset());
	SequenceRenderer.RenderSequence(LevelSequence);
	return FReply::Handled();
}
