// Copyright (c) 2022 YDrive Inc. All rights reserved.

#pragma once

#include "CoreMinimal.h"

#include "SequenceRenderer.h"

#include "CameraRig/CameraRigRosInterface.h"
#include "TextureStyles/SemanticCsvInterface.h"
#include "Widgets/SemanticClassesWidgetManager.h"

class ULevelSequence;

class UTextureStyleManager;
class UWidgetStateAsset;


/**
 * Class that manages main UI widget interaction
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

	/** Callback function handling the choosing of the texture style inside the combo box */
	void OnTextureStyleComboBoxSelectionChanged(TSharedPtr<FString> StringItem, ESelectInfo::Type SelectInfo);

	/** Callback function handling the update of the selected sequencer */
	void OnSequencerSelected(const FAssetData& AssetData) { LevelSequenceAssetData = AssetData; }

	/** Callback function providing the path to the selected sequencer asset */
	FString GetSequencerPath() const;

	/** Checks whether renderer target check box should be checked */
	ECheckBoxState RenderTargetsCheckedState(const FRendererTargetOptions::TargetType TargetType) const;

	/** Target render images checkbox handling */
	void OnRenderTargetsChanged(ECheckBoxState NewState, const FRendererTargetOptions::TargetType TargetType);

	// Callback function handling the choosing of the output format inside the combo box
	void OnOutputFormatSelectionChanged(
		TSharedPtr<FString> StringItem,
		ESelectInfo::Type SelectInfo,
		const FRendererTargetOptions::TargetType TargetType);

	/** Returns the selected output format of the target */
	FText SelectedOutputFormat(const FRendererTargetOptions::TargetType TargetType) const;

	/** Callback function handling the update of the output directory */
	void OnOutputDirectoryChanged(const FString& Directory) { OutputDirectory = Directory; }

	/** Checks if render images button should be enabled */
	bool GetIsRenderImagesEnabled() const;

	/** Handles render images button click */
	FReply OnRenderImagesClicked();

	/** Handles the semantic classes updated event */
	void OnSemanticClassesUpdated();

	/** Handles the sequence renderer finished event */
	void OnRenderingFinished(bool bSuccess);

	/**
	 * Local members
	*/

	/** Load widget options states on startup */
	void LoadWidgetOptionStates();

	/** Save widget options states */
	void SaveWidgetOptionStates(UWidgetStateAsset* WidgetStateAsset = nullptr);

	/** Interface that handles importing semantic classes from CSV */
	FSemanticCsvInterface SemanticCsvInterface;

	/** Interface that handles importing camera rigs from ROS JSON files */
	FCameraRigRosInterface CameraRigRosInterface;

	/** Manager that handles semantic class widget */
	FSemanticClassesWidgetManager SemanticsWidget;

	/** FStrings semantic class names referenced by the combo box */
	TArray<TSharedPtr<FString>> SemanticClassNames;

	/** Semantic class combo box */
	TSharedPtr<SComboBox<TSharedPtr<FString>>> SemanticClassComboBox;

	/** FStrings texture style names referenced by the combo box */
	TArray<TSharedPtr<FString>> TextureStyleNames;

	/** FStrings output image format names referenced by the combo box */
	TArray<TSharedPtr<FString>> OutputFormatNames;

	/** Currently selected sequencer asset data */
	FAssetData LevelSequenceAssetData;

	/** Widget's copy of the chosen renderer targets set */
	FRendererTargetOptions SequenceRendererTargets;

	/** Output image resolution, with the image size always being an even number */
	FIntPoint OutputImageResolution;

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

	/** The name of the JPEG output format */
	static const FString JpegFormatName;

	/** The name of the PNG output format */
	static const FString PngFormatName;

	/** The name of the EXR output format */
	static const FString ExrFormatName;

	/** Default output image resolution */
	static const FIntPoint DefaultOutputImageResolution;
};
