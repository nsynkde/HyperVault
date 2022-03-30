// Fill out your copyright notice in the Description page of Project Settings.


#include "SSettingsWindow.h"
#include "VaultStyle.h"
#include "VaultSettings.h"

#define LOCTEXT_NAMESPACE "NSVaultSettingsWindow"

void SSettingsWindow::Construct(const FArguments& InArgs)
{

	FString Readout1 = "Settings Page - Probably not Coming Soon \n\nAdjust your setting in the Local & Global JSON files:";
	
	FString Readout2 = "Local (Per-User) Settings File - " + FVaultSettings::Get().LocalSettingsFilePathFull;
	FString Readout3 = "Default Global (Per-Team) Settings File - " + FVaultSettings::Get().DefaultGlobalsPath;

	FString ReadoutFull = Readout1 + LINE_TERMINATOR+ LINE_TERMINATOR + Readout2 + LINE_TERMINATOR+ LINE_TERMINATOR + Readout3;

	ChildSlot
		[
			SNew(SVerticalBox)
			+SVerticalBox::Slot()
			.FillHeight(1)
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot()
				.FillHeight(1)
				.VAlign(VAlign_Center)
				[
					SNew(STextBlock)
					.Text(FText::FromString(ReadoutFull))
					.TextStyle(FVaultStyle::Get(), "MetaTitleText")
					.Justification(ETextJustify::Center)
				]
				+ SVerticalBox::Slot()
				.AutoHeight()
				.Padding(FMargin(5, 5, 5, 5))
				.VAlign(VAlign_Center)
				[
					SNew(SButton)
					.ContentPadding(FMargin(6.f))
					.ButtonStyle(FEditorStyle::Get(), "FlatButton.Info")
					.OnClicked(this, &SSettingsWindow::OpenLocalSettingsFile)
					[
						SNew(STextBlock)
						.Justification(ETextJustify::Center)
						.Text(LOCTEXT("OpenLocalSettingsLabel", "Open Local Settings File"))
						.TextStyle(FEditorStyle::Get(), "NormalText.Important")
					]
				]
			]
		];
}

FReply SSettingsWindow::OpenLocalSettingsFile()
{
	if (!FPaths::FileExists(*FVaultSettings::Get().LocalSettingsFilePathFull)) {
		const FText ErrorMsg = LOCTEXT("LocalSettingsMissingText", "Your local settings file is missing (this should not have happened)\n Try re-initializing the plugin.");
		const FText ErrorTitle = LOCTEXT("LocalSettingsMissingTitle", "Local Settings Missing");

		const EAppReturnType::Type ErrorDialog = FMessageDialog::Open(
			EAppMsgType::Ok, ErrorMsg, &ErrorTitle);
		return FReply::Handled();
	}
	FPlatformProcess::LaunchFileInDefaultExternalApplication(*FVaultSettings::Get().LocalSettingsFilePathFull);
	return FReply::Handled();
}


#undef LOCTEXT_NAMESPACE

