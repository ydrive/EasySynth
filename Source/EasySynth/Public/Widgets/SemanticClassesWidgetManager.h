// Copyright (c) YDrive Inc. All rights reserved.
// Licensed under the MIT License.

#pragma once

#include "CoreMinimal.h"


/**
 * Class that manages semantic class widget interatcion
*/
class FSemanticClassesWidgetManager
{
public:
	/** Handles the window creation when requested */
	FReply OnManageSemanticClassesClicked();

private:
    /** Handles semantic class management done click */
	FReply OnDoneClicked();

    /** The widget window weak pointer */
	TWeakPtr<SWindow> WidgetWindow;
};
