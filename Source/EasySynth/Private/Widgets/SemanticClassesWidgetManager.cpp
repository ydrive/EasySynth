// Copyright (c) YDrive Inc. All rights reserved.
// Licensed under the MIT License.

#include "Widgets/SemanticClassesWidgetManager.h"

#include "Interfaces/IMainFrameModule.h"
#include "Widgets/Colors/SColorBlock.h"
#include "Widgets/Layout/SUniformGridPanel.h"


FReply FSemanticClassesWidgetManager::OnManageSemanticClassesClicked()
{
	if (!FModuleManager::Get().IsModuleLoaded("MainFrame"))
	{
		return FReply::Unhandled();
	}

	// Prepare the classes box
	TSharedRef<SVerticalBox> Box = SNew(SVerticalBox);
	ClassesBox = Box;

	// Crate the window
	TSharedRef<SWindow> Window = SNew(SWindow)
		.Title(FText::FromString("Manage Semantic Classes"))
		.SizingRule(ESizingRule::Autosized)
		.Content()
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot()
			.Padding(2)
			[
				SNew(STextBlock)
				.Text(FText::FromString("Edit or remove semantic classes"))
			]
			+ SVerticalBox::Slot()
			.Padding(2)
			[
				Box
			]
			+ SVerticalBox::Slot()
			.Padding(2)
			[
				SNew(STextBlock)
				.Text(FText::FromString("Add new semantic classes"))
			]
			+ SVerticalBox::Slot()
			.Padding(2)
			[
				SNew(SEditableTextBox)
				.Text_Lambda([&](){ return NewClassName; })
				.OnTextChanged_Lambda([&](const FText& NewText){ NewClassName = NewText; })
			]
			+ SVerticalBox::Slot()
			.Padding(2)
			[
				SNew(SColorBlock)
				.Color_Lambda([&](){ return NewClassColor; })
				.ShowBackgroundForAlpha(false)
				.IgnoreAlpha(true)
				.OnMouseButtonDown_Raw(this, &FSemanticClassesWidgetManager::OnNewClassColorClicked)
				.Size(FVector2D(35.0f, 17.0f))
			]
			+ SVerticalBox::Slot()
			.Padding(2)
			[
				SNew(SButton)
				.Text(FText::FromString("Add new class"))
				.OnClicked_Raw(this, &FSemanticClassesWidgetManager::OnAddNewClassClicked)
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

	// Refresh existing semantic classes
	RefreshSemanticClasses();

	TSharedPtr<SWindow> ParentWindow;
	IMainFrameModule& MainFrame = FModuleManager::LoadModuleChecked<IMainFrameModule>("MainFrame");
	ParentWindow = MainFrame.GetParentWindow();
	FSlateApplication::Get().AddModalWindow(Window, ParentWindow, false);

	return FReply::Handled();
}

FReply FSemanticClassesWidgetManager::OnNewClassColorClicked(
	const FGeometry& MyGeometry,
	const FPointerEvent& MouseEvent)
{
	UE_LOG(LogEasySynth, Log, TEXT("%s: Starting color picker"), *FString(__FUNCTION__))
	// TODO: Start color picker
	return FReply::Handled();
}

FReply FSemanticClassesWidgetManager::OnAddNewClassClicked()
{
	UE_LOG(LogEasySynth, Log, TEXT("%s: Adding new semantic class"), *FString(__FUNCTION__))
	// TODO: Create new class
	// TODO: If successful, refresh existing classes

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
        UE_LOG(LogEasySynth, Error, TEXT("%s: Widget window is invalid"), *FString(__FUNCTION__))
    }

	return FReply::Handled();
}

void FSemanticClassesWidgetManager::RefreshSemanticClasses()
{
	if (!ClassesBox.IsValid())
    {
		UE_LOG(LogEasySynth, Error, TEXT("%s: Invalid classes box"), *FString(__FUNCTION__))
		return;
	}

	ClassesBox.Pin()->ClearChildren();

	for (const FString& Name : SemanticClassesManager->SemanticClassNames())
	{
		ClassesBox.Pin()->AddSlot()
		.Padding(FMargin(5.0f))
		[
			SNew(STextBlock)
			.Text(FText::FromString(Name))
		];
	}

}
