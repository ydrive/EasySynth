// Copyright Ydrive 2021

#pragma once

#include "CoreMinimal.h"
#include "Framework/Commands/Commands.h"
#include "SimpleSynthStyle.h"


class FSimpleSynthCommands : public TCommands<FSimpleSynthCommands>
{
public:
	FSimpleSynthCommands()
		: TCommands<FSimpleSynthCommands>(
			TEXT("SimpleSynth"),
			NSLOCTEXT("Contexts", "SimpleSynth", "SimpleSynth Plugin"),
			NAME_None,
			FSimpleSynthStyle::GetStyleSetName()) {}

	// TCommands<> interface
	void RegisterCommands() override;

public:
	TSharedPtr< FUICommandInfo > OpenPluginWindow;
};
