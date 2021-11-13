// Copyright Ydrive 2021

#pragma once

#include "CoreMinimal.h"

#include "SequenceRenderer.h"

class ULevelSequence;


/**
 * Class that manages UI widget interatcion
*/
class FWidgetManager
{
public:
	FWidgetManager();

	/** Handles the UI tab creation when requested */
	TSharedRef<SDockTab> OnSpawnPluginTab(const FSpawnTabArgs& SpawnTabArgs);

private:
	/** Callback funcion handling the update of the selected sequencer */
	void OnSequencerSelected(const FAssetData& AssetData);

	/** Callback funcion providing the path to the selected sequencer asset */
	FString GetSequencerPath() const;

	/** Target render images checkbox handling */
	void OnRenderTargetsChanged(ECheckBoxState NewState, FRendererTargetOptions::TargetType TargetType);

	/** Get the currently selected depth range value */
	float GetDepthRangeValue() const { return SequenceRendererTargets.DepthRangeMetersValue; }

	/** Callback function handling the update of the depth range value */
	void OnDepthRangeValueChanged(float NewValue) { SequenceRendererTargets.DepthRangeMetersValue = NewValue; }

	/** Callback function handling the update of the output directory */
	void OnOutputDirectoryChanged(const FString& Directory);

	/** Handles render images button click */
	FReply OnRenderImagesClicked();

	/** Handles the sequence renderer finished event */
	void OnRenderingFinished(bool bSuccess);

	/** Currently selected sequencer asset data */
	FAssetData LevelSequenceAssetData;

	/** Widget's copy of the chosen renderer targets set */
	FRendererTargetOptions SequenceRendererTargets;

	/** Currently selected output directory */
	FString OutputDirectory;

	/** Error message box title for failed rendering start */
	static const FText StartRenderingErrorMessageBoxTitle;

	/** Error message box title for failure during the rendering */
	static const FText RenderingErrorMessageBoxTitle;

	/** Message box title for successful rendering */
	static const FText SuccessfulRenderingMessageBoxTitle;

	/**
	 * Module that runs sequence rendering,
	 * must be added to the root to avoid garbage collection
	*/
	USequenceRenderer* SequenceRenderer;
};
