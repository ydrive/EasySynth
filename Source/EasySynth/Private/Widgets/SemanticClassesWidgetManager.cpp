// Copyright (c) YDrive Inc. All rights reserved.
// Licensed under the MIT License.

#include "Widgets/SemanticClassesWidgetManager.h"

#include "Interfaces/IMainFrameModule.h"
#include "Widgets/Layout/SUniformGridPanel.h"


FReply FSemanticClassesWidgetManager::OnManageSemanticClassesClicked()
{
	TSharedRef<SWindow> Window = SNew(SWindow)
		.Title(FText::FromString("Manage Semantic Classes"))
		.SizingRule(ESizingRule::Autosized)
		.Content()
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot()
			.Padding(2)
			// .MaxHeight(500.0f)
			[
				SNew(STextBlock)
				.Text(FText::FromString("asdf"))
			]
			+ SVerticalBox::Slot()
			.AutoHeight()
			.HAlign(HAlign_Right)
			.Padding(2)
			[
				SNew(SUniformGridPanel)
				.SlotPadding(2)
				+ SUniformGridPanel::Slot(0, 0)
				[
					SNew(SButton)
					.HAlign(HAlign_Center)
					.Text(FText::FromString("Done"))
					.OnClicked_Raw(this, &FSemanticClassesWidgetManager::OnDoneClicked)
				]
			]
		];
    WidgetWindow = Window;

	TSharedPtr<SWindow> ParentWindow;
	if (FModuleManager::Get().IsModuleLoaded("MainFrame"))
	{
		IMainFrameModule& MainFrame = FModuleManager::LoadModuleChecked<IMainFrameModule>("MainFrame");
		ParentWindow = MainFrame.GetParentWindow();
		FSlateApplication::Get().AddModalWindow(Window, ParentWindow, false);
	}

	return FReply::Handled();
}

FReply FSemanticClassesWidgetManager::OnDoneClicked()
{
    if (WidgetWindow.IsValid())
    {
        WidgetWindow.Pin()->RequestDestroyWindow();
    }
    else
    {
        UE_LOG(LogEasySynth, Error, TEXT("%s: Widget window is invalid"), *FString(__FUNCTION__));
    }

	return FReply::Handled();
}
