// Copyright (c) YDrive Inc. All rights reserved.
// Licensed under the MIT License.

#pragma once

#include "CoreMinimal.h"

class UTextureStyleManager;


/**
 * Class that manages semantic class widget interatcion
*/
class FSemanticClassesWidgetManager
{
public:
    /** Initializes the needed semantic class manager */
    void SetTextureStyleManager(UTextureStyleManager* TextureStyleManager)
    {
        SemanticClassesManager = TextureStyleManager;
    }

	/** Handles the window creation when requested */
	FReply OnManageSemanticClassesClicked();

private:
    /** Displays the color picker and for the new class color */
    FReply OnNewClassColorClicked(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent);

    /** Handles add new semantic class button click */
	FReply OnAddNewClassClicked();

    /** Handles semantic class management done click */
	FReply OnDoneClicked();

    /** Refreshes contnet of the existing semantic colors array */
    void RefreshSemanticClasses();

    /** The widget window weak pointer */
	TWeakPtr<SWindow> WidgetWindow;

    /** References the box that shows the array of existing semantic classes */
    TWeakPtr<SVerticalBox> ClassesBox;

    /** The text value of the new semantic class */
    FText NewClassName;

    /** The color value of the new semantic class */
    FColor NewClassColor;

    /** References the semantic class manager need for storing modifications */
    UTextureStyleManager* SemanticClassesManager;
};
