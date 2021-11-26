// Copyright (c) YDrive Inc. All rights reserved.
// Licensed under the MIT License.

#pragma once

#include "CoreMinimal.h"

#include "SequenceRenderer.h"

#include "Widgets/SemanticClassesWidgetManager.h"

class ULevelSequence;

class UTextureStyleManager;
class UWidgetStateAsset;


/**
 * Class that manages main UI widget interatcion
*/
class FWidgetManager
{
public:
	FWidgetManager();

	/** Handles the UI tab creation when requested */
	TSharedRef<SDockTab> OnSpawnPluginTab(const FSpawnTabArgs& SpawnTabArgs);

private:
	/**
	 * Main plugin widget handlers
	*/

	/** Handles manage semantic classes button click */
	FReply OnManageSemanticClassesClicked();

	/** Callback function handling the choosing of the semantic class inside the combo box */
	void OnSemanticClassComboBoxSelectionChanged(TSharedPtr<FString> StringItem, ESelectInfo::Type SelectInfo);

	/** Callback function handling the opening of the combo box menu */
	void OnSemanticClassComboBoxOpened();

	/** Callback function handling the choosing of the texture style inside the combo box */
	void OnTextureStyleComboBoxSelectionChanged(TSharedPtr<FString> StringItem, ESelectInfo::Type SelectInfo);

	/** Callback funcion handling the update of the selected sequencer */
	void OnSequencerSelected(const FAssetData& AssetData) { LevelSequenceAssetData = AssetData; }

	/** Callback funcion providing the path to the selected sequencer asset */
	FString GetSequencerPath() const;

	/** Checks whether renderer terget check box should be checked */
	ECheckBoxState RenderTargetsCheckedState(const FRendererTargetOptions::TargetType TargetType) const;

	/** Target render images checkbox handling */
	void OnRenderTargetsChanged(ECheckBoxState NewState, const FRendererTargetOptions::TargetType TargetType);

	/** Callback function handling the update of the output directory */
	void OnOutputDirectoryChanged(const FString& Directory) { OutputDirectory = Directory; }

	/** Checks if render images button should be enabled */
	bool GetIsRenderImagesEnabled() const;

	/** Handles render images button click */
	FReply OnRenderImagesClicked();

	/** Handles the sequence renderer finished event */
	void OnRenderingFinished(bool bSuccess);

	/**
	 * Local members
	*/

	/** Load widget options states on startup */
	void LoadWidgetOptionStates();

	/** Save widget options states */
	void SaveWidgetOptionStates(UWidgetStateAsset* WidgetStateAsset = nullptr);

	/** Manager that handles semantic class widget */
	FSemanticClassesWidgetManager SemanticsWidget;

	/** FStrings semantic class names referenced by the combo box */
	TArray<TSharedPtr<FString>> SemanticClassNames;

	/** Semantic class combo box */
	TSharedPtr<SComboBox<TSharedPtr<FString>>> SemanticClassComboBox;

	/** FStrings texture style names referenced by the combo box */
	TArray<TSharedPtr<FString>> TextureStyleNames;

	/** Currently selected sequencer asset data */
	FAssetData LevelSequenceAssetData;

	/** Widget's copy of the chosen renderer targets set */
	FRendererTargetOptions SequenceRendererTargets;

	/** Output image width */
	int OutputImageWidth;

	/** Output image height */
	int OutputImageHeight;

	/** Currently selected output directory */
	FString OutputDirectory;

	/**
	 * Module that manages default color and semantic texture styles,
	 * must be added to the root to avoid garbage collection
	*/
	UTextureStyleManager* TextureStyleManager;

	/**
	 * Module that runs sequence rendering,
	 * must be added to the root to avoid garbage collection
	*/
	USequenceRenderer* SequenceRenderer;

	/** The name of the texture style representing original colors */
	static const FString TextureStyleColorName;

	/** The name of the texture style representing semantic colors */
	static const FString TextureStyleSemanticName;

	/** Default output image width */
	static const int DefaultOutputImageWidth;

	/** Default output image height */
	static const int DefaultOutputImageHeight;

	/** Error message box title for failed rendering start */
	static const FText StartRenderingErrorMessageBoxTitle;

	/** Error message box title for failure during the rendering */
	static const FText RenderingErrorMessageBoxTitle;

	/** Message box title for successful rendering */
	static const FText SuccessfulRenderingMessageBoxTitle;
};
