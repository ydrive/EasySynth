// Copyright (c) 2022 YDrive Inc. All rights reserved.

#pragma once

#include "CoreMinimal.h"

class UTextureStyleManager;


/**
 * Class that manages the widget for manipulating semantic classes
 * The class contains numerous delegate handlers which do not allow
 * for some of the parameters to be referenced, only const
*/
class FSemanticClassesWidgetManager
{
public:
	FSemanticClassesWidgetManager();

	/** Initializes the needed semantic class manager */
	void SetTextureStyleManager(UTextureStyleManager* Value) { TextureStyleManager = Value; }

	/** Handles the window creation when requested */
	FReply OnManageSemanticClassesClicked();

private:
	/**
	 * Existing class operations
	*/

	/** Handles the user request to change a semantic class name */
	void OnClassNameChanged(const FText& NewText, ETextCommit::Type CommitType, const FString ClassName);

	/** Handles the user request to change a semantic class color */
	FReply OnUpdateClassColorClicked(
		const FGeometry& MyGeometry,
		const FPointerEvent& MouseEvent,
		const FString ClassName);

	/** Handles the new submitted color for an existing semantic class */
	void OnUpdateClassColorCommited(const FLinearColor NewLinearColor);

	/** Handles add new semantic class button click */
	FReply OnDeleteClassClicked(const FString ClassName);

	/**
	 * New class operations
	*/

	/** Displays the color picker and for the new class color */
	FReply OnNewClassColorClicked(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent);

	/** Handles the new submitted color for the new semantic class */
	void OnNewClassColorCommited(const FLinearColor NewLinearColor);

	/** Handles add new semantic class button click */
	FReply OnAddNewClassClicked();

	/**
	 * The rest of members
	*/

	/** Reopens the semantic color window after the color picker has been closed */
	void OnColorPickerWindowClosed(const TSharedRef<SWindow>& Window);

	/** Handles semantic class management done click */
	FReply OnDoneClicked();

	/** Refreshes content of the existing semantic colors array */
	void RefreshSemanticClasses();

	/** The widget window weak pointer */
	TWeakPtr<SWindow> WidgetWindow;

	/** References the box that shows the array of existing semantic classes */
	TWeakPtr<SVerticalBox> ClassesBox;

	/** Stores which existing semantic class is being edited */
	FString CurrentlyEditedClass;

	/** Timer handle for the window reopen timer */
	FTimerHandle ReopenTimerHandle;

	/** The text value of the new semantic class */
	FText NewClassName;

	/** The color value of the new semantic class */
	FColor NewClassColor;

	/** References the semantic class manager need for storing modifications */
	UTextureStyleManager* TextureStyleManager;
};
