// Copyright (c) 2022 YDrive Inc. All rights reserved.

#include "EasySynthCommands.h"

#define LOCTEXT_NAMESPACE "FEasySynthModule"

void FEasySynthCommands::RegisterCommands()
{
	UI_COMMAND(OpenPluginWindow, "EasySynth", "Bring up EasySynth window", EUserInterfaceActionType::Button, FInputChord());
}

#undef LOCTEXT_NAMESPACE
