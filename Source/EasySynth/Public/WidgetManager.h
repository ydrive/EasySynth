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
	TSharedRef<SDockTab> OnSpawnPluginTab(const FSpawnTabArgs& SpawnTabArgs);

private:
	/**
	 * Sequence selection
	*/

	/** Callback funcion handling the update of the selected sequencer */
	void OnSequencerSelected(const FAssetData& AssetData);

	/** Callback funcion providing the path to the selected sequencer asset */
	FString GetSequencerPath() const;

	/**
	 * Target render images checkbox handling
	*/

	/** Color images checkbox changed handle */
	void OnRenderColorImagesChanged(ECheckBoxState NewState);

	/** Handles render images button click */
	FReply OnRenderImagesClicked();

	/** Currently selected sequencer asset data */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = "true"), Category = "Content")
	FAssetData LevelSequenceAssetData;

	/** Widget's copy of the renderer targets structure */
	FSequenceRendererTargets SequenceRendererTargets;

	/** Error message box title for failed rendering start */
	static const FText StartRenderingErrorMessageBoxTitle;

	/** Module that runs sequence rendering */
	FSequenceRenderer SequenceRenderer;
};
