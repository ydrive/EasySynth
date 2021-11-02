// Copyright Ydrive 2021

#include "WidgetManager.h"

#include "LevelSequence.h"
#include "PropertyCustomizationHelpers.h"
#include "Widgets/Docking/SDockTab.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Text/STextBlock.h"


TSharedRef<SDockTab> UWidgetManager::OnSpawnPluginTab(const FSpawnTabArgs& SpawnTabArgs)
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
				.ObjectPath_Raw(this, &UWidgetManager::GetSequencerPath)
				.OnObjectChanged_Raw(this, &UWidgetManager::OnSequencerSelected)
				.AllowClear(true)
				.DisplayUseSelected(true)
				.DisplayBrowse(true)
			]
			+SScrollBox::Slot()
			[
				SNew(SButton)
				.OnClicked_Raw(this, &UWidgetManager::OnRenderImagesClicked)
				.Content()
				[
					SNew(STextBlock)
					.Text(FText::FromString("Render Images"))
				]
			]
		];
}

void UWidgetManager::OnSequencerSelected(const FAssetData& AssetData)
{
	LevelSequenceAssetData = AssetData;
}

FString UWidgetManager::GetSequencerPath() const
{
	if (LevelSequenceAssetData.IsValid())
	{
		return LevelSequenceAssetData.ObjectPath.ToString();
	}
	return "";
}

FReply UWidgetManager::OnRenderImagesClicked()
{
	UE_LOG(LogEasySynth, Log, TEXT("%s"), *FString(__FUNCTION__))
	return FReply::Handled();
}
