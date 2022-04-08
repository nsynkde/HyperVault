// Fill out your copyright notice in the Description page of Project Settings.


#include "VaultCommands.h"

#define LOCTEXT_NAMESPACE "VaultCommands"

// Create and Register our Commands. We only use the one so no need for a separate cpp/h file.
void FVaultCommands::RegisterCommands()
{
	UI_COMMAND(PluginAction, "Vault", "Open the Vault", EUserInterfaceActionType::Button, FInputGesture());
	UI_COMMAND(VaultExportAsset, "Export to Vault", "Open the Vault user interface to export the selected asset to the vault.", EUserInterfaceActionType::Button, FInputChord());
	UI_COMMAND(Rename, "Rename Asset", "Rename the selected Vault asset.", EUserInterfaceActionType::Button, FInputChord(FKey("F2")));
}

#undef LOCTEXT_NAMESPACE