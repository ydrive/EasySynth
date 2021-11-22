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
		UE_LOG(LogEasySynth, Error, TEXT("%s: Failed to load the main frame module"), *FString(__FUNCTION__))
		return FReply::Unhandled();
	}
	IMainFrameModule& MainFrame = FModuleManager::LoadModuleChecked<IMainFrameModule>("MainFrame");

	// Prepare the classes box
	TSharedRef<SVerticalBox> Box = SNew(SVerticalBox);
	ClassesBox = Box;

	// Populate the semantic classes box
	RefreshSemanticClasses();

	// Crate the window
	TSharedRef<SWindow> Window = SNew(SWindow)
		.Title(FText::FromString("Manage Semantic Classes"))
		.SizingRule(ESizingRule::Autosized)
		.SupportsMaximize(false)
		.SupportsMinimize(false)
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
			.AutoHeight()
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
			.HAlign(HAlign_Right)
			.Padding(2)
			[
				SNew(SButton)
				.HAlign(HAlign_Center)
				.Text(FText::FromString("Done"))
				.OnClicked_Raw(this, &FSemanticClassesWidgetManager::OnDoneClicked)
			]
		];
	WidgetWindow = Window;

	TSharedPtr<SWindow> ParentWindow = MainFrame.GetParentWindow();
	if (!ParentWindow.IsValid())
	{
		UE_LOG(LogEasySynth, Error, TEXT("%s: Failed to get the parent window"), *FString(__FUNCTION__))
		return FReply::Unhandled();
	}
	FSlateApplication::Get().AddModalWindow(Window, ParentWindow, false);

	return FReply::Handled();
}

void FSemanticClassesWidgetManager::OnClassNameChanged(const FText& NewText, ETextCommit::Type CommitType, FString ClassName)
{
	const bool bSuccess = SemanticClassesManager->UpdateClassName(ClassName, NewText.ToString());
	if (bSuccess)
	{
		RefreshSemanticClasses();
	}
}

FReply FSemanticClassesWidgetManager::OnUpdateClassColorClicked(
	const FGeometry& MyGeometry,
	const FPointerEvent& MouseEvent,
	FString ClassName)
{
	UE_LOG(LogEasySynth, Log, TEXT("%s: Starting color picker"), *FString(__FUNCTION__))
	// TODO: Start color picker and call SemanticClassesManager->UpdateClassName
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
	const bool bSuccess = SemanticClassesManager->NewSemanticClass(NewClassName.ToString(), NewClassColor);
	if (bSuccess)
	{
		RefreshSemanticClasses();
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

	TArray<const FSemanticClass*> SemanticClasses = SemanticClassesManager->SemanticClasses();
	for (int i = 0; i < SemanticClasses.Num(); i++)
	{
		const FSemanticClass* SemanticClass = SemanticClasses[i];
		UE_LOG(LogEasySynth, Error, TEXT("%s: Adding %s"), *FString(__FUNCTION__), *SemanticClass->Name)
		ClassesBox.Pin()->AddSlot()
		[
			SNew(SVerticalBox)
			.IsEnabled_Lambda([i](){ return i > 0; })
			+ SVerticalBox::Slot()
			.Padding(2)
			[
				SNew(SEditableTextBox)
				.Text_Lambda([SemanticClass](){ return FText::FromString(SemanticClass->Name); })
				.OnTextCommitted_Raw(this, &FSemanticClassesWidgetManager::OnClassNameChanged, SemanticClass->Name)
			]
			+ SVerticalBox::Slot()
			.Padding(2)
			[
				SNew(SColorBlock)
				.Color_Lambda([SemanticClass](){ return SemanticClass->Color; })
				.ShowBackgroundForAlpha(false)
				.IgnoreAlpha(true)
				.OnMouseButtonDown_Raw(this, &FSemanticClassesWidgetManager::OnUpdateClassColorClicked, SemanticClass->Name)
			]
			// TODO: Remove a class
		];
	}
}
