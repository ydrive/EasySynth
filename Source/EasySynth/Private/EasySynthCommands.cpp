// Copyright (c) YDrive Inc. All rights reserved.
// Licensed under the MIT License.

#include "EasySynthCommands.h"

#define LOCTEXT_NAMESPACE "FEasySynthModule"

void FEasySynthCommands::RegisterCommands()
{
	UI_COMMAND(OpenPluginWindow, "EasySynth", "Bring up EasySynth window", EUserInterfaceActionType::Button, FInputChord());
}

#undef LOCTEXT_NAMESPACE
