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
	~FWidgetManager();

	TSharedRef<SDockTab> OnSpawnPluginTab(const FSpawnTabArgs& SpawnTabArgs);

private:
	/** Callback funcion handling the update of the selected sequencer */
	void OnSequencerSelected(const FAssetData& AssetData);

	/** Callback funcion providing the path to the selected sequencer asset */
	FString GetSequencerPath() const;

	/** Target render images checkbox handling */
	void OnRenderTargetsChanged(ECheckBoxState NewState, USequenceRendererTargets::TargetType TargetType);

	/** Handles render images button click */
	FReply OnRenderImagesClicked();

	/** Currently selected sequencer asset data */
	FAssetData LevelSequenceAssetData;

	/** Widget's copy of the chosen renderer targets set */
	USequenceRendererTargets SequenceRendererTargets;

	/** Error message box title for failed rendering start */
	static const FText StartRenderingErrorMessageBoxTitle;

	/**
	 * Module that runs sequence rendering,
	 * must be added to the root to avoid garbage collection
	*/
	USequenceRenderer* SequenceRenderer;
};
