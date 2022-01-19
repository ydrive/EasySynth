// Copyright (c) 2022 YDrive Inc. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Framework/Commands/Commands.h"
#include "EasySynthStyle.h"


class FEasySynthCommands : public TCommands<FEasySynthCommands>
{
public:
	FEasySynthCommands()
		: TCommands<FEasySynthCommands>(
			TEXT("EasySynth"),
			NSLOCTEXT("Contexts", "EasySynth", "EasySynth Plugin"),
			NAME_None,
			FEasySynthStyle::GetStyleSetName()) {}

	// TCommands<> interface
	void RegisterCommands() override;

public:
	TSharedPtr< FUICommandInfo > OpenPluginWindow;
};
